add_executable(
  ${PROJECT_NAME}
  main.cpp
)

target_link_libraries(
  ${PROJECT_NAME} 
  minimal-audio-engine::framework
  minimal-audio-engine::trackmanager
  minimal-audio-engine::devicemanager
  minimal-audio-engine::filemanager
  minimal-audio-engine::coreengine
)