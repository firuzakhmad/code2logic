# code2logic_configure_app_icon(<target> <png_path>)
#
# Windows: generates a multi-res .ico via Python+Pillow, embeds it into the
#          .exe through a generated .rc resource file.
# macOS:   generates a .icns via sips + iconutil (both ship with Xcode CLT),
#          turns the target into a real .app bundle.
# Linux:   no single-binary icon convention — just copies the PNG next to
#          the built binary for packaging alongside a .desktop file.

function(code2logic_configure_app_icon target png_path)
    if(NOT EXISTS ${png_path})
        message(WARNING "code2logic_configure_app_icon: '${png_path}' not found - skipping icon setup for ${target}")
        return()
    endif()

    if(WIN32)
        find_package(Python3 COMPONENTS Interpreter QUIET)
        set(PIL_AVAILABLE FALSE)
        if(Python3_FOUND)
            execute_process(
                COMMAND ${Python3_EXECUTABLE} -c "import PIL"
                RESULT_VARIABLE PIL_IMPORT_RESULT
                OUTPUT_QUIET ERROR_QUIET
            )
            if(PIL_IMPORT_RESULT EQUAL 0)
                set(PIL_AVAILABLE TRUE)
            endif()
        endif()

        if(PIL_AVAILABLE)
            set(ICON_GEN_DIR ${CMAKE_BINARY_DIR}/generated_icons)
            set(GENERATED_ICO ${ICON_GEN_DIR}/${target}.ico)
            set(GENERATED_RC  ${ICON_GEN_DIR}/${target}.rc)

            add_custom_command(
                OUTPUT ${GENERATED_ICO}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ICON_GEN_DIR}
                COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/png_to_ico.py ${png_path} ${GENERATED_ICO}
                DEPENDS ${png_path} ${CMAKE_SOURCE_DIR}/scripts/png_to_ico.py
                COMMENT "Generating Windows .ico for ${target} from ${png_path}"
                VERBATIM
            )
            file(WRITE ${GENERATED_RC} "IDI_ICON1 ICON \"${GENERATED_ICO}\"\n")

            target_sources(${target} PRIVATE ${GENERATED_ICO} ${GENERATED_RC})
            set_source_files_properties(${GENERATED_RC} PROPERTIES OBJECT_DEPENDS ${GENERATED_ICO})
        else()
            message(WARNING "Python3 + Pillow not found - ${target}.exe will ship with the default icon. "
                             "Run 'pip install pillow' and reconfigure to enable icon generation.")
        endif()

    elseif(APPLE)
        find_program(SIPS_EXECUTABLE sips)
        find_program(ICONUTIL_EXECUTABLE iconutil)

        if(SIPS_EXECUTABLE AND ICONUTIL_EXECUTABLE)
            set(ICONSET_DIR ${CMAKE_BINARY_DIR}/${target}.iconset)
            set(ICNS_OUTPUT ${CMAKE_BINARY_DIR}/${target}.icns)

            add_custom_command(
                OUTPUT ${ICNS_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${ICONSET_DIR}
                COMMAND ${SIPS_EXECUTABLE} -z 16 16   ${png_path} --out ${ICONSET_DIR}/icon_16x16.png
                COMMAND ${SIPS_EXECUTABLE} -z 32 32   ${png_path} --out ${ICONSET_DIR}/icon_16x16@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 32 32   ${png_path} --out ${ICONSET_DIR}/icon_32x32.png
                COMMAND ${SIPS_EXECUTABLE} -z 64 64   ${png_path} --out ${ICONSET_DIR}/icon_32x32@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 128 128 ${png_path} --out ${ICONSET_DIR}/icon_128x128.png
                COMMAND ${SIPS_EXECUTABLE} -z 256 256 ${png_path} --out ${ICONSET_DIR}/icon_128x128@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 256 256 ${png_path} --out ${ICONSET_DIR}/icon_256x256.png
                COMMAND ${SIPS_EXECUTABLE} -z 512 512 ${png_path} --out ${ICONSET_DIR}/icon_256x256@2x.png
                COMMAND ${SIPS_EXECUTABLE} -z 512 512 ${png_path} --out ${ICONSET_DIR}/icon_512x512.png
                COMMAND ${ICONUTIL_EXECUTABLE} -c icns ${ICONSET_DIR} -o ${ICNS_OUTPUT}
                DEPENDS ${png_path}
                COMMENT "Generating macOS .icns for ${target} from ${png_path}"
                VERBATIM
            )
            add_custom_target(${target}_icon DEPENDS ${ICNS_OUTPUT})
            add_dependencies(${target} ${target}_icon)

            set_target_properties(${target} PROPERTIES
                MACOSX_BUNDLE TRUE
                MACOSX_BUNDLE_ICON_FILE ${target}.icns
                MACOSX_BUNDLE_GUI_IDENTIFIER "com.code2logic.app"
                MACOSX_BUNDLE_BUNDLE_NAME ${target}
                MACOSX_BUNDLE_BUNDLE_VERSION ${CODE2LOGIC_VERSION}
                MACOSX_BUNDLE_SHORT_VERSION_STRING ${CODE2LOGIC_VERSION}
            )
            target_sources(${target} PRIVATE ${ICNS_OUTPUT})
            set_source_files_properties(${ICNS_OUTPUT} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        else()
            message(WARNING "sips/iconutil not found - skipping macOS icon generation for ${target}")
        endif()

    else() # Linux
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${png_path} $<TARGET_FILE_DIR:${target}>/${target}.png
        )
    endif()
endfunction()