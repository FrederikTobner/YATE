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
 * @file copy_buffer.c
 * @brief File containing the implementation of a copy buffer.
 */

#include "copy_buffer.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

void copy_buffer_free(copy_buffer_t * buffer)
{
    free(buffer->buffer);
    copy_buffer_init(buffer);
}

void copy_buffer_init(copy_buffer_t * buffer)
{
    buffer->length = 0;
    buffer->buffer = NULL;
}

void copy_buffer_write(copy_buffer_t * buffer, char const * str, uint32_t length)
{
    char * new = malloc(length);
    if (new == NULL)
        return;
    memcpy(new, str, length);
    if(buffer->buffer)
        free(buffer->buffer);
    buffer->buffer = new;
    buffer->length = length;
}