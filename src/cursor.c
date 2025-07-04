/* ========================================================================= */
/**
 * @file cursor.c
 *
 * @copyright
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cursor.h"

#include <libbase/libbase.h>
#include <inttypes.h>
#include <stdlib.h>
#define WLR_USE_UNSTABLE
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_xcursor_manager.h>
#undef WLR_USE_UNSTABLE

#include "config.h"
#include "idle.h"
#include "toolkit/toolkit.h"

/* == Declarations ========================================================= */

static void handle_motion(
    struct wl_listener *listener_ptr,
    void *data_ptr);
static void handle_motion_absolute(
    struct wl_listener *listener_ptr,
    void *data_ptr);
static void handle_button(
    struct wl_listener *listener_ptr,
    void *data_ptr);
static void handle_axis(
    struct wl_listener *listener_ptr,
    void *data_ptr);
static void handle_frame(
    struct wl_listener *listener_ptr,
    void *data_ptr);

static void handle_seat_request_set_cursor(
    struct wl_listener *listener_ptr,
    void *data_ptr);

static void process_motion(wlmaker_cursor_t *cursor_ptr, uint32_t time_msec);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
wlmaker_cursor_t *wlmaker_cursor_create(
    wlmaker_server_t *server_ptr,
    struct wlr_output_layout *wlr_output_layout_ptr)
{
    wlmaker_cursor_t *cursor_ptr = logged_calloc(1, sizeof(wlmaker_cursor_t));
    if (NULL == cursor_ptr) return NULL;
    cursor_ptr->server_ptr = server_ptr;

    // tinywl: wlr_cursor is a utility tracking the cursor image shown on
    // screen.
    cursor_ptr->wlr_cursor_ptr = wlr_cursor_create();
    if (NULL == cursor_ptr->wlr_cursor_ptr) {
        bs_log(BS_ERROR, "Failed wlr_cursor_create()");
        wlmaker_cursor_destroy(cursor_ptr);
        return NULL;
    }
    wlr_cursor_attach_output_layout(
        cursor_ptr->wlr_cursor_ptr, wlr_output_layout_ptr);

    cursor_ptr->wlr_xcursor_manager_ptr = wlr_xcursor_manager_create(
        server_ptr->style.cursor.name_ptr,
        server_ptr->style.cursor.size);
    if (NULL == cursor_ptr->wlr_xcursor_manager_ptr) {
        bs_log(BS_ERROR, "Failed wlr_xcursor_manager_create(%s, %"PRIu64")",
               server_ptr->style.cursor.name_ptr,
               server_ptr->style.cursor.size);
        wlmaker_cursor_destroy(cursor_ptr);
        return NULL;
    }
    if (!wlr_xcursor_manager_load(cursor_ptr->wlr_xcursor_manager_ptr,
                                  config_output_scale)) {

        bs_log(BS_ERROR, "Failed wlr_xcursor_manager_load() for %s, %"PRIu64,
               server_ptr->style.cursor.name_ptr,
               server_ptr->style.cursor.size);
        wlmaker_cursor_destroy(cursor_ptr);
        return NULL;
    }

    cursor_ptr->pointer_ptr = wlmtk_pointer_create(
        cursor_ptr->wlr_cursor_ptr,
        cursor_ptr->wlr_xcursor_manager_ptr);
    if (NULL == cursor_ptr->pointer_ptr) {
        wlmaker_cursor_destroy(cursor_ptr);
        return NULL;
    }

    wl_signal_init(&cursor_ptr->position_updated);

    // tinywl: wlr_cursor *only* displays an image on screen. It does not move
    // around when the pointer moves. However, we can attach input devices to
    // it, and it will generate aggregate events for all of them. In these
    // events, we can choose how we want to process them, forwarding them to
    // clients and moving the cursor around. More detail on this process is
    // described in the input handling blog post:
    //
    // https://drewdevault.com/2018/07/17/Input-handling-in-wlroots.html

