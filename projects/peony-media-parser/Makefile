#author:chenzhengqiang
#generate date:2016/02/05 12:54:36


INCLUDE_DIR:=./include
SOURCE_DIR:=./src

SUFFIX:=cpp
vpath %.h $(INCLUDE_DIR)
vpath %.$(SUFFIX) $(SOURCE_DIR)

TARGET:=peonyMediaParser
CC:=g++
#define the optimize level of compiler
OLEVEL=0
LDCONFIG:=
COMPILER_FLAGS=-pg -g -W -Wall -Wextra -Wshadow
CFLAGS:=-O$(OLEVEL)  -I$(INCLUDE_DIR) $(COMPILER_FLAGS) $(LDCONFIG)
OBJS:=main ts peonymediaparser mp4 
OBJS:=$(foreach obj,$(OBJS),$(obj).o)

INSTALL_DIR:=/usr/local/bin
CONFIG_PATH:=
SERVICE:=
CONFIG_INSTALL_PATH:=
TAR_NAME=$(TARGET)-$(shell date +%Y%m%d)

.PHONEY:clean
.PHONEY:install
.PHONEY:test
.PHONEY:tar

all:$(TARGET)
$(TARGET):$(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)
$(OBJS):%.o:%.$(SUFFIX)
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	-rm -f *.o *.a *.so *.log *core* $(TARGET) *.tar.gz *.cppe *.out

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
