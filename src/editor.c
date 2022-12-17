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
 * @brief File containing the implementation of the editor functionality.
 */

#include "editor.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include "append_buffer.h"
#include "copy_buffer.h"
#include "project_config.h"
#include "syntax.h"

/// To quit with unsaved changes the quit command must be entered three times
#define QUIT_TIMES (3)

/// Control Key-Combination inputs (e.g. Ctrl-V)
#define CTRL_KEY(k) ((k)&0x1f)

/// Initializes append buffer
#define APPENDBUFFER_INIT \
    {             \
        NULL, 0   \
    }

/// Models a single line that is rendered by the editor
typedef struct
{
    /// Index of the editor row 
    uint32_t index;
    /// The amount of characters stored in the underlying character buffer
    uint32_t size;
    /// The amount of characters stored in the render buffer, that was created with the underlying character buffer
    uint32_t renderSize;
    /// The underlying character buffer
    char * chars;
    /// The render buffer, that was created with the underlying character buffer
    char * render;
    /// Pointer to the words that are highlighted
    unsigned char * highLight;
    /// Determines whether the row is part of a multiline comment
    bool hightLightOpenComment;
} editor_row_t;

/// Models the current state of the editor
typedef struct 
{
    /// X-coordinate of the curser in the underlying character buffer
    uint32_t cursorCurrentX;
    /// Y-coordinate of the curser in the underlying character buffer
    uint32_t cursorCurrentY;
    /// X-coordinate in the render buffer
    uint32_t renderX;
    /// Row offset - used for vertical scrolling
    uint32_t rowOffset;
    /// Column offset - used for horizontal scrolling
    uint32_t columnOffset;
    /// Amount of rows that are rendered on the screen
    uint32_t screenRows;
    /// Amount of columns that is rendered on the screen
    uint32_t screenColumns;
    /// Amount of rows for the oppened buffer
    uint32_t numberOfRows;
    /// Pointer to the rows of opened the buffer
    editor_row_t * editorRows;
    /// Used to track unsafed modifications
    bool unsavedChanges;
    /// The name of the file, that is currently opened
    char * fileName;
    /// The status message that is diplayed under status bar
    char statusMessage[240];
    /// Timestamp of the last message
    time_t statusMessageTimeStamp;
    /// Pointer to the syntax configurations of the editor
    editor_syntax_t * syntax;
    /// Used to store the original state of the terminal
    struct termios originalTermios;
    /// The copy buffer of the editor - used to store yanked characters
    copy_buffer_t copyBuffer;
    /// Amount of times the editor must be quit by the user if there are some unsaved changes
    uint8_t quitTimes;
    /// Configuration that was read by the configuration reader
    configuration_reader_result_t * config;
} editor_config_t;

/// Special control characters
enum editorKey
{
    /// The backspace key
    BACKSPACE = 127,
    /// Left arrow key
    ARROW_LEFT = 1000,
    /// Right arrow key
    ARROW_RIGHT,
    /// Up arrow key
    ARROW_UP,
    /// Down arrow key
    ARROW_DOWN,
    /// Delete key
    DEL_KEY,
    /// End key (jumps to the end of the line)
    END_KEY,
    /// Home key (jumps to the beginning of the line)
    HOME_KEY,
    /// Page up key - used for scrolling up
    PAGE_UP,
    /// Page up key - used for scrolling down
    PAGE_DOWN,
};

/// Used to store the state of the terminal when Yate was invoked, so we can revert it to it's original state
struct termios originalTermios;

/// Global state of the editor 
editor_config_t editorConfig;

static inline void editor_die(char const *);
static inline void editor_disable_raw_mode();
static void editor_delete_character();
static void editor_delete_row(uint32_t);
static void editor_draw_message_bar(append_buffer_t *);
static void editor_draw_rows(append_buffer_t *);
static void editor_draw_status_bar(append_buffer_t *);
static void editor_find();
static void editor_find_callback(char *, uint32_t);
static inline void editor_free_row(editor_row_t *);
static int32_t editor_get_cursor_position(uint32_t *, uint32_t *);
static int32_t editor_get_window_size(uint32_t *, uint32_t *);
static void editor_insert_character(uint32_t);
static void editor_insert_newline();
static void editor_insert_row(uint32_t, char *, size_t);
static inline bool editor_is_separator(uint32_t);
static void editor_move_cursor(uint32_t);
static void editor_open_file();
static void editor_open_file_callback(char *, uint32_t);
static inline void editor_paste_line();
static char * editor_prompt(char *, void (*)(char *, uint32_t));
static void editor_quit();
static uint32_t editor_read_key();
static void editor_render_welcome_screen_row(append_buffer_t *, uint32_t);
static void editor_row_append_string(editor_row_t *, char *, size_t);
static uint32_t editor_row_cx_to_rx(editor_row_t *, uint32_t);
static void editor_row_delete_character(editor_row_t *, uint32_t);
static void editor_row_insert_character(editor_row_t *, uint32_t, uint32_t);
static uint32_t editor_row_rx_to_cx(editor_row_t *, uint32_t);
static char * editor_rows_to_string(uint32_t *);
static void editor_save();
static void editor_scroll();
static void editor_select_syntax_highlight();
static void editor_set_status_message(char const *, ...);
static inline void editor_show_help();
static void editor_update_row(editor_row_t *);
static void editor_update_syntax(editor_row_t *);
static inline void editor_yank_line();

