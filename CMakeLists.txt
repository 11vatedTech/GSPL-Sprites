cmake_minimum_required(VERSION 3.25)
project(gspl_sprites VERSION 0.1.0 LANGUAGES C CXX)

option(BUILD_PREVIEW "Build the preview executable" OFF)
option(GSPL_CORE_ONLY "Build GSPL core without ONNX Runtime dependency (portable)" OFF)

set(GSPL_EXPECT_COMPILER "" CACHE STRING "Expected CXX compiler ID: MSVC, GNU, Clang, AppleClang")
if(GSPL_EXPECT_COMPILER AND NOT CMAKE_CXX_COMPILER_ID MATCHES "${GSPL_EXPECT_COMPILER}")
  message(FATAL_ERROR "GSPL_EXPECT_COMPILER is \"${GSPL_EXPECT_COMPILER}\" "
          "but CMAKE_CXX_COMPILER_ID is \"${CMAKE_CXX_COMPILER_ID}\" "
          "[compiler: ${CMAKE_CXX_COMPILER}]")
endif()
message(STATUS "GSPL Sprites compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")

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

if(GSPL_CORE_ONLY)
  message(STATUS "GSPL_CORE_ONLY: skipping ONNX Runtime dependency")
  target_compile_definitions(gspl_spng INTERFACE GSPL_CORE_ONLY)
else()
  if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The initial governed ONNX Runtime provider currently supports Windows x64 only")
  endif()
  FetchContent_Declare(gspl_onnxruntime
    URL https://github.com/microsoft/onnxruntime/releases/download/v1.26.0/onnxruntime-win-x64-1.26.0.zip
    URL_HASH SHA256=6ebe99b5564bf4d029b6e93eac9ff423682b6212eade769e9ca3f685eaf500b4
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_SUBDIR __gspl_prebuilt_distribution__
  )
  FetchContent_MakeAvailable(gspl_onnxruntime)
  add_library(gspl_onnxruntime_cpu SHARED IMPORTED GLOBAL)
  set_target_properties(gspl_onnxruntime_cpu PROPERTIES
    IMPORTED_LOCATION "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${gspl_onnxruntime_SOURCE_DIR}/include"
  )
  if(MINGW)
    if(EXISTS "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll.a")
      set_target_properties(gspl_onnxruntime_cpu PROPERTIES
        IMPORTED_IMPLIB "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll.a")
    else()
      message(STATUS "Generating MinGW import library for ONNX Runtime...")
      find_program(GENDEF gendef)
      if(GENDEF)
        execute_process(COMMAND ${GENDEF} - "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll"
          OUTPUT_FILE "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.def"
          ERROR_QUIET)
      endif()
      if(EXISTS "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.def")
        execute_process(COMMAND ${CMAKE_DLLTOOL} -d "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.def"
          -l "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll.a" -k)
        set_target_properties(gspl_onnxruntime_cpu PROPERTIES
          IMPORTED_IMPLIB "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.dll.a")
      else()
        set_target_properties(gspl_onnxruntime_cpu PROPERTIES
          IMPORTED_IMPLIB "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib")
      endif()
    endif()
  else()
    set_target_properties(gspl_onnxruntime_cpu PROPERTIES
      IMPORTED_IMPLIB "${gspl_onnxruntime_SOURCE_DIR}/lib/onnxruntime.lib")
  endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(gspl_sprites_core
  src/core.cpp
  src/domain.cpp
  src/image.cpp
  src/animation.cpp
  src/package.cpp
  src/package_semantics.cpp
  src/model.cpp
  src/jobs.cpp
  src/image_png.cpp
  src/sprite2d.cpp
  src/visual_set.cpp
  src/channel_map.cpp
  src/living_runtime.cpp
  src/runtime_persistence.cpp
  src/runtime_replication.cpp
  src/combat.cpp
  src/combo.cpp
  src/transformation.cpp
  src/transformation_persistence.cpp
  src/transformation_manifestation.cpp
  src/runtime_event_router.cpp
  src/runtime_consumers.cpp
  src/projection25d.cpp
  src/projection2d.cpp
  src/projection3d.cpp
  src/animation3d.cpp
  src/gltf_export.cpp
  src/gltf_verify.cpp
  src/mesh_quality.cpp
  src/deformation_quality.cpp
  src/lod_quality.cpp
  src/runtime25d.cpp
  src/target_contract.cpp
  src/target_evidence.cpp
  src/godot_export.cpp
  src/authoring.cpp
  src/authoring_io.cpp
  src/authoring_cli.cpp
  src/synthesis.cpp
  src/headless_evidence.cpp
  src/source.cpp
  src/utf8.cpp
  src/diagnostics.cpp
  src/lexer.cpp
  src/parser.cpp
  src/modules.cpp
  src/types.cpp
  src/expressions.cpp
  src/genes.cpp
  src/ir.cpp
  src/passes.cpp
  src/semantics.cpp
  src/lowering.cpp
  src/cache.cpp
  src/provider.cpp
  src/legacy.cpp
  src/sdk.cpp
  src/cli.cpp
)
if(GSPL_CORE_ONLY)
  target_sources(gspl_sprites_core PRIVATE src/inference_stub.cpp)
  target_compile_definitions(gspl_sprites_core PRIVATE GSPL_CORE_ONLY)
