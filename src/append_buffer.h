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
 * @file append_buffer.h
 * @brief File containing the declaration of the append buffer and the corresponding functions.
 */

#ifndef YATE_APPEND_BUFFER_H_
#define YATE_APPEND_BUFFER_H_

#include <stddef.h>

/// Append buffer
typedef struct 
{
    /// Pointer to the underlying buffer
    char * buffer;
    /// The length of the buffer
    size_t length;
} append_buffer_t;

/// @brief Appends a string to an append buffer
/// @param buffer The buffer where the string is appended
/// @param str The string that is appended
/// @param length The length of the string that is appended
void append_buffer_append_string(append_buffer_t * buffer, char const * str, int length);

/// @brief Frees the append buffer
/// @param buffer the append buffer that is freed
void append_buffer_free(append_buffer_t * buffer);

#endif