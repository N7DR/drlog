# makefile for drlog

CC = ccache g++
CPP = /usr/local/bin/cpp
LIB = ar
ASM = as

# The places to look for include files (in order).
INCL =  -I./include -I./include/hamlib 

# LIBS = -L

LIBRARIES = -lpthread -lasound -lpng -lboost_regex -lboost_serialization -lX11 -lpanel -lhamlib -lieee1284 -lncursesw

#D_REENTRANT -DLINUX -D_FILE_OFFSET_BITS=64 -I"/home/n7dr/projects/drlog/include" -I"/home/n7dr/projects/drlog/include/#hamlib" -O0 -g3 -Wall -c -fmessage-length=0 -Wno-reorder -std=c++17 `libpng-config --cflags`

# Extra Defs

# utility routines
DEL = rm
COPY = cp

# name of main executable to build
PROG = all

# -O works
# -O1 works
# -O2 works
CFLAGS = $(INCL) -D_REENTRANT -c -g3 -O0 -pipe -DLINUX -D_FILE_OFFSET_BITS=64 -fmessage-length=0 -Wno-reorder -std=c++17 `libpng-config --cflags`

LINKFLAGS = $(LIBRARIES)

include/adif.h : include/macros.h
	touch include/adif.h
	
include/audio.h : include/macros.h include/string_functions.h include/x_error.h
	touch include/audio.h
	
include/bandmap.h : include/cluster.h include/drlog_context.h include/log.h include/pthread_support.h include/rules.h \
                    include/screen.h include/serialization.h include/statistics.h
	touch include/bandmap.h
	
include/bands-modes.h : include/string_functions.h
	touch include/bands-modes.h

# cabrillo.h has no dependencies

include/cluster.h : include/drlog_context.h include/macros.h include/socket_support.h
	touch include/cluster.h
	
# command_line.h has no dependencies

include/cty_data.h : include/macros.h include/pthread_support.h include/serialization.h include/x_error.h
	touch include/cty_data.h
	
include/cw_buffer.h : include/parallel_port.h include/pthread_support.h include/rig_interface.h
	touch include/cw_buffer.h
	
# diskfile.h has no dependencies

include/drlog_context.h : include/bands-modes.h include/cty_data.h include/screen.h
	touch include/drlog_context.h
	
# drlog-error.h has no dependencies

include/drmaster.h : include/macros.h include/string_functions.h
	touch include/drmaster.h
	
include/exchange.h : include/macros.h include/pthread_support.h include/rules.h
	touch include/exchange.h
	
# functions.h has no dependencies

include/fuzzy.h : include/drmaster.h
	touch include/fuzzy.h
	
include/grid.h : include/functions.h include/macros.h include/serialization.h
	touch include/grid.h
	
include/keyboard.h : include/pthread_support.h include/macros.h include/string_functions.h
	touch include/keyboard.h
	
include/log.h : include/cty_data.h include/drlog_context.h include/log_message.h include/pthread_support.h include/qso.h \
                include/rules.h include/serialization.h
	touch include/log.h
	
include/log_message.h : include/pthread_support.h
	touch include/log_message.h
	
include/macros.h : include/serialization.h
	touch include/macros.h
	
include/memory.h : include/macros.h
	
include/multiplier.h : include/bands-modes.h include/log_message.h include/macros.h include/pthread_support.h include/serialization.h
	touch include/multiplier.h
	
include/parallel_port.h : include/macros.h include/x_error.h
	touch include/parallel_port.h
	
include/procfs.h : include/string_functions.h

include/pthread_support.h : include/macros.h include/x_error.h
	touch include/pthread_support.h
	
include/qso.h : include/drlog_context.h include/macros.h include/rules.h
	touch include/qso.h
	
include/qtc.h : include/log.h include/macros.h include/qso.h include/serialization.h include/x_error.h
	touch include/qtc.h
	
include/rate.h : include/log_message.h include/pthread_support.h
	touch include/rate.h
	
include/rig_interface.h : include/bands-modes.h include/drlog_context.h include/macros.h include/pthread_support.h include/x_error.h
	touch include/rig_interface.h
	
include/rules.h : include/bands-modes.h	include/cty_data.h include/drlog_context.h include/grid.h include/macros.h \
                  include/pthread_support.h include/serialization.h
	touch include/rules.h
	
