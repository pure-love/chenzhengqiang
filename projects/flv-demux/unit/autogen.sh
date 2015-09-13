#file name:autogen.sh
#author:chenzhengqiag
#start date:2015/9/13
#modified date:
#desc:auto generate the Makefile

#!/bin/bash

########global configuration#######
TARGET="flv_demux_2_ts"
OBJS="test_flv_demux_2_ts"
SOURCE_DIR="../src/"
INCLUDE_DIR="../include/"
MAKEFILE="./Makefile"
COMPILER="g++"
DEPS=
INSTALL_DIR=/usr/local/bin
AUTHOR=chenzhengqiang
DATE=`date '+%Y%m%d-%H:%M:%S'`
########global configuration#######


`rm -rf $MAKEFILE`
`touch $MAKEFILE`
echo "#author:$AUTHOR" >> $MAKEFILE
echo "#generate date:$DATE" >> $MAKEFILE
echo >> $MAKEFILE
echo >> $MAKEFILE

if [ $? -ne 0 ];then
	echo "failed to create $MAKEFILE,permission denied"
fi

echo "INCLUDE_DIR:=$INCLUDE_DIR" >> $MAKEFILE
echo "SOURCE_DIR:=$SOURCE_DIR" >> $MAKEFILE
echo >> $MAKEFILE

if [ -z $COMPILER ];then
	echo "SUFFIX:=cpp" >> $MAKEFILE
elif [ "$COMPILER" == "g++" ];then
	echo "SUFFIX:=cpp" >> $MAKEFILE
elif [ "$COMPILER" == "gcc" ];then
	echo "SUFFIX:=c" >> $MAKEFILE
else 
	echo "plese check the autogen's configuration"
	exit 99
fi
echo "vpath %.h \$(INCLUDE_DIR)" >> $MAKEFILE
echo "vpath %.\$(SUFFIX) \$(SOURCE_DIR)" >> $MAKEFILE
echo >> $MAKEFILE

echo "TARGET:=$TARGET" >> $MAKEFILE
echo "CC:=$COMPILER" >> $MAKEFILE
echo "CFLAGS:=-g -W -Wall -I\$(INCLUDE_DIR) $DEPS" >> $MAKEFILE

for cpp_file in `ls $SOURCE_DIR` 
do
	obj=${cpp_file%%.*}
	OBJS="$obj $OBJS"	
done

echo "OBJS:=$OBJS" >> $MAKEFILE
echo "OBJS:=\$(foreach obj,\$(OBJS),\$(obj).o)" >> $MAKEFILE
echo >> $MAKEFILE

echo "INSTALL_DIR:=$INSTALL_DIR" >> $MAKEFILE
echo "TAR_NAME=\$(TARGET)-\$(shell date '+%Y%m%d')" >> $MAKEFILE
echo ".PHONEY=clean" >> $MAKEFILE
echo ".PHONEY=install" >> $MAKEFILE
echo ".PHONEY=test" >> $MAKEFILE
echo ".PHONEY=tar" >> $MAKEFILE

echo "all:\$(TARGET)" >> $MAKEFILE
echo "\$(TARGET):\$(OBJS)" >> $MAKEFILE
echo -e "\t\$(CC) -o \$@ \$^ \$(CFLAGS)" >> $MAKEFILE
echo "\$(OBJS):%.o:%.\$(SUFFIX)" >> $MAKEFILE
echo -e "\t\$(CC) -o \$@ -c \$< -I\$(INCLUDE_DIR) \$(CFLAGS)" >> $MAKEFILE
echo >> $MAKEFILE

echo "clean:" >> $MAKEFILE
echo -e "\t-rm -f *.o *.a *.so *.log *core* \$(TARGET) *.tar.gz *.cppe" >> $MAKEFILE
echo "install:" >> $MAKEFILE
echo -e "\t-mv \$(TARGET) \$(INSTALL_DIR)" >> $MAKEFILE
echo "test:" >> $MAKEFILE
echo -e "\t./\$(TARGET)" >> $MAKEFILE
echo "tar:" >> $MAKEFILE
echo -e "\ttar -cvzf \$(TAR_NAME).tar.gz ." >> $MAKEFILE