else()
  target_sources(gspl_sprites_core PRIVATE src/inference_onnx.cpp)
endif()
target_include_directories(gspl_sprites_core PUBLIC include)
if(MSVC)
  target_compile_options(gspl_sprites_core PRIVATE /W4 /WX /permissive- /EHsc)
else()
  target_compile_options(gspl_sprites_core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
if(GSPL_CORE_ONLY)
  target_link_libraries(gspl_sprites_core PRIVATE gspl_spng)
else()
  target_link_libraries(gspl_sprites_core PRIVATE gspl_spng gspl_onnxruntime_cpu)
endif()

function(gspl_stage_onnxruntime target)
  if(NOT GSPL_CORE_ONLY)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:gspl_onnxruntime_cpu>" "$<TARGET_FILE_DIR:${target}>"
    )
  endif()
endfunction()

add_executable(gspl-sprites src/main.cpp)
target_link_libraries(gspl-sprites PRIVATE gspl_sprites_core)
if(NOT GSPL_CORE_ONLY)
  gspl_stage_onnxruntime(gspl-sprites)
endif()

add_executable(gsplc src/gsplc_main.cpp)
target_link_libraries(gsplc PRIVATE gspl_sprites_core)

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

if(NOT GSPL_CORE_ONLY)
  add_executable(gspl_sprites_inference_tests tests/inference_tests.cpp)
  target_link_libraries(gspl_sprites_inference_tests PRIVATE gspl_sprites_core)
  gspl_stage_onnxruntime(gspl_sprites_inference_tests)
  add_test(NAME gspl_sprites_inference_tests COMMAND gspl_sprites_inference_tests)
endif()

add_executable(gspl_sprites_2d_tests tests/sprite2d_tests.cpp)
target_link_libraries(gspl_sprites_2d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_2d_tests COMMAND gspl_sprites_2d_tests)

add_executable(gspl_sprites_visual_set_tests tests/visual_set_tests.cpp)
target_link_libraries(gspl_sprites_visual_set_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_visual_set_tests COMMAND gspl_sprites_visual_set_tests)

add_executable(gspl_sprites_cli_tests tests/cli_tests.cpp)
target_link_libraries(gspl_sprites_cli_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_cli_tests COMMAND gspl_sprites_cli_tests $<TARGET_FILE:gspl-sprites>)

add_executable(gspl_sprites_channel_map_tests tests/channel_map_tests.cpp)
target_link_libraries(gspl_sprites_channel_map_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_channel_map_tests COMMAND gspl_sprites_channel_map_tests)

add_executable(gspl_sprites_living_runtime_tests tests/living_runtime_tests.cpp)
target_link_libraries(gspl_sprites_living_runtime_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_living_runtime_tests COMMAND gspl_sprites_living_runtime_tests)

