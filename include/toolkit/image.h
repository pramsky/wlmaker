/* ========================================================================= */
/**
 * @file image.h
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
#ifndef __WLMTK_IMAGE_H__
#define __WLMTK_IMAGE_H__

#include <libbase/libbase.h>

#include "element.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Forward declaration: State of a toolkit image. */
typedef struct _wlmtk_image_t wlmtk_image_t;

/**
 * Creates a toolkit image: An element showing the image.
 *
 * @param image_path_ptr
 *
 * @return Pointer to the toolkit image, or NULL on error.
 */
wlmtk_image_t *wlmtk_image_create(
    const char *image_path_ptr);

/**
 * Creates a toolkit image, scaled while preserving aspect ratio.
 *
 * @param image_path_ptr
 * @param width
 * @param height
 *
 * @return Pointer to the toolkit image or NULL on error.
 */
wlmtk_image_t *wlmtk_image_create_scaled(
    const char *image_path_ptr,
    int width,
    int height);

/**
 * Destroys the toolkit image.
 *
 * @param image_ptr
 */
void wlmtk_image_destroy(wlmtk_image_t *image_ptr);

/** @return the parent @ref wlmtk_element_t of `image_ptr`. */
wlmtk_element_t *wlmtk_image_element(wlmtk_image_t *image_ptr);

/** Unit test cases. */
extern const bs_test_case_t wlmtk_image_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __IMAGE_H__ */
/* == End of image.h ================================================== */
