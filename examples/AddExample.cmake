
function(add_example)

    set(flags TRY_LINK_IMGUI LINK_IMGUI LINK_ASSIMP LINK_JSON LINK_XML AUDIO WEB)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES WEB_EMBED)

    cmake_parse_arguments(arg "${flags}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (EMSCRIPTEN AND NOT arg_WEB)
        return()
    endif ()

    if (arg_LINK_IMGUI AND NOT imgui_FOUND)
        message(AUTHOR_WARNING "imgui not found, skipping '${arg_NAME}' example..")
        return()
    endif ()

    if (arg_LINK_ASSIMP AND NOT assimp_FOUND)
        message(AUTHOR_WARNING "assimp not found, skipping '${arg_NAME}' example..")
        return()
    endif ()

    if (arg_LINK_JSON AND NOT nlohmann_json_FOUND)
        message(AUTHOR_WARNING "nlohmann_json not found, skipping '${arg_NAME}' example..")
        return()
    endif ()

    if (arg_LINK_XML AND NOT pugixml_FOUND)
        message(AUTHOR_WARNING "pugixml not found, skipping '${arg_NAME}' example..")
        return()
    endif ()

    if (arg_AUDIO AND NOT MINIAUDIO_INCLUDE_DIRS)
        message(AUTHOR_WARNING "miniaudio not found, skipping '${arg_NAME}' example..")
        return()
    endif ()


    if (NOT arg_SOURCES)
        add_executable("${arg_NAME}" "${arg_NAME}.cpp")
    else ()
        add_executable("${arg_NAME}" "${arg_SOURCES}")
    endif ()

    target_link_libraries("${arg_NAME}" PRIVATE threepp)

    if ((arg_TRY_LINK_IMGUI OR arg_LINK_IMGUI) AND imgui_FOUND)
        target_link_libraries("${arg_NAME}" PRIVATE imgui::imgui)
    endif ()

    if (arg_LINK_ASSIMP AND assimp_FOUND)
        target_link_libraries("${arg_NAME}" PRIVATE assimp::assimp)
    endif ()

    if (arg_LINK_JSON AND nlohmann_json_FOUND)
        target_link_libraries("${arg_NAME}" PRIVATE nlohmann_json::nlohmann_json)
    endif ()

    if (arg_LINK_XML AND pugixml_FOUND)
        target_link_libraries("${arg_NAME}" PRIVATE pugixml::pugixml)
    endif ()

    if (arg_AUDIO AND MINIAUDIO_INCLUDE_DIRS)
        target_include_directories("${arg_NAME}" PRIVATE "${MINIAUDIO_INCLUDE_DIRS}")
    endif ()

    if (DEFINED EMSCRIPTEN)

        set(LINK_FLAGS " --bind -s USE_GLFW=3 -s GL_DEBUG=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES3 -s -s WASM=1")
        if (arg_WEB_EMBED)
            set(LINK_FLAGS "${LINK_FLAGS} --embed-file ${arg_WEB_EMBED}")
        endif ()

        set_target_properties("${arg_NAME}"
                PROPERTIES SUFFIX ".html"
                LINK_FLAGS "${LINK_FLAGS}")

    endif (DEFINED EMSCRIPTEN)

endfunction()
