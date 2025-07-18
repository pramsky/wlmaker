/* ========================================================================= */
/**
 * @file tile.h
 *
 * @copyright
 * Copyright 2024 Google LLC
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
#ifndef __WLMTK_TILE_H__
#define __WLMTK_TILE_H__

struct _wlmtk_tile_t;
/** Forward declaration: State of a tile. */
typedef struct _wlmtk_tile_t wlmtk_tile_t;

#include <libbase/libbase.h>
#include <stdbool.h>
#include <stdint.h>

#include "buffer.h"
#include "container.h"
#include "element.h"
#include "style.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Style options for the tile. */
typedef struct {
    /** Fill style for the tile's background. */
    wlmtk_style_fill_t        fill;
    /** Size of the tile, in pixels. Tiles are of quadratic shape. */
    uint64_t                  size;
    /** Content size of what's in the tile, in pixels. */
    uint64_t                  content_size;
    /** Width of the bezel. */
    uint64_t                  bezel_width;
} wlmtk_tile_style_t;

/** State of a tile. */
struct _wlmtk_tile_t {
    /** A tile is a container. Holds a background and contents. */
    wlmtk_container_t         super_container;
    /** Virtual method table of the superclass' container. */
    wlmtk_container_vmt_t     orig_super_container_vmt;

    /** The tile background is modelled as @ref wlmtk_buffer_t. */
    wlmtk_buffer_t            buffer;

    /** Style to be used for this tile. */
    wlmtk_tile_style_t        style;

    /** Holds the tile's background, used in @ref wlmtk_tile_t::buffer. */
    struct wlr_buffer         *background_wlr_buffer_ptr;

    /** References the content element from @ref wlmtk_tile_set_content. */
    wlmtk_element_t           *content_element_ptr;
    /** References the content element from @ref wlmtk_tile_set_overlay. */
    wlmtk_element_t           *overlay_element_ptr;
};

/**
 * Initializes the tile.
 *
 * @param tile_ptr
 * @param style_ptr
 *
 * @return true on success.
 */
bool wlmtk_tile_init(
    wlmtk_tile_t *tile_ptr,
    const wlmtk_tile_style_t *style_ptr);

/**
 * Un-initializes the tile.
 *
 * @param tile_ptr
 */
void wlmtk_tile_fini(wlmtk_tile_t *tile_ptr);

/**
 * Sets (overwrites) the default tile's background buffer.
 *
 * This permits specific tiles, eg. a Dock Clip to include active elements in
 * the background, or change the bezel or texture.
 *
 * @param tile_ptr
 * @param wlr_buffer_ptr      Points to a `struct wlr_buffer`. The tile will
 *                            add a buffer lock, so the caller may safely
 *                            drop or unlock the buffer.
 *                            The buffer must match the tile's size.
 *
 * @return false if the buffer did not match the tile size.
 */
bool wlmtk_tile_set_background_buffer(
    wlmtk_tile_t *tile_ptr,
    struct wlr_buffer *wlr_buffer_ptr);

/**
 * Sets `element_ptr` as the content of `tile_ptr`.
 *
 * TODO(kaeser@gubbe.ch): Flesh out the behaviour -- permit only 1 content?
 * Does the tile claim ownerwhip? How to reset the content?
 *
 * @param tile_ptr
 * @param element_ptr
 */
void wlmtk_tile_set_content(
    wlmtk_tile_t *tile_ptr,
    wlmtk_element_t *element_ptr);

/**
 * Sets `element_ptr` as the overlay of `tile_ptr`.
 *
 * TODO(kaeser@gubbe.ch): Flesh out the behaviour -- permit only 1 overlay?
 * Does the tile claim ownerwhip? How to reset the overlay?
 *
 * @param tile_ptr
 * @param element_ptr
 */
void wlmtk_tile_set_overlay(
    wlmtk_tile_t *tile_ptr,
    wlmtk_element_t *element_ptr);

/** @return the superclass' @ref wlmtk_element_t of `tile_ptr`. */
wlmtk_element_t *wlmtk_tile_element(wlmtk_tile_t *tile_ptr);

/** Unit test cases for @ref wlmtk_tile_t. */
extern const bs_test_case_t wlmtk_tile_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __WLMTK_TILE_H__ */
/* == End of tile.h ======================================================== */
