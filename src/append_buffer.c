/****************************************************************************
 * Copyright (C) 2022 by Frederik Tobner                                    *
 *                                                                          *
 * This file is part of Yate.                                               *
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
 * @file append_buffer.c
 * @brief File containing the implementation of a append buffer.
 */

#include "append_buffer.h"

#include <stdlib.h>
#include <string.h>

void append_buffer_append_string(append_buffer_t *buffer, const char *str,
                                 int length) {
  char *new = realloc(buffer->buffer, buffer->length + length);
  if (new == NULL)
    return;
  memcpy(&new[buffer->length], str, length);
  buffer->buffer = new;
  buffer->length += length;
}

void append_buffer_free(append_buffer_t *buffer) { free(buffer->buffer); }

void append_buffer_init(append_buffer_t *buffer) {
  buffer->length = 0;
  buffer->buffer = NULL;
}