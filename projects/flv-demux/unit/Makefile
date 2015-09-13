#author:chenzhengqiang
#generate date:20150912-21:56:16


INCLUDE_DIR:=../include/
SOURCE_DIR:=../src/

SUFFIX:=cpp
vpath %.h $(INCLUDE_DIR)
vpath %.$(SUFFIX) $(SOURCE_DIR)

TARGET:=flv_demux_2_ts
CC:=g++
CFLAGS:=-g -W -Wall -I$(INCLUDE_DIR) 
OBJS:=ts_muxer ts mycrc_32 my_bs h264 flv_script flv_demux flv flv_avc flv_aac crc aac test_flv_demux_2_ts
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

INSTALL_DIR:=/usr/local/bin
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
test:
	./$(TARGET)
tar:
	tar -cvzf $(TAR_NAME).tar.gz .
