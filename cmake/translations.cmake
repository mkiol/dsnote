# all translations
set(enabled_translations ar ca_ES cs de en es fr fr_CA it nl pl ru sv sl zh_CN uk no)
# finished translations
set(enabled_translations ar ca_ES en es fr fr_CA it nl pl ru sv sl uk no)

find_package(Qt5 COMPONENTS Core LinguistTools)

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

qt5_create_translation(qm_files ${CMAKE_SOURCE_DIR}/src ${desktop_dir}/qml ${sfos_dir}/qml ${ts_files})

add_translations_resource(translations_res ${qm_files})
