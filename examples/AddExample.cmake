
function(add_example)

    set(flags TRY_LINK_IMGUI LINK_IMGUI LINK_ASSIMP LINK_JSON LINK_XML)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES EMSCRIPTEN_EMBED)

    cmake_parse_arguments(arg "${flags}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

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

    if (DEFINED EMSCRIPTEN)
        set_target_properties("${arg_NAME}"
                PROPERTIES SUFFIX ".html"
                LINK_FLAGS " --bind -s USE_GLFW=3 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 -s FULL_ES3 -s -s WASM=1 --embed-file ${EMSCRIPTEN_EMBED}")
    endif (DEFINED EMSCRIPTEN)

endfunction()
