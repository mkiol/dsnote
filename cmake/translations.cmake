# all translations
set(enabled_translations ar ca_ES cs de en es fr fr_CA it nl no pt_BR pl ru sv sl tr_TR uk zh_CN zh_TW)
# finished translations
set(enabled_translations ar ca_ES de en es fr fr_CA it nl no pt_BR pl ru sv sl tr_TR uk zh_CN zh_TW)

# QT_VERSION_MAJOR is set in main CMakeLists.txt
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core LinguistTools)

set(ts_files "")
foreach(lang ${enabled_translations})
    list(APPEND ts_files "${translations_dir}/dsnote-${lang}.ts")
endforeach()

function(ADD_TRANSLATIONS_RESOURCE res_file)
    set(_qm_files ${ARGN})
    set(_res_file ${translations_resource_file})

    file(WRITE ${_res_file} "<!DOCTYPE RCC><RCC version=\"1.0\">\n <qresource prefix=\"/translations\">\n")
    foreach(_lang ${_qm_files})
        get_filename_component(_filename ${_lang} NAME)
        file(APPEND ${_res_file} "  <file>${_filename}</file>\n")
    endforeach()
    file(APPEND ${_res_file} " </qresource>\n</RCC>\n")

    set(${res_file} ${_res_file} PARENT_SCOPE)
endfunction()

if(QT_VERSION_MAJOR EQUAL 6)
    qt6_create_translation(qm_files ${CMAKE_SOURCE_DIR}/src ${desktop_dir}/qml ${sfos_dir}/qml ${ts_files})
else()
    qt5_create_translation(qm_files ${CMAKE_SOURCE_DIR}/src ${desktop_dir}/qml ${sfos_dir}/qml ${ts_files})
endif()

add_translations_resource(translations_res ${qm_files})
