if(GSPL_BUILD_STUDIO)
  find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Sql Network LinguistTools)
  qt_standard_project_setup(REQUIRES 6.8)

  add_library(gspl_studio_core
    ${CMAKE_SOURCE_DIR}/src/studio/project.cpp
    ${CMAKE_SOURCE_DIR}/src/studio/workspace.cpp
    ${CMAKE_SOURCE_DIR}/src/studio/document.cpp
  )
  target_include_directories(gspl_studio_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
  target_link_libraries(gspl_studio_core PUBLIC gspl_sprites_core)
  target_compile_features(gspl_studio_core PUBLIC cxx_std_23)
  set_target_properties(gspl_studio_core PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

  add_library(gspl_studio_qt
    ${CMAKE_SOURCE_DIR}/include/gspl/studio/application_controller.hpp
    ${CMAKE_SOURCE_DIR}/src/studio/worker_process.cpp
    ${CMAKE_SOURCE_DIR}/src/studio/shared_memory.cpp
    ${CMAKE_SOURCE_DIR}/src/studio/qt/application_controller.cpp
  )
  target_include_directories(gspl_studio_qt PUBLIC ${CMAKE_SOURCE_DIR}/include)
  target_link_libraries(gspl_studio_qt PUBLIC
    gspl_studio_core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Network
  )
  target_compile_features(gspl_studio_qt PUBLIC cxx_std_23)
  set_target_properties(gspl_studio_qt PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

  foreach(target gspl_studio_core gspl_studio_qt)
    if(MSVC)
      target_compile_options(${target} PRIVATE /W4 /WX /permissive- /EHsc)
    else()
      target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
  endforeach()

  qt_add_executable(gspl_sprites_studio
    ${CMAKE_SOURCE_DIR}/src/studio/shell/main.cpp
  )
  target_link_libraries(gspl_sprites_studio PRIVATE gspl_studio_qt)
  target_compile_features(gspl_sprites_studio PRIVATE cxx_std_23)

  qt_add_qml_module(gspl_sprites_studio
    URI GSPL.Studio
    VERSION 1.0
    RESOURCE_PREFIX "/"
    QML_FILES
      shell/MainWindow.qml
      shell/CommandPalette.qml
      shell/StartupWizard.qml
      shell/PreferencesDialog.qml
      shell/AboutDialog.qml
      shell/LogViewer.qml
      shell/HealthDashboard.qml
      shell/ThemeSettings.qml
      text/TextEditor.qml
      visual/GeneEditor.qml
      visual/MorphologyEditor.qml
      visual/FormEditor.qml
      visual/AnimationEditor.qml
      visual/BehaviorEditor.qml
      visual/CombatEditor.qml
      graph/GraphEditor.qml
      preview/PreviewViewport.qml
      debugger/DebuggerPanel.qml
      replay/ReplayPanel.qml
      artifacts/ArtifactExplorer.qml
      providers/ProviderPanel.qml
      packages/PackagePanel.qml
      packages/PackageCreationWizard.qml
      publishing/PublishWizard.qml
      targets/TargetPanel.qml
      git/GitPanel.qml
      plugins/PluginPanel.qml
  )

  if(BUILD_TESTING)
    add_executable(gspl_studio_project_tests
      ${CMAKE_SOURCE_DIR}/tests/studio/project_tests.cpp
    )
    target_link_libraries(gspl_studio_project_tests PRIVATE gspl_studio_core)
    add_test(NAME gspl_studio_project_tests COMMAND gspl_studio_project_tests)

    add_executable(gspl_studio_document_tests
      ${CMAKE_SOURCE_DIR}/tests/studio/document_tests.cpp
    )
    target_link_libraries(gspl_studio_document_tests PRIVATE gspl_studio_core)
    add_test(NAME gspl_studio_document_tests COMMAND gspl_studio_document_tests)

    add_executable(gspl_studio_application_controller_qt_tests
      ${CMAKE_SOURCE_DIR}/tests/studio/application_controller_qt_tests.cpp
    )
    target_link_libraries(gspl_studio_application_controller_qt_tests PRIVATE gspl_studio_qt)
    add_test(NAME gspl_studio_application_controller_qt_tests COMMAND gspl_studio_application_controller_qt_tests)

    add_test(NAME gspl_studio_headless_startup
      COMMAND gspl_sprites_studio --headless-test --safe-mode --disable-plugins --disable-providers --reset-layout)
    set_tests_properties(gspl_studio_headless_startup PROPERTIES
      ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  endif()
endif()
