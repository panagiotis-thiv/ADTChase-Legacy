cmake_minimum_required(VERSION 3.26)
project(ADTChase C)

set(CMAKE_C_STANDARD 11)

#Find Raylib using vcpkg
find_package(raylib CONFIG REQUIRED)

#Build lib
file(GLOB K08_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/modules/*.c)
add_library(k08 STATIC ${K08_SOURCES})

# Headers that the lib exposes to others
target_include_directories(k08 PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)	

add_executable(ADTChase
src/game.c
src/state.c
src/level.c
src/menu.c
src/interface.c
src/boss_fight.c)

#Link headers
target_include_directories(ADTChase PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/src
)

#Link Raylib
target_link_libraries(ADTChase PRIVATE k08 raylib)

//Copies assets folder next to the exe after build
add_custom_command(TARGET ADTChase POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets
          $<TARGET_FILE_DIR:ADTChase>/assets
)

# Make ADTChase the startup project in the generated .sln
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ADTChase)