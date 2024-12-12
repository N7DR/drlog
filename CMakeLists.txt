cmake_minimum_required(VERSION 3.0)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project(drlog)

#set (CMAKE_CXX_COMPILER "g++")
set (CMAKE_CXX_COMPILER "/usr/local/programs/gcc-14.2.0/bin/g++-14")

set ( SOURCE_FILES
                   src/drlog.cpp
                   src/adif3.cpp
                   src/audio.cpp
                   src/autocorrect.cpp
                   src/bandmap.cpp
                   src/bands-modes.cpp
                   src/cabrillo.cpp
                   src/cluster.cpp
                   src/command_line.cpp
                   src/cty_data.cpp
                   src/cw_buffer.cpp
                   src/diskfile.cpp
                   src/drlog_context.cpp
                   src/drlog_error.cpp
                   src/drmaster.cpp
                   src/exchange.cpp
                   src/exchange_field_template.cpp
                   src/functions.cpp
                   src/fuzzy.cpp
                   src/grid.cpp
                   src/keyboard.cpp
                   src/log.cpp
                   src/log_message.cpp
                   src/memory.cpp
                   src/multiplier.cpp
                   src/parallel_port.cpp
                   src/procfs.cpp
                   src/pthread_support.cpp
                   src/qso.cpp
                   src/qtc.cpp
                   src/query.cpp
                   src/rate.cpp
                   src/rig_interface.cpp
                   src/rules.cpp
                   src/scp.cpp
                   src/screen.cpp
                   src/socket_support.cpp
                   src/statistics.cpp
                   src/string_functions.cpp
                   src/textfile.cpp
                   src/trlog.cpp
                   src/version.cpp
                   src/x_error.cpp
    )

set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall -Wno-reorder -pipe -std=c++2b" )

add_executable(drlog ${SOURCE_FILES})

target_include_directories (drlog PUBLIC include)
target_link_libraries (drlog X11 pthread boost_serialization asound ncursesw png hamlib panel ieee1284)

add_custom_command(
  TARGET drlog
  PRE_BUILD
  COMMAND touch
  ARGS "/home/n7dr/projects/drlog/include/version.h"
  )

 set( CMAKE_VERBOSE_MAKEFILE on )

#install(TARGETS drlog RUNTIME DESTINATION bin)
