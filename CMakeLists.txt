cmake_minimum_required(VERSION 3.25)
project(gspl_sprites VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gspl_sprites_core
  src/core.cpp
  src/domain.cpp
  src/image.cpp
)
target_include_directories(gspl_sprites_core PUBLIC include)
if(MSVC)
  target_compile_options(gspl_sprites_core PRIVATE /W4 /WX /permissive- /EHsc)
else()
  target_compile_options(gspl_sprites_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

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