add_executable(gspl_sprites_runtime_persistence_tests tests/runtime_persistence_tests.cpp)
target_link_libraries(gspl_sprites_runtime_persistence_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_runtime_persistence_tests COMMAND gspl_sprites_runtime_persistence_tests)

add_executable(gspl_sprites_runtime_replication_tests tests/runtime_replication_tests.cpp)
target_link_libraries(gspl_sprites_runtime_replication_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_runtime_replication_tests COMMAND gspl_sprites_runtime_replication_tests)

add_executable(gspl_sprites_combat_tests tests/combat_tests.cpp)
target_link_libraries(gspl_sprites_combat_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_combat_tests COMMAND gspl_sprites_combat_tests)

add_executable(gspl_sprites_combo_tests tests/combo_tests.cpp)
target_link_libraries(gspl_sprites_combo_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_combo_tests COMMAND gspl_sprites_combo_tests)

add_executable(gspl_sprites_transformation_tests tests/transformation_tests.cpp)
target_link_libraries(gspl_sprites_transformation_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_transformation_tests COMMAND gspl_sprites_transformation_tests)

add_executable(gspl_sprites_transformation_persistence_tests tests/transformation_persistence_tests.cpp)
target_link_libraries(gspl_sprites_transformation_persistence_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_transformation_persistence_tests COMMAND gspl_sprites_transformation_persistence_tests)

add_executable(gspl_sprites_transformation_manifestation_tests tests/transformation_manifestation_tests.cpp)
target_link_libraries(gspl_sprites_transformation_manifestation_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_transformation_manifestation_tests COMMAND gspl_sprites_transformation_manifestation_tests)

add_executable(gspl_sprites_transformation_manifestation25d_tests tests/transformation_manifestation25d_tests.cpp)
target_link_libraries(gspl_sprites_transformation_manifestation25d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_transformation_manifestation25d_tests COMMAND gspl_sprites_transformation_manifestation25d_tests)

add_executable(gspl_sprites_transformation_manifestation2d_tests tests/transformation_manifestation2d_tests.cpp)
target_link_libraries(gspl_sprites_transformation_manifestation2d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_transformation_manifestation2d_tests COMMAND gspl_sprites_transformation_manifestation2d_tests)

add_executable(gspl_sprites_transformed_entity_acceptance_tests tests/transformed_entity_acceptance_tests.cpp)
target_link_libraries(gspl_sprites_transformed_entity_acceptance_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_transformed_entity_acceptance_tests COMMAND gspl_sprites_transformed_entity_acceptance_tests)

add_executable(gspl_sprites_unified_cross_representation_tests tests/unified_cross_rep_tests.cpp)
target_link_libraries(gspl_sprites_unified_cross_representation_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_unified_cross_representation_tests COMMAND gspl_sprites_unified_cross_representation_tests)

add_executable(gspl_sprites_runtime_event_router_tests tests/runtime_event_router_tests.cpp)
target_link_libraries(gspl_sprites_runtime_event_router_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_runtime_event_router_tests COMMAND gspl_sprites_runtime_event_router_tests)

add_executable(gspl_sprites_runtime_consumers_tests tests/runtime_consumers_tests.cpp)
target_link_libraries(gspl_sprites_runtime_consumers_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_runtime_consumers_tests COMMAND gspl_sprites_runtime_consumers_tests)

add_executable(gspl_sprites_projection25d_tests tests/projection25d_tests.cpp)
target_link_libraries(gspl_sprites_projection25d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_projection25d_tests COMMAND gspl_sprites_projection25d_tests)

add_executable(gspl_sprites_projection2d_tests tests/projection2d_tests.cpp)
target_link_libraries(gspl_sprites_projection2d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_projection2d_tests COMMAND gspl_sprites_projection2d_tests)

add_executable(gspl_sprites_synthesis_tests tests/synthesis_tests.cpp)
target_link_libraries(gspl_sprites_synthesis_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_synthesis_tests COMMAND gspl_sprites_synthesis_tests)

add_executable(gspl_sprites_projection3d_tests tests/projection3d_tests.cpp)
target_link_libraries(gspl_sprites_projection3d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_projection3d_tests COMMAND gspl_sprites_projection3d_tests)

