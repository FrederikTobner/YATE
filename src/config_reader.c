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
 * @file config_reader.c
 * @brief File containing the implementation of the configuration reader and the corresponding functions.
 */

#include "config_reader.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "editor.h"

/// default size of a tab stop converted to whitespace's
#define DEFAULT_TAB_STOP_SIZE (4)

/// default duration a status message is diplayed (in seconds)
#define DEFAULT_STATUS_MESSAGE_DURATION (5)

static int configuration_reader_parse_configuration_file_line(char **, configuration_reader_result_t **);
static void configuration_reader_string_behead(char *);
static void configuration_reader_string_trim(char *);

configuration_reader_result_t * configuration_reader_read_configuration_file()
{
    configuration_reader_result_t * result = malloc(sizeof(configuration_reader_result_t));
    result->tabStopSize = DEFAULT_TAB_STOP_SIZE;
    result->messageDisplayDuration = DEFAULT_STATUS_MESSAGE_DURATION;
    char configFilePath[120];
    snprintf(configFilePath, 120, "%s/.yaterc", getenv("HOME"));
    FILE * file = fopen(configFilePath, "rb");
    if (!file)
        return result;
    // Seek end of the file
    fseek(file, 0L, SEEK_END);
    // Store filesize
    size_t fileSize = ftell(file);
    // Rewind filepointer to the beginning of the file
    rewind(file);
    // Allocate memory apropriate to store the file
    char * buffer = (char *)malloc(fileSize + 1);
    if (!buffer)
        return result;
    // Store amount of read bytes
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
        return result;
    // We add null the end of the source-code to mark the end of the file
    buffer[bytesRead] = '\0';
    fclose(file);
    char * bufferStart = buffer;
    
    while(configuration_reader_parse_configuration_file_line(&buffer, &result));

    free(bufferStart);
    return result;
}

/// @brief Parses a single line in from a editor configuration file
/// @param fileContentPointer Pointer to the pointer that points to the character sequence representing the configuration file content
/// @param result The result of the config file reading process
/// @return 0 if the current line is the last line in the configuration file, otherwise 1
static int configuration_reader_parse_configuration_file_line(char ** fileContentPointer, configuration_reader_result_t ** result)
{
    printf("%s", *fileContentPointer);
    if((**fileContentPointer) == '\0')
            return 0;
    int parsingOption = 1;
    size_t optionRawLength = 0;
    size_t argumentRawLength = 0;
    while((**fileContentPointer) != '\n')
    {
        if((**fileContentPointer) == '\0')
            break;
        if((**fileContentPointer) == '=')
            parsingOption = 0;
        else if(parsingOption)
            optionRawLength++;
        else 
            argumentRawLength++;

        (*fileContentPointer)++;
    }
    
    if(!optionRawLength || !argumentRawLength)
    {
        if((**fileContentPointer) == '\0')
            return 0;
        (*fileContentPointer)++;
        return -1;
    }

    char * optionRaw  = malloc(optionRawLength + 1);
    char * argumentRaw = malloc(argumentRawLength + 1);
    memccpy(optionRaw, (*fileContentPointer) - optionRawLength - argumentRawLength - 1, sizeof(char), optionRawLength);
    optionRaw[optionRawLength] = '\0';
    argumentRaw[argumentRawLength] = '\0';
    memccpy(argumentRaw, (*fileContentPointer) - argumentRawLength, sizeof(char), argumentRawLength);
    configuration_reader_string_trim(optionRaw);
    configuration_reader_string_trim(argumentRaw);
    if(!strcasecmp(optionRaw, "TAB_STOP_SIZE"))
    {
        if(isdigit(*argumentRaw) || *argumentRaw == '-')
            (*result)->tabStopSize = atol(argumentRaw);
    }
    else if(!strcasecmp(optionRaw, "STATUS_MESSAGE_DURATION"))
    {
        if(isdigit(*argumentRaw) || *argumentRaw == '-')
            (*result)->messageDisplayDuration = atol(argumentRaw);
    }
    free(optionRaw);
    free(argumentRaw);
    (*fileContentPointer)++;
    return 1;
}

/// @brief Removes the first character in a character sequence
/// @param str The character sequnce where the first character is removed
static void configuration_reader_string_behead(char * str)
{
    size_t stringLength = strlen(str);
    for (size_t j = 0u; j < stringLength; j++)
        *(str + j) = *(str + j + 1);
    *(str + stringLength) = '\0';
}

/// @brief Removes all the whitespace characters from the beginning and the end of a character sequence
/// @param str The character sequence where the whitespace characters are removed
static void configuration_reader_string_trim(char * str)
{
	while (isspace(str[0]) || str[0] == '\r')
		configuration_reader_string_behead(str);
	for (size_t i = strlen(str) - 1; i > 0; i--)
	{
		if (!isspace(str[i]))
			break;
		str[i] = '\0';
	}
}
