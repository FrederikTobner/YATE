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
 * @file config_reader.h
 * @brief File containing the declaration of the configuration reader and the corresponding functions.
 */

#ifndef YATE_CONFIGURATION_READER_H_
#define YATE_CONFIGURATION_READER_H_

#include "stddef.h"

/// Result of reading an editor configuration file
typedef struct
{
  /// The size of a tabstop converted to whitespaces
  size_t tabStopSize;
  /// The amount of seconds a message is diplayed within the editor, before it dissappears
  size_t messageDisplayDuration;
} configuration_reader_result_t;

/// Parses the editor configuration file located at the users home directory
/// @return The configuration in the editor configuration file converted to a configuration_reader_result_t
configuration_reader_result_t * configuration_reader_read_configuration_file();

#endif