/// @brief Enables raw mode
/// @details By default the terminal starts in canonical mode. Iput is only sent when the user presses enter
void editor_enable_raw_mode()
{
    if (tcgetattr(STDIN_FILENO, &editorConfig.originalTermios) == -1)
        editor_die("tcgetattr");
    // Disables raw mode when we exit the program
    atexit(editor_disable_raw_mode);
    struct termios raw = editorConfig.originalTermios;
    /*
     * IXON - Disables Ctrl-S and Ctrl-Q
     * ECHO - Disable echoing directly to the terminal
     * ICANON - Turn of canonical mode (read input byte-by-byte instead of line-by-line)
     * SIGINT - Turn of Ctrl-C and Ctrl-Z signals
     * IEXTEN - Disable Ctrl-V
     * ICRNL - Interpret Ctrl-M as carriage return
     * OPOST - Turn of all processing
     * BRKINT - Causes break conitions to send a SIGINT signal
     * INPCK - Enables parity checking
     * ISTRIP - Causes the 8th bit of each input byte to be stripped
     * CS8 - Sets the character size to 8 bits per byte (already like that on most systems)
     */
    raw.c_iflag &= ~(OPOST);
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // Sets amount of bytes of input before read can return
    raw.c_cc[VMIN] = 0;
    // Sets time before read time out to 100 milliseconds
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        editor_die("tcsetattr");
}

void editor_initialize(configuration_reader_result_t * config)
{
    editorConfig.cursorCurrentX = editorConfig.renderX = editorConfig.cursorCurrentY = editorConfig.rowOffset = editorConfig.columnOffset =
    editorConfig.numberOfRows = editorConfig.statusMessageTimeStamp = 0;
    editorConfig.unsavedChanges = false;
    editorConfig.quitTimes = QUIT_TIMES;
    editorConfig.editorRows = NULL;
    editorConfig.fileName = NULL;
    editorConfig.statusMessage[0] = '\0';
    editorConfig.syntax = NULL;
    if (editor_get_window_size(&editorConfig.screenRows, &editorConfig.screenColumns) == -1)
        editor_die("editor_get_window_size");   // Could not determine window size

    // Make room for status bar and message bar
    editorConfig.screenRows -= 2;
    editorConfig.config = config;
    copy_buffer_init(&editorConfig.copyBuffer);
}

void editor_open(char const * filePath)
{   
    FILE * filePointer = fopen(filePath, "r");
    if (!filePointer) 
    {
        editor_set_status_message("File under the path %s not found", filePath);
        return;
    }
    char * line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &linecap, filePointer)) != -1)
    {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            linelen--;
        editor_insert_row(editorConfig.numberOfRows, line, linelen);
    }
    free(line);
    fclose(filePointer);

    free(editorConfig.fileName);
    editorConfig.fileName = strdup(filePath);
    // Selects syntax highlighting configuration based on file extension
    editor_select_syntax_highlight();

    editorConfig.unsavedChanges = false;
    editorConfig.cursorCurrentX  = 0;
}

void editor_process_keypress()
{

    uint32_t c = editor_read_key();
    switch (c)
    {
    case '\r':
        editor_insert_newline();
        break;
    // Ctrl-q quits the editor
    case CTRL_KEY('q'):
        editor_quit();
        break;

    // Save file
    case CTRL_KEY('s'):
        editor_save();
        break;

    case HOME_KEY:
        editorConfig.cursorCurrentX = 0;
        break;
    case END_KEY:
        if (editorConfig.cursorCurrentY < editorConfig.numberOfRows)
            editorConfig.cursorCurrentX = editorConfig.editorRows[editorConfig.cursorCurrentY].size;
        break;

    case CTRL_KEY('f'):
        editor_find();
        break;

    case CTRL_KEY('h'):
        editor_show_help();
        break;
    case CTRL_KEY('d'):
        editor_yank_line();
        editor_delete_row(editorConfig.cursorCurrentY);
        break;
    
    // Yank
    case CTRL_KEY('y'):
        editor_yank_line();
        break;
    // Paste
    case CTRL_KEY('p'):
        editor_paste_line();
        break;

    case CTRL_KEY('o'):
        editor_open_file();
        break;    

    case BACKSPACE:
    case DEL_KEY:
        if (c == DEL_KEY)
            editor_move_cursor(ARROW_RIGHT);
        editor_delete_character();
        break;

    case PAGE_UP:
    case PAGE_DOWN:
    {
        if (c == PAGE_UP)
        {
            editorConfig.cursorCurrentY = editorConfig.rowOffset;
        }
        else if (c == PAGE_DOWN)
        {
            editorConfig.cursorCurrentY = editorConfig.rowOffset + editorConfig.screenRows - 1;
            if (editorConfig.cursorCurrentY > editorConfig.numberOfRows)
                editorConfig.cursorCurrentY = editorConfig.numberOfRows;
        }

        int times = editorConfig.screenRows;
        while (times--)
            editor_move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
    }
    break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        editor_move_cursor(c);
        break;

    // Escape
    case CTRL_KEY('l'):
    case '\x1b':
        break;

    default:
        editor_insert_character(c);
        break;
    }
    if(c != CTRL_KEY('q'))
        editorConfig.quitTimes = QUIT_TIMES;
}

void editor_refresh_screen()
{
    editor_scroll();

    append_buffer_t buffer = APPENDBUFFER_INIT;
    append_buffer_append_string(&buffer, "\x1b[?25l", 6);
    append_buffer_append_string(&buffer, "\x1b[H", 3);
    editor_draw_rows(&buffer);
    editor_draw_status_bar(&buffer);
    editor_draw_message_bar(&buffer);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (editorConfig.cursorCurrentY - editorConfig.rowOffset) + 1,
             (editorConfig.renderX - editorConfig.columnOffset) + 1);
    append_buffer_append_string(&buffer, buf, strlen(buf));

    append_buffer_append_string(&buffer, "\x1b[?25h", 6);
    write(STDOUT_FILENO, buffer.buffer, buffer.length);
    append_buffer_free(&buffer);
}

/// @brief Emits an error message and quits the editor
/// @param message The message that is ommitted
static inline void editor_die(char const * message)
{
    perror(message);
    if(editorConfig.config)
        free(editorConfig.config);
    exit(1);
}

/// @brief Disables raw input mode
static inline void editor_disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editorConfig.originalTermios) == -1)
        editor_die("tcsetattr");
}

