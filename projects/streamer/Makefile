#author:chenzhengqiang
#generate date:20150913-23:36:03


INCLUDE_DIR:=./include/
SOURCE_DIR:=./src/

SUFFIX:=cpp
vpath %.h $(INCLUDE_DIR)
vpath %.$(SUFFIX) $(SOURCE_DIR)

TARGET:=streamer
CC:=g++
CFLAGS:=-g -W -Wall -I$(INCLUDE_DIR) -lev -lpthread
OBJS:=system_info streamerutility streamer state_server parson logging main
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

INSTALL_DIR:=/usr/local/bin
CONFIG_PATH:=./config
SERVICE:=./scripts/streamer
CONFIG_INSTALL_PATH:=/etc/streamer
TAR_NAME=$(TARGET)-$(shell date '+%Y%m%d')
.PHONEY=clean
.PHONEY=install
.PHONEY=test
.PHONEY=tar
all:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
$(OBJS):%.o:%.$(SUFFIX)
	$(CC) -o $@ -c $< -I$(INCLUDE_DIR) $(CFLAGS)

clean:
	-rm -f *.o *.a *.so *.log *core* $(TARGET) *.tar.gz *.cppe

install:
	-mv $(TARGET) $(INSTALL_DIR)
	-cp -f $(SERVICE) /etc/init.d/$(TARGET)
	-rm -rf $(CONFIG_INSTALL_PATH)
	-mkdir $(CONFIG_INSTALL_PATH)
	-cp -f $(CONFIG_PATH)/* $(CONFIG_INSTALL_PATH)

test:
	./$(TARGET)
tar:
	tar -cvzf $(TAR_NAME).tar.gz .
