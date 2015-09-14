#!/bin/bash
IP=$1
PORT=$2
CHANNEL=$3
if [ -z "$IP" ];then
    IP=192.168.1.11
fi

if [ -z "$PORT" ];then
    PORT=8080
fi

if [ -z "$CHANNEL" ];then
   CHANNEL="swwy"
fi

ffmpeg -f video4linux2 -i /dev/video0 -r 15 -f alsa -i hw:0 -ar 22050 -ac 1 -vcodec libx264 -g 15 -acodec libfaac -b:v 500k -f flv -chunked_post 0 http://$IP:$PORT/channel=$CHANNEL&type=flv
