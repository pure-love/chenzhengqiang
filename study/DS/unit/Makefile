#author:chenzhengqiang
#start date:2015/8/2

PROG_SUFFIX:=cpp
INCLUDE_DIR:=../include/
SOURCE_DIR:=../src/

CC:=g++
CFLAGS:=-W -Wall -g -I$(INCLUDE_DIR)

vpath %.h $(INCLUDE_DIR)
vpath %.$(PROG_SUFFIX) $(SOURCE_DIR)

TARGET=go
OBJS:=test_sorts
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
$(OBJS):%.o:%.$(PROG_SUFFIX)
	$(CC) -o $@ -c $< $(CFLAGS)

install:
	-mv $(TARGET) $(INSTALL_DIR)
clean:
	-rm -rf *.o $(TARGET) *.tmp *.cppe *.gz *.bz
