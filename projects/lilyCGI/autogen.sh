#file name:autogen.sh
#author:chenzhengqiang
#start date:2016/01/15 09:44:31
#modified date:
#desc:auto generate the Makefile
#!/bin/bash
#########global configuration#######
TARGET=lilyCGI
MAIN_FILE=main
AUTHOR=chenzhengqiang
DATE=`date '+%Y/%m/%d %H:%M:%S'`
COMPILER=g++
COMPILER_FLAGS="-pg -g -W -Wall -Werror -Wshadow -Wconversion -Wextra -Wunused-parameter -Wdeprecated"
#define the optimize level
OLEVEL=0
MAKEFILE=./Makefile
LDCONFIG=
SOURCE_DIR=./src
INCLUDE_DIR=./include
INSTALL_DIR=/usr/local/bin
#you didn't have to configure this
CONFIG_PATH=./config
CONFIG_INSTALL_PATH=/etc/$TARGET
SERVICE=./scripts/$TARGET
#########global configuration#######
`rm -rf $MAKEFILE`
`touch $MAKEFILE`
echo "#author:$AUTHOR" >> $MAKEFILE
echo "#generate date:$DATE" >> $MAKEFILE
echo >> $MAKEFILE
echo >> $MAKEFILE
echo "INCLUDE_DIR:=$INCLUDE_DIR" >> $MAKEFILE
echo "SOURCE_DIR:=$SOURCE_DIR" >> $MAKEFILE
echo >> $MAKEFILE
if [ -z "$COMPILER" ];then
    echo 'SUFFIX:=cpp' >> $MAKEFILE
elif [ "$COMPILER" == "g++" ];then
    echo 'SUFFIX:=cpp' >> $MAKEFILE
elif [ "$COMPILER" == "gcc" ];then
    echo 'SUFFIX:=c' >> $MAKEFILE
else
    echo plese check the autogen\'s configuration
exit 99
fi
echo "vpath %.h \$(INCLUDE_DIR)" >> $MAKEFILE
echo "vpath %.\$(SUFFIX) \$(SOURCE_DIR)" >> $MAKEFILE
echo >> $MAKEFILE
echo "TARGET:=$TARGET" >> $MAKEFILE
echo "CC:=$COMPILER" >> $MAKEFILE
echo "#define the optimize level of compiler" >> $MAKEFILE
echo "OLEVEL=$OLEVEL" >> $MAKEFILE
echo "LDCONFIG:=$LDCONFIG" >> $MAKEFILE
echo "COMPILER_FLAGS=-pg -g -W -Wall -Wextra -Wconversion -Wshadow -Wunused-parameter" >> $MAKEFILE
echo "CFLAGS:=-O\$(OLEVEL)  -I\$(INCLUDE_DIR) \$(COMPILER_FLAGS) \$(LDCONFIG)" >> $MAKEFILE
for cpp_file in `ls $SOURCE_DIR`
do
    obj=${cpp_file%%.*}
    OBJS="$obj $OBJS"
done
echo "OBJS:=$MAIN_FILE $OBJS" >> $MAKEFILE
echo "OBJS:=\$(foreach obj,\$(OBJS),\$(obj).o)" >> $MAKEFILE
echo >> $MAKEFILE
echo "INSTALL_DIR:=$INSTALL_DIR" >> $MAKEFILE
echo "CONFIG_PATH:=$CONFIG_PATH" >> $MAKEFILE
echo "SERVICE:=$SERVICE" >> $MAKEFILE
echo "CONFIG_INSTALL_PATH:=$CONFIG_INSTALL_PATH" >> $MAKEFILE
echo "TAR_NAME=\$(TARGET)-\$(shell date "+%Y%m%d")" >> $MAKEFILE
echo >> $MAKEFILE
echo ".PHONEY:clean" >> $MAKEFILE
echo ".PHONEY:install" >> $MAKEFILE
echo ".PHONEY:test" >> $MAKEFILE
echo ".PHONEY:tar" >> $MAKEFILE
echo >> $MAKEFILE
echo "all:\$(TARGET)" >> $MAKEFILE
echo "\$(TARGET):\$(OBJS)" >> $MAKEFILE
echo -e "\t\$(CC) -o \$@ \$^ \$(CFLAGS)" >> $MAKEFILE
echo "\$(OBJS):%.o:%.\$(SUFFIX)" >> $MAKEFILE
echo -e "\t\$(CC) -o \$@ -c \$< \$(CFLAGS)" >> $MAKEFILE
echo >> $MAKEFILE
echo "clean:" >> $MAKEFILE
echo -e "\t-rm -f *.o *.a *.so *.log *core* \$(TARGET) *.tar.gz *.cppe" >> $MAKEFILE
echo >> $MAKEFILE
echo "install:" >> $MAKEFILE
echo -e "\t-mv \$(TARGET) \$(INSTALL_DIR)" >> $MAKEFILE
echo -e "\t-cp -f \$(SERVICE) /etc/init.d/\$(TARGET)" >> $MAKEFILE
echo -e "\t-rm -rf \$(CONFIG_INSTALL_PATH)" >> $MAKEFILE
echo -e "\t-mkdir \$(CONFIG_INSTALL_PATH)" >> $MAKEFILE
echo -e "\t-cp -f \$(CONFIG_PATH)/* \$(CONFIG_INSTALL_PATH)" >> $MAKEFILE
echo >> $MAKEFILE
echo "test:" >> $MAKEFILE
echo -e "\t./\$(TARGET)" >> $MAKEFILE
echo "tar:" >> $MAKEFILE
echo -e "\ttar -cvzf \$(TAR_NAME).tar.gz ." >> $MAKEFILE