/// @brief Deletes the character at the current curser poisition 
static void editor_delete_character()
{
    if (editorConfig.cursorCurrentY == editorConfig.numberOfRows)
        return;
    if (editorConfig.cursorCurrentX == 0 && editorConfig.cursorCurrentY == 0)
        return;
    editor_row_t * row = &editorConfig.editorRows[editorConfig.cursorCurrentY];
    if (editorConfig.cursorCurrentX > 0)
    {
        editor_row_delete_character(row, editorConfig.cursorCurrentX - 1);
        editorConfig.cursorCurrentX--;
    }
    else
    {
        editorConfig.cursorCurrentX = editorConfig.editorRows[editorConfig.cursorCurrentY - 1].size;
        editor_row_append_string(&editorConfig.editorRows[editorConfig.cursorCurrentY - 1], row->chars, row->size);
        editor_delete_row(editorConfig.cursorCurrentY);
        editorConfig.cursorCurrentY--;
    }
}

/// @brief Deletes a complete row in the editor 
/// @param at The index of the row that is deleted
static void editor_delete_row(uint32_t at)
{
    if (at < 0 || at >= editorConfig.numberOfRows)
        return;
    editor_free_row(&editorConfig.editorRows[at]);
    memmove(&editorConfig.editorRows[at], &editorConfig.editorRows[at + 1], sizeof(editor_row_t) * (editorConfig.numberOfRows - at - 1));
    for (int j = at; j < editorConfig.numberOfRows - 1; j++)
        editorConfig.editorRows[j].index--;
    editorConfig.numberOfRows--;
    editorConfig.unsavedChanges = true;
}

/// @brief Draws the message bar of the editor
/// @param buffer Pointer to the appendbuffer that stores the editor page
static void editor_draw_message_bar(append_buffer_t * buffer)
{
    append_buffer_append_string(buffer, "\x1b[K", 3);
    size_t msglen = strlen(editorConfig.statusMessage);
    if (msglen > editorConfig.screenColumns)
        msglen = editorConfig.screenColumns;
    if (msglen && time(NULL) - editorConfig.statusMessageTimeStamp < editorConfig.config->messageDisplayDuration)
        append_buffer_append_string(buffer, editorConfig.statusMessage, msglen);
}

/// @brief Appends a single row of the welcome screen to a given append buffer
/// @param buffer Pointer to the append buffer where the welcome screen row is appended
/// @param rowIndex The index of the row in the welcomePage
static void editor_render_welcome_screen_row(append_buffer_t * buffer, uint32_t rowIndex)
{
    char welcomeMessageRow[80];    
    size_t welcomeMessageRowLength;
    switch (rowIndex)
    {
    case 0:
        welcomeMessageRowLength = snprintf(welcomeMessageRow, sizeof(welcomeMessageRow), "%s - Yet another text editor", PROJECT_NAME);
        break;
    case 1:
        return;
    case 2:
        welcomeMessageRowLength = snprintf(welcomeMessageRow, sizeof(welcomeMessageRow), "version %d.%d", PROJECT_VERSION_MAJOR, PROJECT_VERSION_MINOR);
        break;
    case 3:
        welcomeMessageRowLength = snprintf(welcomeMessageRow, sizeof(welcomeMessageRow), "by %s", PROJECT_VENDOR);
        break;
    case 4:
        welcomeMessageRowLength = snprintf(welcomeMessageRow, sizeof(welcomeMessageRow), "%s is open source and freely distributable", PROJECT_NAME);
        break;
    case 5:
        return;
    case 6:
        welcomeMessageRowLength = snprintf(welcomeMessageRow, sizeof(welcomeMessageRow), "Press \x1b[38;2;255;230;102mCtrl-q\x1b[39;49m to exit");
        break;
    case 7:
        welcomeMessageRowLength = snprintf(welcomeMessageRow, sizeof(welcomeMessageRow), "Press \x1b[38;2;255;230;102mCtrl-h\x1b[39;49m to show the help");
        break;

    default:
    editor_die("editor_render_welcome_screen_row");
        break;
    }
    if (welcomeMessageRowLength > editorConfig.screenColumns)
        welcomeMessageRowLength = editorConfig.screenColumns;
    int padding = (editorConfig.screenColumns - welcomeMessageRowLength) / 2;
    if (rowIndex > 5)
        padding += 13;
    if (padding)
    {
        append_buffer_append_string(buffer, "~", 1);
        padding--;
    }
    while (padding--)
        append_buffer_append_string(buffer, " ", 1);
    append_buffer_append_string(buffer, welcomeMessageRow, welcomeMessageRowLength);
}

/// @brief Draws all the rows that are visible in the terminal
/// @param buffer The buffer where all the rows that are currently visible are appended
static void editor_draw_rows(append_buffer_t * buffer)
{
    uint32_t y;
    for (y = 0; y < editorConfig.screenRows; y++)
    {
        uint32_t filerow = y + editorConfig.rowOffset;
        if (filerow >= editorConfig.numberOfRows)
        {
            if (editorConfig.numberOfRows == 0 && y >= editorConfig.screenRows / 3 && y <= editorConfig.screenRows / 3 + 7)
                editor_render_welcome_screen_row(buffer, y - editorConfig.screenRows / 3);
            else
                append_buffer_append_string(buffer, "~", 1);
        }
        else
        {
            uint32_t length = editorConfig.editorRows[filerow].renderSize - editorConfig.columnOffset;
            if (length < 0)
                length = 0;
            if (length > editorConfig.screenColumns)
                length = editorConfig.screenColumns;
            char * c = &editorConfig.editorRows[filerow].render[editorConfig.columnOffset];
            unsigned char * highLight = &editorConfig.editorRows[filerow].highLight[editorConfig.columnOffset];
            int32_t current_color = -1;
            uint32_t j;
            char str[16];
            for (j = 0; j < length; j++)
            {
                if (iscntrl(c[j]))
                {
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    append_buffer_append_string(buffer, "\x1b[7m", 4);
                    append_buffer_append_string(buffer, &sym, 1);
                    append_buffer_append_string(buffer, "\x1b[m", 3);
                    if (current_color != -1)
                    {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        append_buffer_append_string(buffer, buf, clen);
                    }
                }
                else if (highLight[j] == HIGHTLIGHT_NORMAL)
                {
                    if (current_color != -1)
                    {
                        // Reset back- and foreground color to default
                        append_buffer_append_string(buffer, "\x1b[39;49m", 8);
                        current_color = -1;
                    }
                    append_buffer_append_string(buffer, &c[j], 1);
                }
                else
                {
                    int32_t color = syntax_convert_to_color(highLight[j]);
                    if (color != current_color)
                    {
                        current_color = color;
                        char buf[36];
                        int clen = snprintf(buf, sizeof(buf), 
                        (color & 0xff000000) ? "\x1b[48;2;%d;%d;%dm" : "\x1b[38;2;%d;%d;%dm",
                        (color & 0x00ff0000) >> 16,
                        (color & 0x0000ff00) >> 8,
                        color & 0x000000ff);
                        append_buffer_append_string(buffer, buf, clen);
                    }
                    append_buffer_append_string(buffer, &c[j], 1);
                }
            }
            // Reset back- and foreground color to default
            append_buffer_append_string(buffer, "\x1b[39;49m", 8);
        }
        append_buffer_append_string(buffer, "\x1b[K", 3);
        append_buffer_append_string(buffer, "\r\n", 2);
    }
}

