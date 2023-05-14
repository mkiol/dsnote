install(TARGETS ${info_binary_id} RUNTIME DESTINATION bin)

install(FILES "${sfos_dir}/${info_binary_id}.desktop" DESTINATION share/applications)

if(DOWNLOAD_LIBSTT)
    install(FILES "${external_lib_dir}/libstt.so" DESTINATION share/${info_binary_id}/lib)
    install(FILES "${external_lib_dir}/libkenlm.so" DESTINATION share/${info_binary_id}/lib)
endif()

if(BUILD_OPENBLAS)
    install(FILES "${external_lib_dir}/libopenblas.so.0.3" DESTINATION share/${info_binary_id}/lib RENAME libopenblas.so.0)
endif()

if(BUILD_PIPER OR BUILD_ESPEAK)
    install(PROGRAMS "${external_bin_dir}/mbrola" DESTINATION share/${info_binary_id}/bin)
    install(FILES "${PROJECT_BINARY_DIR}/espeakdata.tar.xz" DESTINATION share/${info_binary_id})
endif()

if(BUILD_PIPER)
    install(FILES "${external_lib_dir}/libonnxruntime.so.1.14.1" DESTINATION share/${info_binary_id}/lib RENAME libonnxruntime.so.1.14.1)
endif()

if(BUILD_VOSK)
    install(FILES "${external_lib_dir}/libvosk.so" DESTINATION share/${info_binary_id}/lib)
endif()

if(BUILD_PYTHON_MODULE)
    install(FILES ${PROJECT_BINARY_DIR}/python.tar.xz DESTINATION share/${info_binary_id})
endif()

install(FILES "${sfos_dir}/icons/86x86/${info_binary_id}.png" DESTINATION share/icons/hicolor/86x86/apps)
install(FILES "${sfos_dir}/icons/108x108/${info_binary_id}.png" DESTINATION share/icons/hicolor/108x108/apps)
install(FILES "${sfos_dir}/icons/128x128/${info_binary_id}.png" DESTINATION share/icons/hicolor/128x128/apps)
install(FILES "${sfos_dir}/icons/172x172/${info_binary_id}.png" DESTINATION share/icons/hicolor/172x172/apps)

install(FILES ${qm_files} DESTINATION "share/${info_binary_id}/translations")

install(DIRECTORY "${sfos_dir}/qml" DESTINATION share/${info_binary_id})

configure_file("${systemd_dir}/${info_id}.service" "${PROJECT_BINARY_DIR}/${info_binary_id}.service")
install(FILES "${PROJECT_BINARY_DIR}/${info_binary_id}.service" DESTINATION lib/systemd/user)

configure_file("${dbus_dir}/${info_dbus_service}.service" "${PROJECT_BINARY_DIR}/${info_dbus_service}.service")
install(FILES "${PROJECT_BINARY_DIR}/${info_dbus_service}.service" DESTINATION "share/dbus-1/services")
