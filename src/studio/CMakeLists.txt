if(GSPL_BUILD_STUDIO)
  find_package(Qt6 QUIET COMPONENTS Quick QuickControls2 Sql Network)
  if(NOT Qt6_FOUND)
    message(FATAL_ERROR "GSPL_BUILD_STUDIO requires Qt6 (Quick, QuickControls2, Sql, Network)")
  endif()

  add_library(gspl_studio

    # IPC foundation
    src/studio/ipc_envelope.cpp
    src/studio/ipc_channel.cpp
    src/studio/worker_process.cpp
    src/studio/shared_memory.cpp

    # Project model
    src/studio/project.cpp
    src/studio/workspace.cpp
    src/studio/document.cpp
  )

  target_include_directories(gspl_studio PUBLIC include)
  target_link_libraries(gspl_studio PUBLIC
    gspl_sprites_core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Network
  )

  target_compile_features(gspl_studio PUBLIC cxx_std_23)
  set_target_properties(gspl_studio PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

  if(MSVC)
    target_compile_options(gspl_studio PRIVATE /W4 /WX /permissive- /EHsc)
  else()
    target_compile_options(gspl_studio PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()

  # Test target
  if(BUILD_TESTING)
    add_executable(gspl_studio_tests
      tests/studio/ipc_tests.cpp
      tests/studio/project_tests.cpp
      tests/studio/workspace_tests.cpp
      tests/studio/document_tests.cpp
    )
    target_link_libraries(gspl_studio_tests PRIVATE gspl_studio)
    add_test(NAME gspl_studio_tests COMMAND gspl_studio_tests)
  endif()
endif()
