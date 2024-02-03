# only finished translations
set(enabled_translations en fr it nl pl sv uk)
#set(enabled_translations cs de en fr it nl pl sv zh_CN uk)

find_package(Qt5 COMPONENTS Core LinguistTools)

set(ts_files "")
foreach(lang ${enabled_translations})
    list(APPEND ts_files "${translations_dir}/dsnote-${lang}.ts")
endforeach()

qt5_create_translation(qm_files ${CMAKE_SOURCE_DIR}/src ${desktop_dir}/qml ${sfos_dir}/qml ${ts_files})

string(REPLACE ";" " " enabled_translations_str "${enabled_translations}")
add_custom_command(
  OUTPUT translations.qrc
  COMMAND sh -c "${tools_dir}/make_translations_qrc.sh ${info_translations_id} /translations ${CMAKE_BINARY_DIR}/translations.qrc ${enabled_translations_str}"
  DEPENDS ${qm_files}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM)
add_library(translations STATIC "${CMAKE_BINARY_DIR}/translations.qrc")
target_link_libraries(${info_binary_id} translations)
