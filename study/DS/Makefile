#author:chenzhengqiang
#start date:2015/8/2

PROG_SUFFIX:=cpp
CC:=g++
CFLAGS=-g -W -Wall

INCLUDE_DIR:=./include/
SOURCE_DIR:=../src/
vpath %.h $(INCLUDE_DIR)
vpath %.$(PROG_SUFFIX) $(SOURCE_DIR)

INSTALL_DIR:=/usr/local/bin

TARGET=dgo
OBJS:=main
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
$(OBJS):%.o:%.$(PROG_SUFFIX)
	$(CC) -o $@ -c $<

install:
	-mv $(TARGET) $(INSTALL_DIR)
clean:
	-rm -rf *.o 