include/scp.h : include/drmaster.h
	touch include/scp.h
	
include/screen.h : include/keyboard.h include/log_message.h include/macros.h include/pthread_support.h include/string_functions.h
	touch include/screen.h
	
# serialization.h has no dependencies

include/socket_support.h : include/drlog_error.h include/macros.h include/pthread_support.h
	touch include/socket_support.h
	
include/statistics.h : include/cty_data.h include/drlog_context.h include/log.h include/multiplier.h include/pthread_support.h \
                       include/qso.h include/rules.h include/serialization.h
	touch include//statistics.h

include/string_functions.h : include/macros.h include/x_error.h
	touch include/string_functions.h
	
include/trlog.h : include/bands-modes.h
	touch include/trlog.h
	
# version.h has no dependencies

# x_error.h has no dependencies

src/adif.cpp : include/adif.h include/string_functions.h
	touch src/adif.cpp
	
src/audio.cpp : include/audio.h include/log_message.h include/string_functions.h
	touch src/audio.cpp
	
src/bandmap.cpp : include/bandmap.h include/exchange.h include/log_message.h include/statistics.h include/string_functions.h
	touch src/bandmap.cpp
	
src/bands-modes.cpp : include/bands-modes.h
	touch src/bands-modes.cpp
	
src/cabrillo.cpp : include/cabrillo.h include/macros.h include/string_functions.h
	touch src/cabrillo.cpp
	
src/cluster.cpp : include/cluster.h include/pthread_support.h include/string_functions.h
	touch src/cluster.cpp
	
src/command_line.cpp : include/command_line.h include/string_functions.h
	touch src/command_line.cpp
	
src/cty_data.cpp : include/cty_data.h include/drlog_context.h include/string_functions.h
	touch src/cty_data.cpp
	
src/cw_buffer.cpp : include/cw_buffer.h include/log_message.h
	touch src/cw_buffer.cpp
	
src/diskfile.cpp : include/diskfile.h
	touch src/diskfile.cpp
	
src/drlog.cpp : include/audio.h include/bandmap.h include/bands-modes.h include/cluster.h include/command_line.h \
                include/cty_data.h include/cw_buffer.h include/diskfile.h include/drlog_context.h include/exchange.h \
                include/functions.h include/fuzzy.h include/grid.h include/keyboard.h include/log.h \
                include/memory.h \
                include/log_message.h include/parallel_port.h include/procfs.h include/qso.h include/qtc.h include/rate.h \
                include/rig_interface.h include/rules.h include/scp.h include/screen.h include/serialization.h \
                include/socket_support.h include/statistics.h include/string_functions.h include/trlog.h include/version.h
	touch src/drlog.cpp
	
src/drlog_context.cpp : include/diskfile.h include/drlog_context.h include/exchange.h include/string_functions.h
	touch src/drlog_context.cpp
	
src/drlog_error.cpp : include/drlog_error.h
	touch src/drlog_error.cpp
	
src/drmaster.cpp : include/drmaster.h
	touch src/drmaster.cpp
	
src/exchange.cpp : include/cty_data.h include/diskfile.h include/drmaster.h include/exchange.h include/log.h \
                   include/string_functions.h
	touch src/exchange.cpp
	
src/functions.cpp : include/functions.h include/log_message.h include/string_functions.h
	touch src/functions.cpp
	
src/fuzzy.cpp : include/fuzzy.h	include/log_message.h	include/string_functions.h
	touch src/fuzzy.cpp
	
src/grid.cpp : include/grid.h include/log_message.h include/string_functions.h
	touch src/grid.cpp
	
src/keyboard.cpp : include/keyboard.h include/log_message.h include/string_functions.h
	touch src/keyboard.cpp
	
src/log.cpp : include/cabrillo.h include/log.h include/statistics.h include/string_functions.h
	touch src/log.cpp
	
src/log_message.cpp : include/diskfile.h include/log_message.h
	touch src/log_message.cpp
	
src/memory.cpp : include/memory.h

src/multiplier.cpp : include/multiplier.h
	touch src/multiplier.cpp
	
src/parallel_port.cpp : include/log_message.h include/parallel_port.h
	touch src/parallel_port.cpp
	
src/procfs.cpp : include/procfs.h
	
