#author:chenzhengqiang
#generate date:20151124-10:18:26


INCLUDE_DIR:=./
SOURCE_DIR:=./

SUFFIX:=c
vpath %.h $(INCLUDE_DIR)
vpath %.$(SUFFIX) $(SOURCE_DIR)

DEP_INCLUDE_DIR:=/usr/local/ffmpeg/include
DEP_LIB_DIR:=/usr/local/ffmpeg/lib
TARGET:=flv2ts
CC:=gcc
CFLAGS:=-I$(INCLUDE_DIR) -I$(DEP_INCLUDE_DIR) -L$(DEP_LIB_DIR)
LDCONFIG:=-lavcodec -lavformat -lavutil -lswscale -lpthread -lm -lz

OBJS:=flv2ts
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

INSTALL_DIR:=/usr/local/bin
CONFIG_PATH:=
SERVICE:=
CONFIG_INSTALL_PATH:=
TAR_NAME=$(TARGET)-$(shell date '+%Y%m%d')

.PHONEY:clean
.PHONEY:install
.PHONEY:test
.PHONEY:tar

all:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDCONFIG)
$(OBJS):%.o:%.$(SUFFIX)
	$(CC) -o $@ -c $< $(CFLAGS) $(LDCONFIG)

clean:
	-rm -f *.o *.a *.so *core* $(TARGET) *.tar.gz *.cppe

install:
	-rm -f $(INSTALL_DIR)/$(TARGET)
	-mv $(TARGET) $(INSTALL_DIR)
	-cp -f $(SERVICE) /etc/init.d/$(TARGET)
	-rm -rf $(CONFIG_INSTALL_PATH)
	-mkdir $(CONFIG_INSTALL_PATH)
	-cp -f $(CONFIG_PATH)/* $(CONFIG_INSTALL_PATH)

test:
	./$(TARGET)
tar:
	tar -cvzf $(TAR_NAME).tar.gz .
