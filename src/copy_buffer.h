/****************************************************************************
 * Copyright (C) 2022 by Frederik Tobner                                    *
 *                                                                          *
 * This file is part of Yate.                                             *
 *                                                                          *
 * Permission to use, copy, modify, and distribute this software and its    *
 * documentation under the terms of the GNU General Public License is       *
 * hereby granted.                                                          *
 * No representations are made about the suitability of this software for   *
 * any purpose.                                                             *
 * It is provided "as is" without express or implied warranty.              *
 * See the <https://www.gnu.org/licenses/gpl-3.0.html/>GNU General Public   *
 * License for more details.                                                *
 ****************************************************************************/

/**
 * @file copy_buffer.h
 * @brief File containing the declaration of the copy buffer and the corresponding functions.
 */

#ifndef YATE_COPY_BUFFER_H_
#define YATE_COPY_BUFFER_H_

#include <stddef.h>

/// Copy buffer
typedef struct 
{
    /// The underlying character buffer
    char * buffer;
    /// The length of the character buffer
    size_t length;
} copy_buffer_t;

/// @brief Frees the copy buffer
/// @param buffer the copy buffer that is freed
void copy_buffer_free(copy_buffer_t * buffer);

/// @brief Initializes a copy buffer
/// @param buffer The copy buffer that is initialized
void copy_buffer_init(copy_buffer_t * buffer);

/// @brief Writes a string to an copy buffer
/// @param buffer The buffer where the string is writen to
/// @param str The string that is appended
/// @param length The length of the string that is writen to the buffer
void copy_buffer_write(copy_buffer_t * buffer, char const * str, int length);

#endif
