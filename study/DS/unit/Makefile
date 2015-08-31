#author:chenzhengqiang
#start date:2015/8/2

PROG_SUFFIX:=c
INCLUDE_DIR:=../include/
SOURCE_DIR:=../src/

CC:=gcc
CFLAGS:=-W -Wall -g -I$(INCLUDE_DIR)

vpath %.h $(INCLUDE_DIR)
vpath %.$(PROG_SUFFIX) $(SOURCE_DIR)

TARGET=go
OBJS:=test_selection_problem selection_problem
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
$(OBJS):%.o:%.$(PROG_SUFFIX)
	$(CC) -o $@ -c $< $(CFLAGS)

install:
	-mv $(TARGET) $(INSTALL_DIR)
clean:
	del *.o *~
