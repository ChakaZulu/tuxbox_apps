#!/bin/sh
#pzapit -kill
#dvbtune -c 0 -f 12603750 -p H -s 22000 -n 768
dvbnetctrl -add 768
ifconfig dvb0_0 10.0.0.1 netmask 255.0.0.0
#ip link set dvb0_0 up
echo Device dvb0_0 aktiviert
sleep 2
#ip addr add 10.0.0.1/8 dev dvb0_0
echo IP fuer dvb0_0 gesetzt
sleep 2
/sbin/route add -net 224.0.0.0 netmask 240.0.0.0 dev eth0
#ip route add 224.0.0.0/4 dev eth0
echo Multicastroute gesetzt
sleep 2
/bin/mrouted -c /etc/mrouted.conf
echo Multicastrouter gestartet
