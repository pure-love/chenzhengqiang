base_dir := ../..
TARGET_TYPE := SHARED_LIBRARY
include $(base_dir)/scripts/inc.Target

TARGET = librmflvdemux.$(lib_ext)

VPATH    = ./
SRC     = $(wildcard $(foreach EVERY_VPATH,$(VPATH),$(EVERY_VPATH)/*.c $(EVERY_VPATH)/*.cpp))


RMCFLAGS    += -D_GNU_SOURCE -I../include
USE_STD_LIB := 1

include $(base_dir)/scripts/inc.Makefile
