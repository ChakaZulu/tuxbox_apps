#!/bin/sh
modprobe gtx_fb
ln -s /dev/fb/0 /dev/fb0
fbset -depth 8 -match -xres 320 -yres 288 -vxres 320 -vyres 288