src/pthread_support.cpp : include/log_message.h include/pthread_support.h include/string_functions.h
	touch src/pthread_support.cpp
	
src/qso.cpp : include/bands-modes.h include/cty_data.h include/exchange.h include/qso.h include/statistics.h \
              include/string_functions.h
	touch src/qso.cpp
	
src/qtc.cpp : include/diskfile.h include/qtc.h
	touch src/qtc.cpp
	
src/rate.cpp : include/rate.h include/string_functions.h
	touch include/rate.cpp
	
src/rig_interface.cpp : include/bands-modes.h include/drlog_context.h include/rig_interface.h include/string_functions.h
	touch include/rig_interface.cpp
	
src/rules.cpp : include/exchange.h include/macros.h include/qso.h include/rules.h include/string_functions.h
	touch src/rules.cpp
	
src/scp.cpp : include/log_message.h include/scp.h include/string_functions.h
	touch src/scp.cpp
	
src/screen.cpp : include/screen.h include/string_functions.h
	touch src/screen.cpp
	
src/socket_support.cpp : include/log_message.h include/pthread_support.h include/socket_support.h include/string_functions.h
	touch src/socket_support.cpp
	
src/statistics.cpp : include/bands-modes.h include/log_message.h include/rules.h include/statistics.h include/string_functions.h
	touch src/statistics.cpp
	
src/string_functions.cpp : include/macros.h include/string_functions.h
	touch src/string_functions.cpp
	
src/trlog.cpp : include/string_functions.h include/trlog.h
	touch src/trlog.cpp
	
src/version.cpp : include/version.h
	touch version.cpp
	
src/x_error.cpp : include/x_error.h
	touch src/x_error.cpp
	
bin/adif.o : src/adif.cpp
	$(CC) $(CFLAGS) -o $@ src/adif.cpp

bin/audio.o : src/audio.cpp
	$(CC) $(CFLAGS) -o $@ src/audio.cpp

bin/bandmap.o : src/bandmap.cpp
	$(CC) $(CFLAGS) -o $@ src/bandmap.cpp

bin/bands-modes.o : src/bands-modes.cpp
	$(CC) $(CFLAGS) -o $@ src/bands-modes.cpp

bin/cabrillo.o : src/cabrillo.cpp
	$(CC) $(CFLAGS) -o $@ src/cabrillo.cpp

bin/cluster.o : src/cluster.cpp
	$(CC) $(CFLAGS) -o $@ src/cluster.cpp

bin/command_line.o : src/command_line.cpp
	$(CC) $(CFLAGS) -o $@ src/command_line.cpp

bin/cty_data.o : src/cty_data.cpp
	$(CC) $(CFLAGS) -o $@ src/cty_data.cpp

bin/cw_buffer.o : src/cw_buffer.cpp
	$(CC) $(CFLAGS) -o $@ src/cw_buffer.cpp

bin/diskfile.o : src/diskfile.cpp
	$(CC) $(CFLAGS) -o $@ src/diskfile.cpp

bin/drlog.o : src/drlog.cpp
	$(CC) $(CFLAGS) -o $@ src/drlog.cpp

bin/drlog_context.o : src/drlog_context.cpp
	$(CC) $(CFLAGS) -o $@ src/drlog_context.cpp

bin/drlog_error.o : src/drlog_error.cpp
	$(CC) $(CFLAGS) -o $@ src/drlog_error.cpp

bin/drmaster.o : src/drmaster.cpp
	$(CC) $(CFLAGS) -o $@ src/drmaster.cpp

bin/exchange.o : src/exchange.cpp
	$(CC) $(CFLAGS) -o $@ src/exchange.cpp

bin/functions.o : src/functions.cpp
	$(CC) $(CFLAGS) -o $@ src/functions.cpp

bin/fuzzy.o : src/fuzzy.cpp
	$(CC) $(CFLAGS) -o $@ src/fuzzy.cpp

bin/grid.o : src/grid.cpp
	$(CC) $(CFLAGS) -o $@ src/grid.cpp

bin/keyboard.o : src/keyboard.cpp
	$(CC) $(CFLAGS) -o $@ src/keyboard.cpp

bin/log.o : src/log.cpp
	$(CC) $(CFLAGS) -o $@ src/log.cpp

bin/log_message.o : src/log_message.cpp
	$(CC) $(CFLAGS) -o $@ src/log_message.cpp

