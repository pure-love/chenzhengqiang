#author:chenzhengqiang
#generate date:20150925-17:55:22


INCLUDE_DIR:=./include/
SOURCE_DIR:=./src/

SUFFIX:=cpp
vpath %.h $(INCLUDE_DIR)
vpath %.$(SUFFIX) $(SOURCE_DIR)

TARGET:=happy_hls
CC:=g++
CFLAGS:=-g -W -Wall -I$(INCLUDE_DIR) -lcrypto -lpthread -lev
OBJS:=ts_muxer ts netutility mycrc_32 my_bs m3u8 logging happy_hls h264 flv_script flv flv_avc flv_aac crc bits_io aac main
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

INSTALL_DIR:=/usr/local/bin
CONFIG_PATH:=./config
SERVICE:=./scripts/happy_hls
CONFIG_INSTALL_PATH:=/etc/happy_hls
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
