#@Author:chenzhengqiang
#@Company:swwy
#@Date: 2015/6/23
#@Version: v1.0
#@Desc

SUFFIX:=cpp
vpath %.h include/
vpath %.$(SUFFIX) src/

TARGET:=go
CC:=g++
OBJS:=main transfer netutility logging rtp aes opus ortp_send

CFLAGS:=-g -lpthread -lcrypto -Wall -lopus -lortp -I/usr/include -L/usr/lib
INCLUDE_DIR:=./include/
INSTALL_DIR:=/usr/local/bin
CONFIG_FILE:=./config/server.conf
CONFIG_INSTALL_PATH:=/etc/transfer
SERVICE:=./scripts/transfer
TAR_NAME=$(TARGET)-$(shell date '+%Y%m%d')

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
test:
	./$(TARGET) -f ./config/server.conf
tar:
	tar -cvzf $(TAR_NAME).tar.gz .  
