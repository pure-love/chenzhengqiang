#author:chenzhengqiang
#generate date:20151225-15:20:08


INCLUDE_DIR:=./include/
SOURCE_DIR:=./src/

SUFFIX:=cpp
vpath %.h $(INCLUDE_DIR)
vpath %.$(SUFFIX) $(SOURCE_DIR)

TARGET:=xtrartmp
CC:=g++
CFLAGS:=-O0 -g -W -Wall -Werror -Wshadow
LDCONFIG:=-I$(INCLUDE_DIR) -lev -lpthread -lev
OBJS:=xtrartmp serverutil netutil main
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

INSTALL_DIR:=/usr/local/bin
CONFIG_PATH:=./config
SERVICE:=./scripts/xtrartmp
CONFIG_INSTALL_PATH:=/etc/xtrartmp
TAR_NAME=$(TARGET)-$(shell date '+%Y%m%d')

.PHONEY=clean
.PHONEY=install
.PHONEY=test
.PHONEY=tar

all:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDCONFIG)
$(OBJS):%.o:%.$(SUFFIX)
	$(CC) -o $@ -c $< $(CFLAGS) $(LDCONFIG)

clean:
	-rm -f *.o *.a *.so *.log *core* $(TARGET) *.tar.gz *.cppe

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
