install(TARGETS ${info_binary_id} RUNTIME DESTINATION bin)

install(FILES "${sfos_dir}/${info_binary_id}.desktop" DESTINATION share/applications)

configure_file("${sfos_dir}/dbus_app.service.in" "${PROJECT_BINARY_DIR}/dbus_app.service")
install(FILES "${PROJECT_BINARY_DIR}/dbus_app.service" DESTINATION share/dbus-1/services RENAME ${info_dbus_app_service}.service)

if(BUILD_WHISPERCPP)
    install(FILES "${external_lib_dir}/libwhisper-openblas.so" DESTINATION share/${info_binary_id}/lib)
    install(FILES "${external_lib_dir}/libwhisper-fallback.so" DESTINATION DESTINATION share/${info_binary_id}/lib)
endif()

if(DOWNLOAD_LIBSTT)
    install(FILES "${external_lib_dir}/libstt.so" DESTINATION share/${info_binary_id}/lib)
    install(FILES "${external_lib_dir}/libkenlm.so" DESTINATION share/${info_binary_id}/lib)
    install(FILES "${external_lib_dir}/libtensorflowlite.so" DESTINATION share/${info_binary_id}/lib)
    install(FILES "${external_lib_dir}/libtflitedelegates.so" DESTINATION share/${info_binary_id}/lib)
endif()

if(BUILD_OPENBLAS)
    install(FILES "${external_lib_dir}/libopenblas.so.0.3" DESTINATION share/${info_binary_id}/lib RENAME libopenblas.so.0)
endif()

if(BUILD_PIPER OR BUILD_ESPEAK)
    install(PROGRAMS "${external_bin_dir}/mbrola" DESTINATION share/${info_binary_id}/bin)
    install(FILES "${PROJECT_BINARY_DIR}/espeakdata.tar.xz" DESTINATION share/${info_binary_id})
endif()

if(BUILD_RHVOICE)
    install(FILES "${external_lib_dir}/libRHVoice_core.so.1.2.2" DESTINATION share/${info_binary_id}/lib RENAME libRHVoice_core.so.1)
    install(FILES "${external_lib_dir}/libRHVoice.so.1.2.2" DESTINATION share/${info_binary_id}/lib RENAME libRHVoice.so.1)
    install(FILES "${PROJECT_BINARY_DIR}/rhvoicedata.tar.xz" DESTINATION share/${info_binary_id})
    install(FILES "${PROJECT_BINARY_DIR}/rhvoiceconfig.tar.xz" DESTINATION share/${info_binary_id})
endif()

if(BUILD_PIPER)
    install(FILES "${external_lib_dir}/libonnxruntime.so.1.14.1" DESTINATION share/${info_binary_id}/lib)
endif()

if(BUILD_VOSK)
    install(FILES "${external_lib_dir}/libvosk.so" DESTINATION share/${info_binary_id}/lib)
endif()

if(BUILD_BERGAMOT)
    install(FILES "${external_lib_dir}/libbergamot_api.so" DESTINATION share/${info_binary_id}/lib)
endif()

if(BUILD_LIBNUMBERTEXT)
    install(DIRECTORY "${external_share_dir}/libnumbertext" DESTINATION share/${info_binary_id})
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

if(WITH_SYSTEMD_SERVICE)
    configure_file("${systemd_dir}/speech.service.in" "${PROJECT_BINARY_DIR}/speech.service")
    install(FILES "${PROJECT_BINARY_DIR}/speech.service" DESTINATION lib/systemd/user RENAME ${info_binary_id}.service)

    configure_file("${dbus_dir}/dbus_speech.service.in" "${PROJECT_BINARY_DIR}/dbus_speech.service")
    install(FILES "${PROJECT_BINARY_DIR}/dbus_speech.service" DESTINATION share/dbus-1/services RENAME ${info_dbus_speech_service}.service)
endif()