add_executable(gspl_sprites_animation3d_tests tests/animation3d_tests.cpp)
target_link_libraries(gspl_sprites_animation3d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_animation3d_tests COMMAND gspl_sprites_animation3d_tests)

add_executable(gspl_sprites_gltf_export_tests tests/gltf_export_tests.cpp)
target_link_libraries(gspl_sprites_gltf_export_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_gltf_export_tests COMMAND gspl_sprites_gltf_export_tests)

add_executable(gspl_sprites_mesh_quality_tests tests/mesh_quality_tests.cpp)
target_link_libraries(gspl_sprites_mesh_quality_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_mesh_quality_tests COMMAND gspl_sprites_mesh_quality_tests)

add_executable(gspl_sprites_deformation_quality_tests tests/deformation_quality_tests.cpp)
target_link_libraries(gspl_sprites_deformation_quality_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_deformation_quality_tests COMMAND gspl_sprites_deformation_quality_tests)

add_executable(gspl_sprites_lod_quality_tests tests/lod_quality_tests.cpp)
target_link_libraries(gspl_sprites_lod_quality_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_lod_quality_tests COMMAND gspl_sprites_lod_quality_tests)

add_executable(gspl_sprites_runtime25d_tests tests/runtime25d_tests.cpp)
target_link_libraries(gspl_sprites_runtime25d_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_runtime25d_tests COMMAND gspl_sprites_runtime25d_tests)

add_executable(gspl_sprites_target_contract_tests tests/target_contract_tests.cpp)
target_link_libraries(gspl_sprites_target_contract_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_target_contract_tests COMMAND gspl_sprites_target_contract_tests $<TARGET_FILE:gspl-sprites>)

add_executable(gspl_sprites_godot_export_tests tests/godot_export_tests.cpp)
target_link_libraries(gspl_sprites_godot_export_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_godot_export_tests COMMAND gspl_sprites_godot_export_tests)

add_executable(gspl_sprites_authoring_tests tests/authoring_tests.cpp)
target_link_libraries(gspl_sprites_authoring_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_authoring_tests COMMAND gspl_sprites_authoring_tests)

add_executable(gspl_sprites_authoring_io_tests tests/authoring_io_tests.cpp)
target_link_libraries(gspl_sprites_authoring_io_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_authoring_io_tests COMMAND gspl_sprites_authoring_io_tests)

add_executable(gspl_sprites_authoring_cli_tests tests/authoring_cli_tests.cpp)
target_link_libraries(gspl_sprites_authoring_cli_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_authoring_cli_tests COMMAND gspl_sprites_authoring_cli_tests $<TARGET_FILE:gspl-sprites>)

add_executable(gspl_sprites_living_combat_wiring_tests tests/living_combat_wiring_tests.cpp)
target_link_libraries(gspl_sprites_living_combat_wiring_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_living_combat_wiring_tests COMMAND gspl_sprites_living_combat_wiring_tests)

add_executable(gspl_sprites_voltfox_integration_tests tests/voltfox_integration_tests.cpp)
target_link_libraries(gspl_sprites_voltfox_integration_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_voltfox_integration_tests COMMAND gspl_sprites_voltfox_integration_tests)

add_executable(gspl_sprites_frame_distinctness_tests tests/frame_distinctness_tests.cpp)
target_link_libraries(gspl_sprites_frame_distinctness_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_frame_distinctness_tests COMMAND gspl_sprites_frame_distinctness_tests)

add_executable(gspl_sprites_entity_identity_tests tests/entity_identity_tests.cpp)
target_link_libraries(gspl_sprites_entity_identity_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_entity_identity_tests COMMAND gspl_sprites_entity_identity_tests)

add_executable(gspl_sprites_headless_evidence_tests tests/headless_evidence_tests.cpp)
target_link_libraries(gspl_sprites_headless_evidence_tests PRIVATE gspl_sprites_core)
target_compile_definitions(gspl_sprites_headless_evidence_tests PRIVATE
  GSPL_SPRITES_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME gspl_sprites_headless_evidence_tests COMMAND gspl_sprites_headless_evidence_tests)