/// @brief Draws the status bar of the editor
/// @param buffer The append buffer of the editor, where the status bar is appended
static void editor_draw_status_bar(append_buffer_t * buffer)
{
    append_buffer_append_string(buffer, "\x1b[7m", 4);
    char statusBarLeftMessage[4200], StatusBarRightMessage[80], realPath[4096];
    if(editorConfig.fileName)
        realpath(editorConfig.fileName, realPath);
    int leftMessageLength = snprintf(statusBarLeftMessage, sizeof(statusBarLeftMessage), "%.80s - %d lines %s",
                       editorConfig.fileName ? realPath : "[No file name]", editorConfig.numberOfRows,
                       editorConfig.unsavedChanges ? "(modified)" : "");
    int rightMessageLength = snprintf(StatusBarRightMessage, sizeof(StatusBarRightMessage), "%s | %d/%d",
                        editorConfig.syntax ? editorConfig.syntax->filetype : "", editorConfig.cursorCurrentY + 1, editorConfig.numberOfRows);
    if (leftMessageLength > editorConfig.screenColumns)
        leftMessageLength = editorConfig.screenColumns;
    append_buffer_append_string(buffer, statusBarLeftMessage, leftMessageLength);
    while (leftMessageLength < editorConfig.screenColumns)
    {
        if (editorConfig.screenColumns - leftMessageLength == rightMessageLength)
        {
            append_buffer_append_string(buffer, StatusBarRightMessage, rightMessageLength);
            break;
        }
        else
        {
            append_buffer_append_string(buffer, " ", 1);
            leftMessageLength++;
        }
    }
    append_buffer_append_string(buffer, "\x1b[m", 3);
    append_buffer_append_string(buffer, "\r\n", 2);
}

/// @brief Finds all occurences of a word in the currently oppend source file
static void editor_find()
{
    uint32_t savedCurrentX = editorConfig.cursorCurrentX;
    uint32_t savedCurrentY = editorConfig.cursorCurrentY;
    uint32_t savedColumnOffset = editorConfig.columnOffset;
    uint32_t savedRowOffset = editorConfig.rowOffset;
    char * query = editor_prompt("Search: %s (Use ESC/Arrows/Enter)", editor_find_callback);
    if (query)
    {
        free(query);
    }
    else
    {
        editorConfig.cursorCurrentX = savedCurrentX;
        editorConfig.cursorCurrentY = savedCurrentY;
        editorConfig.columnOffset = savedColumnOffset;
        editorConfig.rowOffset = savedRowOffset;
    }
}

/// @brief Callback of the find function
/// @param query The word that is searched for
/// @param key The key that was pressed previously
static void editor_find_callback(char * query, uint32_t key)
{
    static int32_t lastMatch = -1;
    static int32_t direction = 1;
    static int32_t savedHighLightedLine;
    static char * savedHighLight = NULL;
    if (savedHighLight)
    {
        memcpy(editorConfig.editorRows[savedHighLightedLine].highLight, savedHighLight, editorConfig.editorRows[savedHighLightedLine].renderSize);
        free(savedHighLight);
        savedHighLight = NULL;
    }

    if (key == '\r' || key == '\x1b')
    {
        lastMatch = -1;
        direction = 1;
        return;
    }
    else if (key == ARROW_RIGHT || key == ARROW_DOWN)
    {
        direction = 1;
    }
    else if (key == ARROW_LEFT || key == ARROW_UP)
    {
        direction = -1;
    }
    else
    {
        lastMatch = -1;
        direction = 1;
    }
    if (lastMatch == -1)
        direction = 1;
    int32_t current = lastMatch;
    size_t i;
    for (i = 0; i < editorConfig.numberOfRows; i++)
    {
        current += direction;
        if (current == -1)
            current = editorConfig.numberOfRows - 1;
        else if (current == editorConfig.numberOfRows)
            current = 0;
        editor_row_t * row = &editorConfig.editorRows[current];
        char * match = strstr(row->render, query);
        if (match)
        {
            lastMatch = current;
            editorConfig.cursorCurrentY = current;
            editorConfig.cursorCurrentX = editor_row_rx_to_cx(row, match - row->render) + 1;
            editorConfig.rowOffset = editorConfig.numberOfRows;
            savedHighLightedLine = current;
            savedHighLight = malloc(row->renderSize);
            memcpy(savedHighLight, row->highLight, row->renderSize);
            memset(&row->highLight[match - row->render], HIGHLIGHT_MATCH, strlen(query));
            break;
        }
    }
}

/// @brief Frees the contents of a single row
/// @param row  The row where the contents are freed
static inline void editor_free_row(editor_row_t * row)
{
    free(row->render);
    free(row->chars);
    free(row->highLight);
}