    wlmtk_util_connect_listener_signal(
        &cursor_ptr->wlr_cursor_ptr->events.motion,
        &cursor_ptr->motion_listener,
        handle_motion);
    wlmtk_util_connect_listener_signal(
        &cursor_ptr->wlr_cursor_ptr->events.motion_absolute,
        &cursor_ptr->motion_absolute_listener,
        handle_motion_absolute);
    wlmtk_util_connect_listener_signal(
        &cursor_ptr->wlr_cursor_ptr->events.button,
        &cursor_ptr->button_listener,
        handle_button);
    wlmtk_util_connect_listener_signal(
        &cursor_ptr->wlr_cursor_ptr->events.axis,
        &cursor_ptr->axis_listener,
        handle_axis);
    wlmtk_util_connect_listener_signal(
        &cursor_ptr->wlr_cursor_ptr->events.frame,
        &cursor_ptr->frame_listener,
        handle_frame);

    wlmtk_util_connect_listener_signal(
        &cursor_ptr->server_ptr->wlr_seat_ptr->events.request_set_cursor,
        &cursor_ptr->seat_request_set_cursor_listener,
        handle_seat_request_set_cursor);

    return cursor_ptr;
}

/* ------------------------------------------------------------------------- */
void wlmaker_cursor_destroy(wlmaker_cursor_t *cursor_ptr)
{
    if (NULL != cursor_ptr->pointer_ptr) {
        wlmtk_pointer_destroy(cursor_ptr->pointer_ptr);
        cursor_ptr->pointer_ptr = NULL;
    }

    if (NULL != cursor_ptr->wlr_xcursor_manager_ptr) {
        wlr_xcursor_manager_destroy(cursor_ptr->wlr_xcursor_manager_ptr);
        cursor_ptr->wlr_xcursor_manager_ptr = NULL;
    }

    if (NULL != cursor_ptr->wlr_cursor_ptr) {
        wlr_cursor_destroy(cursor_ptr->wlr_cursor_ptr);
        cursor_ptr->wlr_cursor_ptr = NULL;
    }

    free(cursor_ptr);
}

/* ------------------------------------------------------------------------- */
void wlmaker_cursor_attach_input_device(
    wlmaker_cursor_t *cursor_ptr,
    struct wlr_input_device *wlr_input_device_ptr)
{
     wlr_cursor_attach_input_device(
         cursor_ptr->wlr_cursor_ptr,
         wlr_input_device_ptr);
}

/* ------------------------------------------------------------------------- */
void wlmaker_cursor_get_position(
    const wlmaker_cursor_t *cursor_ptr,
    double *x_ptr,
    double *y_ptr)
{
    if (NULL != x_ptr) *x_ptr = cursor_ptr->wlr_cursor_ptr->x;
    if (NULL != y_ptr) *y_ptr = cursor_ptr->wlr_cursor_ptr->y;
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/**
 * Handler for the `motion` event of `wlr_cursor`.
 *
 * @param listener_ptr
 * @param data_ptr Points to a `wlr_pointer_motion_event`.
 */
void handle_motion(struct wl_listener *listener_ptr,
                   void *data_ptr)
{
    wlmaker_cursor_t *cursor_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_cursor_t, motion_listener);
    struct wlr_pointer_motion_event *wlr_pointer_motion_event_ptr = data_ptr;

    wlmaker_idle_monitor_reset(cursor_ptr->server_ptr->idle_monitor_ptr);

    wlr_cursor_move(
        cursor_ptr->wlr_cursor_ptr,
        &wlr_pointer_motion_event_ptr->pointer->base,
        wlr_pointer_motion_event_ptr->delta_x,
        wlr_pointer_motion_event_ptr->delta_y);

    process_motion(
        cursor_ptr,
        wlr_pointer_motion_event_ptr->time_msec);
}

/* ------------------------------------------------------------------------- */
/**
 * Handler for the `motion_absolute` event of `wlr_cursor`.
 *
 * @param listener_ptr
 * @param data_ptr Points to a `wlr_pointer_motion_absolute_event`.
 */
void handle_motion_absolute(struct wl_listener *listener_ptr,
                            void *data_ptr)
{
    wlmaker_cursor_t *cursor_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_cursor_t, motion_absolute_listener);
    struct wlr_pointer_motion_absolute_event
        *wlr_pointer_motion_absolute_event_ptr = data_ptr;

    wlmaker_idle_monitor_reset(cursor_ptr->server_ptr->idle_monitor_ptr);

    wlr_cursor_warp_absolute(
        cursor_ptr->wlr_cursor_ptr,
        &wlr_pointer_motion_absolute_event_ptr->pointer->base,
        wlr_pointer_motion_absolute_event_ptr->x,
        wlr_pointer_motion_absolute_event_ptr->y);

    process_motion(
        cursor_ptr,
        wlr_pointer_motion_absolute_event_ptr->time_msec);
}

