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
 * @file syntax.h
 * @brief File containing the declaration of the syntax highlighting functionality.
 */

#ifndef YATE_SYNTAX_H_
#define YATE_SYNTAX_H_

/// Syntax highlighting for numbers
#define SYNTAX_HIGHLIGHT_NUMBERS (1 << 0)

/// Syntax highlighting for strings
#define SYNTAX_HIGHLIGHT_STRINGS (1 << 1)

#include <stdint.h>
#include <stddef.h>

/// Differnt syntax highlighting groups of the editor
enum editorHighlight
{
    HIGHTLIGHT_NORMAL = 0,
    HIGHLIGHT_COMMENT,
    HIGHLIGHT_MLCOMMENT,
    HIGHLIGHT_KEYWORDS_FIRST_GROUP,
    HIGHLIGHT_KEYWORDS_SECOND_GROUP,
    HIGHLIGHT_KEYWORDS_THIRD_GROUP,
    HIGHLIGHT_KEYWORDS_FOURTH_GROUP,
    HIGHLIGHT_STRING,
    HIGHLIGHT_NUMBER,
    HIGHLIGHT_MATCH
};

/// Models a syntax for a programming language
typedef struct 
{
    /// The name of the type of file e.g. xml, Cellox
    char * filetype;
    /// The file extension of the language
    char ** filematch;
    /// The keywords of the language
    char ** keywords;
    /// Single line comment syntax of the language
    char * singleline_comment_start;
    /// Start pattern of a multiline comment
    char * multiline_comment_start;
    /// End pattern of a multiline comment
    char * multiline_comment_end;
    /// General Language flags (e.g higlight all strings / numbers)
    uint32_t flags;
} editor_syntax_t;

/// Syntax Highlighting database
extern editor_syntax_t HighLightDataBase [];

/// @brief Gets the amount of languages for which syntax highlighting is provided
/// @return The number of languages with syntax highlighting
size_t syntax_get_language_count();

/// @brief Get the rgb color value of a syntax highlighting group
/// @param highlightGroup The highlight group where the corresponding color value is determined
/// @return A 32 bit value where the 3 least significant bytes control the displayed color and 
/// the most significant bit is used to detetemnine whether the highlighting applies to the fore- or the background.
int32_t syntax_convert_to_color(int highlightGroup);

#endif