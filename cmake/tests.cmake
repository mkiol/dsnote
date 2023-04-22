if(BUILD_CATCH2)
    include(FetchContent)

    FetchContent_Declare(
      Catch2
      URL ${catch2_source_url}
      URL_HASH MD5=${catch2_checksum}
      SOURCE_DIR "${external_dir}/catch2"
      BINARY_DIR "${PROJECT_BINARY_DIR}/external/catch2"
      INSTALL_DIR "${PROJECT_BINARY_DIR}/external"
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

include(CTest)
include(Catch)

catch_discover_tests(tests)
