# configure cellox config to show version
configure_file(project_config.h.in project_config.h)

set(EDITOR_SOURCES
append_buffer.c
copy_buffer.c
config_reader.c
editor.c
main.c
syntax.c
)

set(EDITOR_HEADERS
append_buffer.h
config_reader.h
copy_buffer.h
editor.h
syntax.h
)

if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    add_compile_definitions(DEBUG_BUILD_TYPE)
else()
    add_compile_definitions(RELEASE_BUILD_TYPE)
endif()

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)
add_executable(${PROJECT_NAME_LOWERCASE} ${EDITOR_SOURCES} ${EDITOR_HEADERS}) 
# for including the cellox_config.h file
target_include_directories(${PROJECT_NAME_LOWERCASE} PUBLIC ${PROJECT_BINARY_DIR}/src)

if(NOT CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    # Sets properties for the package created using cpack
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_VERSION "${YATE_VERSION_MAJOR}.${YATE_VERSION_MINOR}.${YATE_VERSION_PATCH}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} Yet another text editor. A simple text editor for the terminal")
    set(CPACK_BUILD_SOURCE_DIRS "${PROJECT_BINARY_DIR}/src")
    set(CPACK_PACKAGE_VENDOR ${PROJECT_VENDOR})
    
    include(CPack)
    install(TARGETS ${PROJECT_NAME_LOWERCASE} DESTINATION bin)
endif()