bin/memory.o : src/memory.cpp
	$(CC) $(CFLAGS) -o $@ src/memory.cpp

bin/multiplier.o : src/multiplier.cpp
	$(CC) $(CFLAGS) -o $@ src/multiplier.cpp

bin/parallel_port.o : src/parallel_port.cpp
	$(CC) $(CFLAGS) -o $@ src/parallel_port.cpp

bin/procfs.o : src/procfs.cpp
	$(CC) $(CFLAGS) -o $@ src/procfs.cpp

bin/pthread_support.o : src/pthread_support.cpp
	$(CC) $(CFLAGS) -o $@ src/pthread_support.cpp

bin/qso.o : src/qso.cpp
	$(CC) $(CFLAGS) -o $@ src/qso.cpp

bin/qtc.o : src/qtc.cpp
	$(CC) $(CFLAGS) -o $@ src/qtc.cpp

bin/rate.o : src/rate.cpp
	$(CC) $(CFLAGS) -o $@ src/rate.cpp

bin/rig_interface.o : src/rig_interface.cpp
	$(CC) $(CFLAGS) -o $@ src/rig_interface.cpp

bin/rules.o : src/rules.cpp
	$(CC) $(CFLAGS) -o $@ src/rules.cpp

bin/scp.o : src/scp.cpp
	$(CC) $(CFLAGS) -o $@ src/scp.cpp

bin/screen.o : src/screen.cpp
	$(CC) $(CFLAGS) -o $@ src/screen.cpp

bin/socket_support.o : src/socket_support.cpp
	$(CC) $(CFLAGS) -o $@ src/socket_support.cpp

bin/statistics.o : src/statistics.cpp
	$(CC) $(CFLAGS) -o $@ src/statistics.cpp

bin/string_functions.o : src/string_functions.cpp
	$(CC) $(CFLAGS) -o $@ src/string_functions.cpp

bin/trlog.o : src/trlog.cpp
	$(CC) $(CFLAGS) -o $@ src/trlog.cpp

bin/version.o : src/version.cpp FORCE
	$(CC) $(CFLAGS) -o $@ src/version.cpp

bin/x_error.o : src/x_error.cpp
	$(CC) $(CFLAGS) -o $@ src/x_error.cpp

bin/drlog : bin/adif.o bin/audio.o bin/bandmap.o bin/bands-modes.o bin/cabrillo.o \
            bin/cluster.o bin/command_line.o bin/cty_data.o bin/cw_buffer.o bin/diskfile.o \
            bin/drlog.o bin/drlog_context.o bin/drlog_error.o bin/drmaster.o bin/exchange.o \
            bin/functions.o bin/fuzzy.o bin/grid.o bin/keyboard.o bin/log.o \
            bin/log_message.o bin/memory.o bin/multiplier.o bin/parallel_port.o \
            bin/procfs.o bin/pthread_support.o bin/qso.o \
            bin/qtc.o bin/rate.o bin/rig_interface.o bin/rules.o bin/scp.o \
            bin/screen.o bin/socket_support.o bin/statistics.o bin/string_functions.o bin/trlog.o \
            bin/version.o bin/x_error.o
	$(CC) $(LINKFLAGS) bin/adif.o bin/audio.o bin/bandmap.o bin/bands-modes.o bin/cabrillo.o \
	bin/cluster.o bin/command_line.o bin/cty_data.o bin/cw_buffer.o bin/diskfile.o \
	bin/drlog.o bin/drlog_context.o bin/drlog_error.o bin/drmaster.o bin/exchange.o \
	bin/functions.o bin/fuzzy.o bin/grid.o bin/keyboard.o bin/log.o \
	bin/log_message.o bin/memory.o bin/multiplier.o bin/parallel_port.o \
	bin/procfs.o bin/pthread_support.o bin/qso.o \
	bin/qtc.o bin/rate.o bin/rig_interface.o bin/rules.o bin/scp.o \
	bin/screen.o bin/socket_support.o bin/statistics.o bin/string_functions.o bin/trlog.o \
	bin/version.o bin/x_error.o \
	-o bin/drlog
	
directory:
	@mkdir -p bin
	
drlog : directory bin/drlog

# clean everything
clean :
	rm bin/*
	
FORCE:

