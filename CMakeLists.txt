cmake_minimum_required(VERSION 3.25)
project(gspl_sprites VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(gspl_zlib
  URL https://github.com/madler/zlib/archive/da607da739fa6047df13e66a2af6b8bec7c2a498.tar.gz
  URL_HASH SHA256=b9258cf6254e7f7c37f1cd61dba943a1c5ea3cff5718c789834dac359094f5f7
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(gspl_zlib)

FetchContent_Declare(gspl_libspng
  URL https://github.com/randy408/libspng/archive/fb768002d4288590083a476af628e51c3f1d47cd.tar.gz
  URL_HASH SHA256=d656813290d70a750b69e768323ff3b3875adf8c2e8c2fdb4e57ca1467abf86a
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  SOURCE_SUBDIR __gspl_no_upstream_cmake__
)
FetchContent_MakeAvailable(gspl_libspng)
add_library(gspl_spng STATIC ${gspl_libspng_SOURCE_DIR}/spng/spng.c)
target_include_directories(gspl_spng PUBLIC ${gspl_libspng_SOURCE_DIR}/spng)
target_compile_definitions(gspl_spng PUBLIC SPNG_STATIC)
target_link_libraries(gspl_spng PRIVATE ZLIB::ZLIBSTATIC)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gspl_sprites_core
  src/core.cpp
  src/domain.cpp
  src/image.cpp
  src/animation.cpp
  src/package.cpp
  src/model.cpp
  src/jobs.cpp
  src/image_png.cpp
)
target_include_directories(gspl_sprites_core PUBLIC include)
if(MSVC)
  target_compile_options(gspl_sprites_core PRIVATE /W4 /WX /permissive- /EHsc)
else()
  target_compile_options(gspl_sprites_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
target_link_libraries(gspl_sprites_core PRIVATE gspl_spng)

add_executable(gspl-sprites src/main.cpp)
target_link_libraries(gspl-sprites PRIVATE gspl_sprites_core)

enable_testing()
add_executable(gspl_sprites_tests tests/core_tests.cpp)
target_link_libraries(gspl_sprites_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_tests COMMAND gspl_sprites_tests)

add_executable(gspl_sprites_domain_tests tests/domain_tests.cpp)
target_link_libraries(gspl_sprites_domain_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_domain_tests COMMAND gspl_sprites_domain_tests)

add_executable(gspl_sprites_image_tests tests/image_tests.cpp)
target_link_libraries(gspl_sprites_image_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_image_tests COMMAND gspl_sprites_image_tests)

add_executable(gspl_sprites_animation_tests tests/animation_tests.cpp)
target_link_libraries(gspl_sprites_animation_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_animation_tests COMMAND gspl_sprites_animation_tests)

add_executable(gspl_sprites_package_tests tests/package_tests.cpp)
target_link_libraries(gspl_sprites_package_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_package_tests COMMAND gspl_sprites_package_tests)

add_executable(gspl_sprites_model_tests tests/model_tests.cpp)
target_link_libraries(gspl_sprites_model_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_model_tests COMMAND gspl_sprites_model_tests)

add_executable(gspl_sprites_job_tests tests/job_tests.cpp)
target_link_libraries(gspl_sprites_job_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_job_tests COMMAND gspl_sprites_job_tests)
