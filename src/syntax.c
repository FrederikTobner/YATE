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
 * @file syntax.c
 * @brief File containing the implementation of the syntax highlighting functionality.
 */

#include "syntax.h"

char * CFileExtensions[] = {".c", ".h", NULL};

char * CKeywords[] = 
{
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "case",
    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", NULL
};

char * CPPFileExtensions[] = {".cpp", ".hpp", ".cc", ".hh", NULL};

char * CPPKeywords[] = 
{
    "switch", "if", "while", "for", "break", "continue", "return", "else",
    "struct", "union", "typedef", "static", "enum", "class", "case", "private", "public"
    "int|", "long|", "double|", "float|", "char|", "unsigned|", "signed|",
    "void|", NULL
};

char * CelloxFileExtensions[] = {".clx", NULL};

char * CelloxKeywords[] =
{
    "if", "else", "for", "while", "return", "and", "or", "null", "this", "super",
    "false", "true",
    "fun|", "class|", "var|", NULL
};

char * GoFileExtensions[] = { ".go", NULL };

char * GoKeywords[] = {
    // Go keywords
    "if", "for", "range", "while", "defer", "switch", "case", "else", "func", "package", "import", "type", "struct", "import", "const", "var",
    // Go types
    "nil|", "true|", "false|", "error|", "err|", "int|", "int32|", "int64|", "uint|", "uint32|", "uint64|", "string|", "bool|", NULL
};

char * LuaFileExtensions[] = {".lua", NULL};

char * LuaKeywords[] =
{
    "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "if",
    "in", "local|", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while", NULL
};


char * PythonFileExtensions[] = { ".py", NULL };

char * PythonKeywords[] = {
    // Python keywords
    "and", "as", "assert", "break", "class", "continue", "def", "del", "elif", "else",
    "except", "exec", "finally", "for", "from", "global", "if", "import", "in", "is", "lambda",
    "not", "or", "pass", "print", "raise", "return", "try", "while", "with", "yield",
    // Python types
    "buffer|", "bytearray|", "complex|", "False|", "float|", "frozenset|", "int|", "list|",
    "long|", "None|", "set|", "str|", "tuple|", "True|", "type|", "unicode|", "xrange|", NULL
};

editor_syntax_t HighLightDataBase[] = {
    {
        "C",
        CFileExtensions,
        CKeywords,
        "//", "/*", "*/",
        SYNTAX_HIGHLIGHT_NUMBERS | SYNTAX_HIGHLIGHT_STRINGS
    },
    {
        "C++",
        CPPFileExtensions,
        CPPKeywords,
        "//", "/*", "*/",
        SYNTAX_HIGHLIGHT_NUMBERS | SYNTAX_HIGHLIGHT_STRINGS
    },
    {
        "Cellox",
        CelloxFileExtensions,
        CelloxKeywords,
        "//", "/*", "*/",
        SYNTAX_HIGHLIGHT_NUMBERS | SYNTAX_HIGHLIGHT_STRINGS
    },
    {
        "Go",
        GoFileExtensions,
        GoKeywords,
        "#", "", "",
        SYNTAX_HIGHLIGHT_NUMBERS | SYNTAX_HIGHLIGHT_STRINGS
    },
    {
        "Lua",
        LuaFileExtensions,
        LuaKeywords,
        "--", "--[[", "--]]",
        SYNTAX_HIGHLIGHT_NUMBERS | SYNTAX_HIGHLIGHT_STRINGS
    },
    {
        "Python",
        PythonFileExtensions,
        PythonKeywords,
        "//", "", "",
        SYNTAX_HIGHLIGHT_NUMBERS | SYNTAX_HIGHLIGHT_STRINGS
    },
};

size_t syntax_get_language_count()
{
    return sizeof(HighLightDataBase) / sizeof(editor_syntax_t);
}

int32_t syntax_convert_to_color(editorHighlight highlightGroup)
{
    switch (highlightGroup)
    {
    case HIGHLIGHT_COMMENT:
    case HIGHLIGHT_MLCOMMENT:
        return (90 << 16) | (90 << 8) | 90;
    case HIGHLIGHT_KEYWORDS_FIRST_GROUP:
        return (211 << 16) | (33 << 8) | 45;;
    case HIGHLIGHT_KEYWORDS_SECOND_GROUP:
        return (55 << 16) | (187 << 8) | 255;
    case HIGHLIGHT_KEYWORDS_THIRD_GROUP:
        return (128 << 16) | (255 << 8) | 128;
    case HIGHLIGHT_KEYWORDS_FOURTH_GROUP:
        return (230 << 16) | (38 << 8) | 0;
    case HIGHLIGHT_STRING:
        return (255 << 16) | (166 << 8) | 77;
    case HIGHLIGHT_NUMBER:
        return (196 << 16) | (77 << 8) | 255;
    case HIGHLIGHT_MATCH:
        return (1 << 24) | (150 << 16) | (150 << 8) | 150;
    default:
        return (255 << 16) | (255 << 8) | 255;
    }
}