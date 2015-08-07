#@Author:chenzhengqiang
#@Company:swwy
#@Date: 2015/3/26
#@Modified: 2015/3/27
#@Version: v1.0 v1.1
#@Desc

SUFFIX:=cpp
vpath %.h include/
vpath %.$(SUFFIX) src/

TARGET:=streamer
CC:=g++
OBJS:=main streamer streamerutility logging  
OBJS+=system_info state_server parson

CFLAGS:=-g -lev -lpthread -W -Wall
INCLUDE_DIR:=./include/
INSTALL_DIR:=/usr/local/bin
CONFIG_FILE:=./config/streamer.conf
NOTIFY_FILE:=./config/notify_server.conf
CONFIG_INSTALL_PATH:=/etc/streamer
SERVICE:=./scripts/streamer
TAR_NAME=streamer-$(shell date '+%Y%m%d')

OBJS:=$(foreach obj,$(OBJS),$(obj).o)

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
	-rm -f *.o *.a *.so *.log *core* $(TARGET) *.tar.gz
install:
	-mv $(TARGET) $(INSTALL_DIR)
	-cp -f $(SERVICE) /etc/init.d/$(TARGET)
	-rm -rf $(CONFIG_INSTALL_PATH)
	-mkdir $(CONFIG_INSTALL_PATH)
	-cp -f $(CONFIG_FILE) $(CONFIG_INSTALL_PATH)
	-cp -f $(NOTIFY_FILE) $(CONFIG_INSTALL_PATH)
test:
	./$(TARGET) -f ./config/streamer.conf
tar:
	tar -cvzf $(TAR_NAME).tar.gz .  
