if(BUILD_CATCH2)
    include(FetchContent)

    set(catch2_source_url "https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz")
    set(catch2_checksum "SHA256=269543a49eb76f40b3f93ff231d4c24c27a7e16c90e47d2e45bcc564de470c6e")

    FetchContent_Declare(
      Catch2
      URL ${catch2_source_url}
      URL_HASH ${catch2_checksum}
      SOURCE_DIR "${external_dir}/catch2"
      BINARY_DIR "${PROJECT_BINARY_DIR}/external/catch2"
      INSTALL_DIR "${PROJECT_BINARY_DIR}/external"
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(Catch2)
else()
    find_package(Catch2 3 REQUIRED)
endif()

file(GLOB tests_src
    "${tests_dir}/*.hpp"
    "${tests_dir}/*.cpp"
)

add_executable(tests ${tests_src})
target_link_libraries(tests Catch2::Catch2WithMain)
target_link_libraries(tests dsnote_lib)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# run tests in build step
#include(CTest)
#include(Catch)
#catch_discover_tests(tests)
