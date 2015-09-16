#author:chenzhengqiang
#generate date:20150915-22:57:10


INCLUDE_DIR:=../include/
SOURCE_DIR:=../src/

SUFFIX:=cpp
vpath %.h $(INCLUDE_DIR)
vpath %.$(SUFFIX) $(SOURCE_DIR)

TARGET:=flv_demux_2_h264_aac
CC:=g++
CFLAGS:=-g -W -Wall -I$(INCLUDE_DIR) 
OBJS:=flv_script flv_demux flv flv_avc flv_aac test_flv_demux_264_aac
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
