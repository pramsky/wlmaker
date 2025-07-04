/* ========================================================================= */
/**
 * @file xwl_toplevel.c
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
#if defined(WLMAKER_HAVE_XWAYLAND)

#include "xwl_toplevel.h"

#include <libbase/libbase.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

#include "config.h"
#include "tl_menu.h"

/* == Declarations ========================================================= */

/** State of a XWayland toplevel window. */
struct _wlmaker_xwl_toplevel_t {
    /** Corresponding toolkit window. */
    wlmtk_window_t            *window_ptr;

    /** Back-link to server. */
    wlmaker_server_t          *server_ptr;

    /** The toplevel's window menu. */
    wlmaker_tl_menu_t         *tl_menu_ptr;

    /** Listener for `map` event of the surface. */
    struct wl_listener        surface_map_listener;
    /** Listener for `unmap` event of the surface. */
    struct wl_listener        surface_unmap_listener;
};

static void _xwl_toplevel_handle_surface_map(
    struct wl_listener *listener_ptr,
    void *data_ptr);
static void _xwl_toplevel_handle_surface_unmap(
    struct wl_listener *listener_ptr,
    void *data_ptr);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
wlmaker_xwl_toplevel_t *wlmaker_xwl_toplevel_create(
    wlmaker_xwl_content_t *content_ptr,
    wlmaker_server_t *server_ptr)
{
    wlmaker_xwl_toplevel_t *xwl_toplevel_ptr = logged_calloc(
        1, sizeof(wlmaker_xwl_toplevel_t));
    if (NULL == xwl_toplevel_ptr) return NULL;
    xwl_toplevel_ptr->server_ptr = server_ptr;

    xwl_toplevel_ptr->window_ptr = wlmtk_window_create(
        wlmtk_content_from_xwl_content(content_ptr),
        &server_ptr->style.window,
        &server_ptr->style.menu,
        server_ptr->wlr_seat_ptr);
    if (NULL == xwl_toplevel_ptr->window_ptr) {
        wlmaker_xwl_toplevel_destroy(xwl_toplevel_ptr);
        return NULL;
    }

    xwl_toplevel_ptr->tl_menu_ptr = wlmaker_tl_menu_create(
        xwl_toplevel_ptr->window_ptr,
        server_ptr);
    if (NULL == xwl_toplevel_ptr->tl_menu_ptr) {
        wlmaker_xwl_toplevel_destroy(xwl_toplevel_ptr);
        return NULL;
    }

    wl_signal_emit(&server_ptr->window_created_event,
                   xwl_toplevel_ptr->window_ptr);

    wlmtk_surface_connect_map_listener_signal(
        wlmtk_surface_from_xwl_content(content_ptr),
        &xwl_toplevel_ptr->surface_map_listener,
        _xwl_toplevel_handle_surface_map);
    wlmtk_surface_connect_unmap_listener_signal(
        wlmtk_surface_from_xwl_content(content_ptr),
        &xwl_toplevel_ptr->surface_unmap_listener,
        _xwl_toplevel_handle_surface_unmap);

    return xwl_toplevel_ptr;
}

/* ------------------------------------------------------------------------- */
void wlmaker_xwl_toplevel_destroy(
    wlmaker_xwl_toplevel_t *xwl_toplevel_ptr)
{
    if (NULL != xwl_toplevel_ptr->window_ptr) {
        wl_signal_emit(&xwl_toplevel_ptr->server_ptr->window_destroyed_event,
                       xwl_toplevel_ptr->window_ptr);

        BS_ASSERT(NULL ==
                  wlmtk_window_get_workspace(xwl_toplevel_ptr->window_ptr));
        wlmtk_window_destroy(xwl_toplevel_ptr->window_ptr);
        xwl_toplevel_ptr->window_ptr = NULL;
    }

    wl_list_remove(&xwl_toplevel_ptr->surface_unmap_listener.link);
    wl_list_remove(&xwl_toplevel_ptr->surface_map_listener.link);

    if (NULL != xwl_toplevel_ptr->tl_menu_ptr) {
        wlmaker_tl_menu_destroy(xwl_toplevel_ptr->tl_menu_ptr);
        xwl_toplevel_ptr->tl_menu_ptr = NULL;
    }

    free(xwl_toplevel_ptr);
}

/* ------------------------------------------------------------------------- */
void wlmaker_xwl_toplevel_set_decorations(
    wlmaker_xwl_toplevel_t *xwl_toplevel_ptr,
    bool decorated)
{
    wlmtk_window_set_server_side_decorated(
        xwl_toplevel_ptr->window_ptr,
        decorated);
}

/* ------------------------------------------------------------------------- */
wlmtk_window_t *wlmtk_window_from_xwl_toplevel(
    wlmaker_xwl_toplevel_t *xwl_toplevel_ptr)
{
    return xwl_toplevel_ptr->window_ptr;
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/** Surface map handler: also indicates the window can be mapped. */
void _xwl_toplevel_handle_surface_map(
    struct wl_listener *listener_ptr,
    __UNUSED__ void *data_ptr)
{
    wlmaker_xwl_toplevel_t *xwl_toplevel_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_xwl_toplevel_t, surface_map_listener);

    wlmtk_workspace_t *workspace_ptr =
        wlmtk_root_get_current_workspace(
            xwl_toplevel_ptr->server_ptr->root_ptr);

    wlmtk_workspace_map_window(workspace_ptr, xwl_toplevel_ptr->window_ptr);
    wlmtk_window_set_position(xwl_toplevel_ptr->window_ptr, 40, 30);
}

/* ------------------------------------------------------------------------- */
/** Surface unmap: indicates the window should be unmapped. */
void _xwl_toplevel_handle_surface_unmap(
    struct wl_listener *listener_ptr,
    __UNUSED__ void *data_ptr)
{
    wlmaker_xwl_toplevel_t *xwl_toplevel_ptr = BS_CONTAINER_OF(
        listener_ptr, wlmaker_xwl_toplevel_t, surface_unmap_listener);

    wlmtk_workspace_unmap_window(
        wlmtk_window_get_workspace(xwl_toplevel_ptr->window_ptr),
        xwl_toplevel_ptr->window_ptr);
}

#endif  // defined(WLMAKER_HAVE_XWAYLAND)
/* == End of xwl_toplevel.c ================================================ */