/// @brief Determines the position of the curser
/// @param rows Pointer to store vertical position of the editor
/// @param columns Pointer to store horizontal position of the editor
/// @return 0 if the position was successfully determined, -1 if not
static int32_t editor_get_cursor_position(uint32_t * rows, uint32_t * columns)
{
    char buf[32];
    uint32_t i = 0;
    // Uses the special escape sequence 'cursor position report' to determine position
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
        return -1;
    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", rows, columns) != 2)
        return -1;
    return 0;
}

/// @brief Determines the size of the display window in the terminal 
/// @param rows Pointer to store the number of rows
/// @param columns Pointer to store the number of columns
/// @return -1 if an error occured, 0 if not
static int32_t editor_get_window_size(uint32_t * rows, uint32_t * columns)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        // Write cursor forward and cursor down command
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return editor_get_cursor_position(rows, columns);
    }
    else
    {
        *columns = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/// @brief Inserts a character at the current position
/// @param c The character that is inserted
static void editor_insert_character(uint32_t c)
{
    if (editorConfig.cursorCurrentY == editorConfig.numberOfRows)
    {
        editor_insert_row(editorConfig.numberOfRows, "", 0);
    }
    editor_row_insert_character(&editorConfig.editorRows[editorConfig.cursorCurrentY], editorConfig.cursorCurrentX, c);
    editorConfig.cursorCurrentX++;
}

/// @brief Inserts a new line a the current cursor position
static void editor_insert_newline()
{
    if (editorConfig.cursorCurrentX == 0)
    {
        editor_insert_row(editorConfig.cursorCurrentY, "", 0);
    }
    else
    {
        editor_row_t * row = &editorConfig.editorRows[editorConfig.cursorCurrentY];
        editor_insert_row(editorConfig.cursorCurrentY + 1, &row->chars[editorConfig.cursorCurrentX], row->size - editorConfig.cursorCurrentX);
        row = &editorConfig.editorRows[editorConfig.cursorCurrentY];
        row->size = editorConfig.cursorCurrentX;
        row->chars[row->size] = '\0';
        editor_update_row(row);
    }
    editorConfig.cursorCurrentY++;
    editorConfig.cursorCurrentX = 0;
}

/// @brief Inserts a new row into the character buffer
/// @param at Offset to the point where the linebreak is added
/// @param str The character buffer where the new row is added
/// @param length The length of the new row
static void editor_insert_row(uint32_t at, char * str, size_t length)
{
    if (at < 0 || at > editorConfig.numberOfRows)
        return;
    editorConfig.editorRows = realloc(editorConfig.editorRows, sizeof(editor_row_t) * (editorConfig.numberOfRows + 1));
    memmove(&editorConfig.editorRows[at + 1], &editorConfig.editorRows[at], sizeof(editor_row_t) * (editorConfig.numberOfRows - at));

    for (int32_t j = at + 1; j <= editorConfig.numberOfRows; j++)
        editorConfig.editorRows[j].index++;

    editorConfig.editorRows[at].index = at;

    editorConfig.editorRows[at].size = length;
    editorConfig.editorRows[at].chars = malloc(length + 1);
    memcpy(editorConfig.editorRows[at].chars, str, length);
    editorConfig.editorRows[at].chars[length] = '\0';

    editorConfig.editorRows[at].renderSize = 0;
    editorConfig.editorRows[at].render = NULL;
    editorConfig.editorRows[at].highLight = NULL;

    editorConfig.editorRows[at].hightLightOpenComment = false;

    editor_update_row(&editorConfig.editorRows[at]);

    editorConfig.numberOfRows++;
    editorConfig.unsavedChanges = true;
}

/// @brief Determines whether a character is a seperator
/// @param c The character that is evaluated
/// @return true if the character is a seperator, false if not
/// @details A seperator can be for example: a file, group, record, unit or information seperator
static inline bool editor_is_separator(uint32_t c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

/// @brief Moves the cursor based on the input
/// @param key The key that was pressed
static void editor_move_cursor(uint32_t key)
{
    editor_row_t *row = (editorConfig.cursorCurrentY >= editorConfig.numberOfRows) ? NULL : &editorConfig.editorRows[editorConfig.cursorCurrentY];
    switch (key)
    {
    case ARROW_LEFT:
        if (editorConfig.cursorCurrentX != 0)
        {
            editorConfig.cursorCurrentX--;
        }
        else if (editorConfig.cursorCurrentY > 0)
        {
            // Jump to the end of the previous line
            editorConfig.cursorCurrentY--;
            editorConfig.cursorCurrentX = editorConfig.editorRows[editorConfig.cursorCurrentY].size;
        }
        break;
    case ARROW_RIGHT:
        if (row && editorConfig.cursorCurrentX < row->size)
        {
            editorConfig.cursorCurrentX++;
        }
        else if (row && editorConfig.cursorCurrentX == row->size)
        {
            // Jump to the beginning of the next line
            editorConfig.cursorCurrentY++;
            // The next line beginns after the line numbers
            editorConfig.cursorCurrentX = 0;
        }
        break;
    case ARROW_UP:
        if (editorConfig.cursorCurrentY != 0)
        {
            editorConfig.cursorCurrentY--;
        }
        break;
    case ARROW_DOWN:
        if (editorConfig.cursorCurrentY < editorConfig.numberOfRows)
        {
            editorConfig.cursorCurrentY++;
        }
        break;
    }
    row = (editorConfig.cursorCurrentY >= editorConfig.numberOfRows) ? NULL : &editorConfig.editorRows[editorConfig.cursorCurrentY];
    int rowlen = row ? row->size : 0;
    if (editorConfig.cursorCurrentX > rowlen)
    {
        editorConfig.cursorCurrentX = rowlen;
    }
}

/// @brief Opens another file in the editor
static void editor_open_file()
{
    uint32_t savedCurrentX = editorConfig.cursorCurrentX;
    uint32_t savedCurrentY = editorConfig.cursorCurrentY;
    uint32_t savedColumnOffset = editorConfig.columnOffset;
    uint32_t savedRowOffset = editorConfig.rowOffset;
    char * query = editor_prompt("Open: %s (Use ESC/Enter)", editor_open_file_callback);
    if (query)
    {
        free(query);
    }
    else
    {
        editorConfig.cursorCurrentX = savedCurrentX;
        editorConfig.cursorCurrentY = savedCurrentY;
        editorConfig.columnOffset = savedColumnOffset;
        editorConfig.rowOffset = savedRowOffset;
    }
}

/// @brief Callback of the open file fuction
/// @param query The path of the file that is opened
/// @param key The last key that was pressed
static void editor_open_file_callback(char * query, uint32_t key)
{    
    if(key != '\r')
        return;    
    if(editorConfig.unsavedChanges)
    {
        editor_set_status_message("Can not open a new file, while currently opened file has some unsaved changes");
        return;
    }
    editor_initialize(editorConfig.config);
    editor_open(query);
}

/// @brief Opens the editor prompt
/// @param prompt The message that is displayed by the prompt
/// @param callback The callback of the prompt
/// @return NULL if the prompt was cancelled, otherwise a pointer to the input
static char * editor_prompt(char * prompt, void (* callback)(char *, uint32_t))
{
    size_t bufsize = 128;
    char * buf = malloc(bufsize);
    size_t buflen = 0;
    buf[0] = '\0';
    while (1)
    {
        editor_set_status_message(prompt, buf);
        editor_refresh_screen();
        uint32_t c = editor_read_key();
        if (c == DEL_KEY || c == BACKSPACE)
        {
            if (buflen != 0)
                buf[--buflen] = '\0';
        }
        // Escape
        else if (c == '\x1b')
        {
            editor_set_status_message("");
            if (callback)
                callback(buf, c);
            free(buf);
            return NULL;
        }
        // Enter
        else if (c == '\r')
        {
            if (buflen != 0)
            {
                if (callback)
                    callback(buf, c);
                return buf;
            }
        }
        // Text input for the prompt
        else if (!iscntrl(c) && c < 128)
        {
            if (buflen == bufsize - 1)
            {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
        if (callback)
            callback(buf, c);
    }
}

/// @brief Pastes the content that is stroed in the copy buffer at the current position of the curser
static inline void editor_paste_line()
{
    if(editorConfig.copyBuffer.buffer && editorConfig.copyBuffer.length)
        editor_insert_row(editorConfig.cursorCurrentY, editorConfig.copyBuffer.buffer, editorConfig.copyBuffer.length);
}

/// @brief Quits the editor
static void editor_quit()
{

    if (editorConfig.unsavedChanges && editorConfig.quitTimes > 0)
    {
        editor_set_status_message("WARNING!!! File has unsaved changes. "
                               "Press Ctrl-Q %d more times to quit.",
                               editorConfig.quitTimes);
        editorConfig.quitTimes--;
        return;
    }
    copy_buffer_free(&editorConfig.copyBuffer);
    // Clears the screen when the editor is quit
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    if(editorConfig.config)
        free(editorConfig.config);
    exit(0);
}

/// @brief Reads a single character from the keyboard
/// @return The character that was read
static uint32_t editor_read_key()
{
    uint32_t nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if (nread == -1 && errno != EAGAIN)
            editor_die("read");
    }
    if (c == '\x1b')
    {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1)
            return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1)
            return '\x1b';
        if (seq[0] == '[')
        {
            if (seq[1] >= '0' && seq[1] <= '9')
            {
                if (read(STDIN_FILENO, &seq[2], 1) != 1)
                    return '\x1b';
                if (seq[2] == '~')
                {
                    switch (seq[1])
                    {
                    case '1':
                        return HOME_KEY;
                    case '3':
                        return DEL_KEY;
                    case '4':
                        return END_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    case '7':
                        return HOME_KEY;
                    case '8':
                        return END_KEY;
                    }
                }
            }
            else
            {
                switch (seq[1])
                {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'F':
                    return END_KEY;
                case 'H':
                    return HOME_KEY;
                }
            }
        }
        else if (seq[0] == 'O')
        {
            switch (seq[1])
            {
            case 'F':
                return END_KEY;
            case 'H':
                return HOME_KEY;
            }
        }
        return '\x1b';
    }
    else
    {
        return c;
    }
}

/// @brief Appends a character sequence to an editor row
/// @param row The row where the character sequence is appended
/// @param str The character sequence that is appended
/// @param length The length of the character sequence 
static void editor_row_append_string(editor_row_t * row, char * str, size_t length)
{
    row->chars = realloc(row->chars, row->size + length + 1);
    memcpy(&row->chars[row->size], str, length);
    row->size += length;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    editorConfig.unsavedChanges = true;
}

/// @brief Translates the a horizontal coordinate from the underlying character pointer to the renderer coordinate
/// @param row The editor row of the coordinate systems of booth coordinates
/// @param cursorCurrentX The current horizontal coordinate from the underlying character pointer
/// @return The coordinate tranlated to a render coordinate
static uint32_t editor_row_cx_to_rx(editor_row_t * row, uint32_t cursorCurrentX)
{
    uint32_t renderX = 0;
    uint32_t j;
    for (j = 0; j < cursorCurrentX; j++)
    {
        if (row->chars[j] == '\t')
            renderX += (editorConfig.config->tabStopSize - 1) - (renderX % editorConfig.config->tabStopSize);
        renderX++;
    }
    return renderX;
}

/// @brief Deletes a single character from an editor row
/// @param row The row where the character is deleted
/// @param at The index of the character in the underlying character buffer of the editor row
static void editor_row_delete_character(editor_row_t * row, uint32_t at)
{
    if (at < 0 || at >= row->size)
        return;
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    editor_update_row(row);
    editorConfig.unsavedChanges = true;
}

/// @brief Inserts a single character in an editor row
/// @param row The row where the character is inserted
/// @param at The index where the character is inserted
/// @param c The character that is inserted
static void editor_row_insert_character(editor_row_t * row, uint32_t at, uint32_t c)
{
    if (at < 0 || at > row->size)
        at = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    editor_update_row(row);
    editorConfig.unsavedChanges = true;
}

/// @brief Translates a render coordinate into a coordinate of the underlying character buffer
/// @param row The row where the coordinates are translated
/// @param renderX The coordinate in the renderer
/// @return The coordinate in the underlying character buffer
static uint32_t editor_row_rx_to_cx(editor_row_t * row, uint32_t renderX)
{
    uint32_t cur_rx = 0;
    uint32_t cursorCurrentX;
    for (cursorCurrentX = 0; cursorCurrentX < row->size; cursorCurrentX++)
    {
        if (row->chars[cursorCurrentX] == '\t')
            cur_rx += (editorConfig.config->tabStopSize - 1) - (cur_rx % editorConfig.config->tabStopSize);
        cur_rx++;
        if (cur_rx > renderX)
            return cursorCurrentX;
    }
    return cursorCurrentX;
}

/// @brief Coverts the rows of the editor to a character sequence and returns a pointer to the begining of that sequence
/// @param bufferLength Pointer to the length of the underlying buffer
/// @return The created character sequence
static char * editor_rows_to_string(uint32_t * bufferLength)
{
    size_t totlen = 0;
    size_t j;
    for (j = 0; j < editorConfig.numberOfRows; j++)
        totlen += editorConfig.editorRows[j].size + 1;
    *bufferLength = totlen;
    char * buffer = malloc(totlen);
    char * bufferPointer = buffer;
    for (j = 0; j < editorConfig.numberOfRows; j++)
    {
        memcpy(bufferPointer, editorConfig.editorRows[j].chars , editorConfig.editorRows[j].size);
        bufferPointer += editorConfig.editorRows[j].size;
        *bufferPointer = '\n';
        bufferPointer++;
    }
    return buffer;
}

/// @brief Saves the file that is currently opened
static void editor_save()
{
    if (editorConfig.fileName == NULL)
    {
        editorConfig.fileName = editor_prompt("Save as: %s (ESC to cancel)", NULL);
        if (editorConfig.fileName == NULL)
        {
            editor_set_status_message("Save aborted");
            return;
        }
        editor_select_syntax_highlight();
    }
    uint32_t length;
    char * buffer = editor_rows_to_string(&length);
    int fileDescriptor = open(editorConfig.fileName, O_RDWR | O_CREAT, 0644);
    if (fileDescriptor != -1)
    {
        if (ftruncate(fileDescriptor, length) != -1)
        {
            if (write(fileDescriptor, buffer, length) == length)
            {
                close(fileDescriptor);
                free(buffer);
                editorConfig.unsavedChanges = false;
                editor_set_status_message("%d bytes written to disk", length);
                return;
            }
        }
        close(fileDescriptor);
    }
    free(buffer);
    editor_set_status_message("Can't save! I/O error: %s", strerror(errno));
}

/// @brief Scrolls throught the opened file
static void editor_scroll()
{
    editorConfig.renderX = 0;
    if (editorConfig.cursorCurrentY < editorConfig.numberOfRows)
    {
        editorConfig.renderX = editor_row_cx_to_rx(&editorConfig.editorRows[editorConfig.cursorCurrentY], editorConfig.cursorCurrentX);
    }
    if (editorConfig.cursorCurrentY < editorConfig.rowOffset)
    {
        editorConfig.rowOffset = editorConfig.cursorCurrentY;
    }
    if (editorConfig.cursorCurrentY >= editorConfig.rowOffset + editorConfig.screenRows)
    {
        editorConfig.rowOffset = editorConfig.cursorCurrentY - editorConfig.screenRows + 1;
    }
    if (editorConfig.renderX < editorConfig.columnOffset)
    {
        editorConfig.columnOffset = editorConfig.renderX;
    }
    if (editorConfig.renderX >= editorConfig.columnOffset + editorConfig.screenColumns)
    {
        editorConfig.columnOffset = editorConfig.renderX - editorConfig.screenColumns + 1;
    }
}

/// @brief Changes the currently selected syntax highlighting configuration
static void editor_select_syntax_highlight()
{
    editorConfig.syntax = NULL;
    if (editorConfig.fileName == NULL)
        return;
    char * fileExtension = strrchr(editorConfig.fileName, '.');
    size_t languageCount = syntax_get_language_count();
    for (size_t j = 0; j < languageCount; j++)
    {
        editor_syntax_t *s = &HighLightDataBase[j];
        unsigned int i = 0;
        while (s->filematch[i])
        {
            bool isFileExtension = (s->filematch[i][0] == '.');
            if ((isFileExtension && fileExtension && !strcmp(fileExtension, s->filematch[i])) || (!isFileExtension && strstr(editorConfig.fileName, s->filematch[i])))
            {
                editorConfig.syntax = s;
                for (uint32_t filerow = 0; filerow < editorConfig.numberOfRows; filerow++)
                    editor_update_syntax(&editorConfig.editorRows[filerow]);
                return;
            }
            i++;
        }
    }
}

/// @brief Sets the message that is displayed in the status bar
/// @param format The format of the message that is displayed
/// @param  args The arguments that are used to create the message
static void editor_set_status_message(char const * format, ...)
{
    va_list argumentPointer;
    va_start(argumentPointer, format);
    vsnprintf(editorConfig.statusMessage, sizeof(editorConfig.statusMessage), format, argumentPointer);
    va_end(argumentPointer);
    editorConfig.statusMessageTimeStamp = time(NULL);
}

/// Shows the hotkeys of the editor as a status message
static inline void editor_show_help()
{
    editor_set_status_message("HELP: Ctrl-D = delete | Ctrl-F = find | Ctrl-H = help | Ctrl-O = open | Ctrl-P = paste | Ctrl-Q = quit | Ctrl-S = save | Ctrl-Y = yank");
}

/// @brief Updates the contents that are diplayed by a single editor row
/// @param row The row that is updated
static void editor_update_row(editor_row_t * row)
{
    size_t tabs = 0;
    size_t j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t')
            tabs++;
    free(row->render);
    row->render = malloc(row->size + tabs * (editorConfig.config->tabStopSize - 1) + 1);
    size_t idx = 0;
    for (j = 0; j < row->size; j++)
    {
        if (row->chars[j] == '\t')
        {
            row->render[idx++] = ' ';
            while (idx % editorConfig.config->tabStopSize != 0)
                row->render[idx++] = ' ';
        }
        else
        {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->renderSize = idx;

    editor_update_syntax(row);
}

/// @brief Updates the syntax highlighting for a given editor row
/// @param row The editor row where the syntax highlighting is applied
static void editor_update_syntax(editor_row_t * row)
{
    row->highLight = realloc(row->highLight, row->renderSize);
    memset(row->highLight, HIGHTLIGHT_NORMAL, row->renderSize);

    if (editorConfig.syntax == NULL)
        return;

    char ** keywords = editorConfig.syntax->keywords;
    char * singleLineCommentStart = editorConfig.syntax->singleline_comment_start;
    char * multiLineCommentStart = editorConfig.syntax->multiline_comment_start;
    char * multiLineCommentEnd = editorConfig.syntax->multiline_comment_end;
    size_t singleLineCommentStartLength = singleLineCommentStart ? strlen(singleLineCommentStart) : 0;
    size_t multiLineCommentStartLength = multiLineCommentStart ? strlen(multiLineCommentStart) : 0;
    size_t multiLineCommentEndLength = multiLineCommentEnd ? strlen(multiLineCommentEnd) : 0;
    bool previousSeperator = true;
    bool insideString = false;
    bool insideComment = (row->index > 0 && editorConfig.editorRows[row->index - 1].hightLightOpenComment);
    size_t i = 0;
    while (i < row->renderSize)
    {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->highLight[i - 1] : HIGHTLIGHT_NORMAL;

        if (singleLineCommentStartLength && !insideString && !insideComment)
        {
            if (!strncmp(&row->render[i], singleLineCommentStart, singleLineCommentStartLength))
            {
                memset(&row->highLight[i], HIGHLIGHT_COMMENT, row->renderSize - i);
                break;
            }
        }
        if (multiLineCommentStartLength && multiLineCommentEndLength && !insideString)
        {
            if (insideComment)
            {
                row->highLight[i] = HIGHLIGHT_MLCOMMENT;
                if (!strncmp(&row->render[i], multiLineCommentEnd, multiLineCommentEndLength))
                {
                    memset(&row->highLight[i], HIGHLIGHT_MLCOMMENT, multiLineCommentEndLength);
                    i += multiLineCommentEndLength;
                    insideComment = 0;
                    previousSeperator = true;
                    continue;
                }
                else
                {
                    i++;
                    continue;
                }
            }
            else if (!strncmp(&row->render[i], multiLineCommentStart, multiLineCommentStartLength))
            {
                memset(&row->highLight[i], HIGHLIGHT_MLCOMMENT, multiLineCommentStartLength);
                i += multiLineCommentStartLength;
                insideComment = 1;
                continue;
            }
        }
        if (editorConfig.syntax->flags & SYNTAX_HIGHLIGHT_STRINGS)
        {
            if (insideString)
            {
                row->highLight[i] = HIGHLIGHT_STRING;
                if (c == '\\' && i + 1 < row->renderSize)
                {
                    row->highLight[i + 1] = HIGHLIGHT_STRING;
                    i += 2;
                    continue;
                }
                if (c == insideString)
                    insideString = 0;
                i++;
                previousSeperator = true;
                continue;
            }
            else
            {
                if (c == '"' || c == '\'')
                {
                    insideString = c;
                    row->highLight[i] = HIGHLIGHT_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (editorConfig.syntax->flags & SYNTAX_HIGHLIGHT_NUMBERS)
        {
            if ((isdigit(c) && (previousSeperator || prev_hl == HIGHLIGHT_NUMBER)) ||
                (c == '.' && prev_hl == HIGHLIGHT_NUMBER))
            {
                row->highLight[i] = HIGHLIGHT_NUMBER;
                i++;
                previousSeperator = false;
                continue;
            }
        }
        if (previousSeperator)
        {
            uint32_t j;
            for (j = 0; keywords[j]; j++)
            {
                size_t keywordLength = strlen(keywords[j]);
                bool belongsToSecondKeywordGroup = keywords[j][keywordLength - 1] == '|';
                bool belongsToThirdKeywordGroup = keywords[j][keywordLength - 1] == '&';
                bool belongsToFourthKeywordGroup = keywords[j][keywordLength - 1] == '~';
                if (belongsToSecondKeywordGroup || belongsToThirdKeywordGroup || belongsToFourthKeywordGroup)
                    keywordLength--;
                if (!strncmp(&row->render[i], keywords[j], keywordLength) && editor_is_separator(row->render[i + keywordLength]))
                {
                    if(belongsToSecondKeywordGroup)
                        memset(&row->highLight[i], HIGHLIGHT_KEYWORDS_SECOND_GROUP, keywordLength);
                    else if(belongsToThirdKeywordGroup)
                        memset(&row->highLight[i], HIGHLIGHT_KEYWORDS_THIRD_GROUP, keywordLength);
                    else if(belongsToFourthKeywordGroup)
                        memset(&row->highLight[i], HIGHLIGHT_KEYWORDS_FOURTH_GROUP, keywordLength);
                    else
                        memset(&row->highLight[i], HIGHLIGHT_KEYWORDS_FIRST_GROUP, keywordLength);
                    i += keywordLength;
                    break;
                }
            }
            if (keywords[j])
            {
                previousSeperator = false;
                continue;
            }
        }
        previousSeperator = editor_is_separator(c);
        i++;
    }
    bool changed = (row->hightLightOpenComment != insideComment);
    row->hightLightOpenComment = insideComment;
    if (changed && row->index + 1 < editorConfig.numberOfRows)
        editor_update_syntax(&editorConfig.editorRows[row->index + 1]);
}

/// @brief Yanks the line the cursor is currently positioned in
static inline void editor_yank_line()
{
    copy_buffer_write(&editorConfig.copyBuffer, (char *)editorConfig.editorRows[editorConfig.cursorCurrentY].chars, editorConfig.editorRows[editorConfig.cursorCurrentY].size);
}
