function(AddTutorials Path)
    file(GLOB examples ${Path}/*.cpp)
    foreach (example ${examples})
        message(STATUS "File Path: ${example}")
        string(REGEX MATCHALL "[0-9A-Za-z_]*.cpp" file_name ${example})
        string(REGEX REPLACE ".cpp" "" file_base ${file_name})
        message(${file_base})
        #        ${ARGC}
        #        ${ARGV} ${ARGV0} ${ARGV1} ${ARGV2}
        #        get_filename_component(file_base ${example} NAME_WE)
        add_executable(${file_base} ${example})
        target_link_libraries(${file_base}
#                -Wl,--whole-archive
                ${ARGN}
#                -Wl,--no-whole-archive
        )
    endforeach ()
endfunction()

AddTutorials(${CMAKE_CURRENT_SOURCE_DIR} zcm)


