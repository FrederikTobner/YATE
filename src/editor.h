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
 * @file editor.c
 * @brief File containing the declarations of the editor functionality.
 */

#ifndef YATE_EDITOR_H_
#define YATE_EDITOR_H_

#include "config_reader.h"

/// @brief 
void editor_enable_raw_mode();

/// @brief 
void editor_initialize(configuration_reader_result_t * config);

/// @brief 
/// @param filePath 
void editor_open(char const * filePath);

/// @brief 
void editor_process_keypress();

/// @brief
void editor_refresh_screen();

/// @brief
void editor_show_help();

#endif