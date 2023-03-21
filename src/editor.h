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
 * @file editor.h
 * @brief File containing the declarations of the editor functionality.
 */

#ifndef YATE_EDITOR_H_
#define YATE_EDITOR_H_

#include "config_reader.h"

/// @brief Enables raw mode for the current terminal session instead of
/// canonical mode
void editor_enable_raw_mode();

/// @brief Initializes the editor
void editor_initialize(configuration_reader_result_t *config);

/// @brief Opens a file and renders the content of the file
/// @param filePath The path of the file that is opened, read and rendered
void editor_open(char const *filePath);

/// @brief Processes a single keypress
void editor_process_keypress();

/// @brief Refreshes the content that is rendered by the editor
void editor_refresh_screen();

#endif