add_executable(gspl_sprites_utf8_tests tests/utf8_tests.cpp)
target_link_libraries(gspl_sprites_utf8_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_utf8_tests COMMAND gspl_sprites_utf8_tests)

add_executable(gspl_sprites_module_tests tests/module_tests.cpp)
target_link_libraries(gspl_sprites_module_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_module_tests COMMAND gspl_sprites_module_tests)

add_executable(gspl_sprites_type_system_tests tests/type_system_tests.cpp)
target_link_libraries(gspl_sprites_type_system_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_type_system_tests COMMAND gspl_sprites_type_system_tests)

add_executable(gspl_sprites_expression_tests tests/expression_tests.cpp)
target_link_libraries(gspl_sprites_expression_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_expression_tests COMMAND gspl_sprites_expression_tests)

add_executable(gspl_sprites_gene_tests tests/gene_tests.cpp)
target_link_libraries(gspl_sprites_gene_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_gene_tests COMMAND gspl_sprites_gene_tests)

add_executable(gspl_sprites_resource_limit_tests tests/resource_limit_tests.cpp)
target_link_libraries(gspl_sprites_resource_limit_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_resource_limit_tests COMMAND gspl_sprites_resource_limit_tests)

add_executable(gspl_sprites_cache_tests tests/cache_tests.cpp)
target_link_libraries(gspl_sprites_cache_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_cache_tests COMMAND gspl_sprites_cache_tests)

add_executable(gspl_sprites_provider_tests tests/provider_tests.cpp)
target_link_libraries(gspl_sprites_provider_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_provider_tests COMMAND gspl_sprites_provider_tests)

add_executable(gspl_sprites_legacy_tests tests/legacy_tests.cpp)
target_link_libraries(gspl_sprites_legacy_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_legacy_tests COMMAND gspl_sprites_legacy_tests)

add_executable(gspl_sprites_fuzz_parsing_tests tests/fuzz_parsing_tests.cpp)
target_link_libraries(gspl_sprites_fuzz_parsing_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_fuzz_parsing_tests COMMAND gspl_sprites_fuzz_parsing_tests)

add_executable(gspl_sprites_mutation_tests tests/mutation_tests.cpp)
target_link_libraries(gspl_sprites_mutation_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_mutation_tests COMMAND gspl_sprites_mutation_tests)

add_executable(gspl_sprites_compiler_tests tests/gspl_compiler_tests.cpp)
target_link_libraries(gspl_sprites_compiler_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_compiler_tests COMMAND gspl_sprites_compiler_tests)

add_executable(gspl_sprites_semantic_pipeline_tests tests/semantic_pipeline_tests.cpp)
target_link_libraries(gspl_sprites_semantic_pipeline_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_semantic_pipeline_tests COMMAND gspl_sprites_semantic_pipeline_tests)

add_executable(gspl_sprites_cli_comprehensive_tests tests/cli_comprehensive_tests.cpp)
target_link_libraries(gspl_sprites_cli_comprehensive_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_cli_comprehensive_tests COMMAND gspl_sprites_cli_comprehensive_tests)

add_executable(gspl_sprites_header_tests tests/header_self_containment_tests.cpp)
target_link_libraries(gspl_sprites_header_tests PRIVATE gspl_sprites_core)
add_test(NAME gspl_sprites_header_tests COMMAND gspl_sprites_header_tests)

if(BUILD_PREVIEW)
  find_package(SDL2 QUIET)
  if(SDL2_FOUND)
    add_executable(gspl_sprites_preview src/preview.cpp)
    target_link_libraries(gspl_sprites_preview PRIVATE gspl_sprites_core SDL2::SDL2)
    target_compile_definitions(gspl_sprites_preview PRIVATE GSPL_SPRITES_HAS_SDL2)
  else()
    message(STATUS "SDL2 not found — preview target will build as a headless functional test")
    add_executable(gspl_sprites_preview src/preview.cpp)
    target_link_libraries(gspl_sprites_preview PRIVATE gspl_sprites_core)
  endif()
  gspl_stage_onnxruntime(gspl_sprites_preview)
endif()
