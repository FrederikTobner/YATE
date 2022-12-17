cmake_minimum_required(VERSION 3.16)

# Yet another text editor
project(YATE VERSION 0.1.0)
include(CheckIncludeFile)

# C99 standard is required to build the editor
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(PROJECT_VENDOR "Frederik Tobner")

# Check dependecies under unix-like systems
if(UNIX)
    CHECK_INCLUDE_FILE("termios.h" TERMIOS_AVAILABLE)
    CHECK_INCLUDE_FILE("sys/ioctl.h" IOCTL_AVAILABLE)
    CHECK_INCLUDE_FILE("sys/types.h" TYPES_AVAILABLE)
    CHECK_INCLUDE_FILE("unistd.h" UNISTD_AVAILABLE)
    if(NOT ${TERMIOS_AVAILABLE})
        message(FATAL_ERROR "termios.h is required to build the editor")
    endif() # termios.h not available
    if(NOT ${IOCTL_AVAILABLE})
        message(FATAL_ERROR "ioctl.h is required to build the editor")
    endif() # ioctl.h not available
    if(NOT ${TYPES_AVAILABLE})
        message(FATAL_ERROR "sys/types.h is required to build the editor")
    endif() # types.h not available
    if(NOT ${UNISTD_AVAILABLE})
        message(FATAL_ERROR "unistd.h is required to build the editor")
    endif() # unistd.h not available
    
else()
    message(FATAL_ERROR "YATE is only supported under unix systems")
endif() # Unix system

add_subdirectory(src)