/* ------------------------------------------------------------------------- */
/**
 * Handler for the `button` event of `wlr_cursor`.
 *
 * @param listener_ptr
 * @param data_ptr Points to a `wlr_pointer_button_event`.
 */
void handle_button(struct wl_listener *listener_ptr,
                   void *data_ptr)
{
    wlmaker_cursor_t *cursor_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_cursor_t, button_listener);
    struct wlr_pointer_button_event *wlr_pointer_button_event_ptr = data_ptr;

    wlmaker_idle_monitor_reset(cursor_ptr->server_ptr->idle_monitor_ptr);

    wlmtk_root_pointer_button(
        cursor_ptr->server_ptr->root_ptr,
        wlr_pointer_button_event_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Handler for the `axis` event of `wlr_cursor`.
 *
 * @param listener_ptr
 * @param data_ptr Points to a `wlr_pointer_axis_event`.
 */
void handle_axis(struct wl_listener *listener_ptr,
                 void *data_ptr)
{
    wlmaker_cursor_t *cursor_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_cursor_t, axis_listener);
    struct wlr_pointer_axis_event *wlr_pointer_axis_event_ptr = data_ptr;

    wlmaker_idle_monitor_reset(cursor_ptr->server_ptr->idle_monitor_ptr);

    wlmtk_root_pointer_axis(
        cursor_ptr->server_ptr->root_ptr,
        wlr_pointer_axis_event_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Handler for the `frame` event of `wlr_cursor`.
 *
 * @param listener_ptr
 * @param data_ptr
 */
void handle_frame(struct wl_listener *listener_ptr,
                  __UNUSED__ void *data_ptr)
{
    wlmaker_cursor_t *cursor_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_cursor_t, frame_listener);

    /* Notify the client with pointer focus of the frame event. */
    wlr_seat_pointer_notify_frame(cursor_ptr->server_ptr->wlr_seat_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Handler for the `request_set_cursor` event of `wlr_seat`.
 *
 * This event is raised when a client provides a cursor image. It is accepted
 * only if the client also has the pointer focus.
 *
 * @param listener_ptr
 * @param data_ptr Points to a `wlr_seat_pointer_request_set_cursor_event`.
 */
void handle_seat_request_set_cursor(
    struct wl_listener *listener_ptr,
    void *data_ptr)
{
    wlmaker_cursor_t *cursor_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_cursor_t, seat_request_set_cursor_listener);
    struct wlr_seat_pointer_request_set_cursor_event
        *wlr_seat_pointer_request_set_cursor_event_ptr = data_ptr;

    struct wlr_seat_client *focused_wlr_seat_client_ptr =
        cursor_ptr->server_ptr->wlr_seat_ptr->pointer_state.focused_client;
    if (focused_wlr_seat_client_ptr ==
        wlr_seat_pointer_request_set_cursor_event_ptr->seat_client) {
        wlr_cursor_set_surface(
            cursor_ptr->wlr_cursor_ptr,
            wlr_seat_pointer_request_set_cursor_event_ptr->surface,
            wlr_seat_pointer_request_set_cursor_event_ptr->hotspot_x,
            wlr_seat_pointer_request_set_cursor_event_ptr->hotspot_y);

    } else {
        bs_log(BS_WARNING, "request_set_cursor called without pointer focus.");
    }
}

/* ------------------------------------------------------------------------- */
/**
 * Processes the cursor motion: Lookups up the view & surface under the
 * pointer and sets (respectively: clears) the pointer focus. Also sets the
 * default cursor image, if no view is given (== no client-side cursor).
 *
 * @param cursor_ptr
 * @param time_msec
 */
void process_motion(wlmaker_cursor_t *cursor_ptr, uint32_t time_msec)
{
    wl_signal_emit_mutable(
        &cursor_ptr->position_updated,
        cursor_ptr->wlr_cursor_ptr);

    // TODO(kaeser@gubbe.ch): also make this an event-based callback.
    wlmtk_root_pointer_motion(
        cursor_ptr->server_ptr->root_ptr,
        cursor_ptr->wlr_cursor_ptr->x,
        cursor_ptr->wlr_cursor_ptr->y,
        time_msec,
        cursor_ptr->pointer_ptr);
}

/* == End of cursor.c ====================================================== */
