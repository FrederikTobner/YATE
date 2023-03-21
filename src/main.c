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
 * @file main.c
 * @brief File containing main entry point of the editor.
 */

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config_reader.h"
#include "editor.h"
#include "project_config.h"

static void printConsoleHelp();
static void printHotKeys();
static void printSettings();

/// @brief Main entry point of the editor
/// @param argc The amount of arguments that were specified by the user
/// @param argv The arguments that were spepcified by the user
/// @return 0 (Unreachable)
int main(int argc, char const **argv) {
  if (argc >= 2) {
    if (!strcmp(argv[1], "--config") || !strcmp(argv[1], "-c")) {
      printSettings();
      return 0;
    } else if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
      printConsoleHelp();
      return 0;
    } else if (!strcmp(argv[1], "--keys") || !strcmp(argv[1], "-k")) {
      printHotKeys();
      return 0;
    } else if (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-v")) {
      printf("%s version %d.%d\n", PROJECT_NAME, PROJECT_VERSION_MAJOR,
             PROJECT_VERSION_MINOR);
#ifdef RELEASE_BUILD_TYPE
      printf("Build type: Release\n");
#endif
#ifdef DEBUG_BUILD_TYPE
      printf("Build type: Debug\n");
#endif
      return 0;
    }
  }
  configuration_reader_result_t *config =
      configuration_reader_read_configuration_file();
  editor_enable_raw_mode();
  editor_initialize(config);
  if (argc >= 2)
    editor_open(argv[1]);

  while (1) {
    editor_refresh_screen();
    editor_process_keypress();
  }
  return 0;
}

/// @brief Displays the help of the editor in the console
static void printConsoleHelp() {
  printf("%s version %d.%d\n", PROJECT_NAME, PROJECT_VERSION_MAJOR,
         PROJECT_VERSION_MINOR);
  printf("Usage yate <option> <filepath>\n\n");
  printf("Options\n");
  printf("  -c, --config\t\tShows configurable settings of the editor\n");
  printf("  -h, --help\t\tDisplay this help\n");
  printf("  -k, --key\t\tShows hotkeys of the editor\n");
  printf("  -v, --version\t\tShows the version of the installed editor\n");
}

/// @brief Displays the available hotkeys of the editor
static void printHotKeys() {
  printf("HotKeys\n");
  printf("  ctrl-d\t\tYanks and deletes the current line\n");
  printf("  ctrl-f\t\tFind occurences in file\n");
  printf("  ctrl-h\t\tShows help\n");
  printf("  ctrl-o\t\tOpens file\n");
  printf("  ctrl-p\t\tPaste last yanked content\n");
  printf("  ctrl-q\t\tExit the editor\n");
  printf("  ctrl-s\t\tSaves the currently opened file\n");
  printf("  ctrl-x\t\tExecute the currently opened file\n");
  printf("  ctrl-y\t\tYank the current line\n");
}

/// @brief Displays the configurable settings of the editor
static void printSettings() {
  printf("Settings\n");
  printf("  STATUS_MESSAGE_DURATION\tDuration in seconds for how long a status "
         "message will be displayed\n");
  printf("  TAB_STOP_SIZE\t\t\tSize of a tabstop converted to white space's\n");
}
