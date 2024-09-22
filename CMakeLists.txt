cmake_minimum_required(VERSION 3.16)

project(
  wineman
  VERSION 1.0.0
  LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SETUP_COMPILE_COMMANDS "Automatically set up compile_commands.json" ON)
if(SETUP_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
  execute_process(
    COMMAND
      ${CMAKE_COMMAND} -E create_symlink
      ${CMAKE_BINARY_DIR}/compile_commands.json
      ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json)
endif()

option(USE_CLANG_TIDY "Whether or not to use Clang Tidy during the build" OFF)
if(USE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY clang-tidy;
                           -header-filter=${CMAKE_CURRENT_SOURCE_DIR};)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(
  wineman
  src/main.cpp
  src/maindialog.cpp
  src/maindialog.h
  src/winemanager.cpp
  src/winemanager.h
  src/winemonitor.cpp
  src/winemonitor.h
  src/winemonitor_linux.cpp
  src/winemonitor_linux.h
  src/wineserverlist.cpp
  src/wineserverlist.h)

target_link_libraries(wineman PRIVATE Qt6::Widgets Qt6::DBus)

install(TARGETS wineman DESTINATION ${CMAKE_INSTALL_BINDIR})
