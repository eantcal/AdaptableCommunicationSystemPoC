#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

SIPSERVERNAME="sip.server.org"
SA="serveragent1"
TUN="tunnel1"
MYIP=`ifconfig | grep inet | grep "192.168" | awk '{print $2}'` # try to discover the local interface
SIPSRV=$MYIP #192.168.0.213 # NG gateway
PORT=15060

TMP=$(mktemp /tmp/REGISTER.XXXXXX)

H1=`printf "REGISTER sip:$SIPSERVERNAME SIP/2.0"`
H2=`printf "Contact: <sip:$SA>"`
H3=`printf "Allow: INVITE"`
H4=`printf "x-tunnel: $TUN"`
H5=`printf "CSeq: 1 REGISTER"`
H6=`printf "Content-Length: 0"`

printf "$H1\r\n$H2\r\n$H3\r\n$H4\r\n$H5\r\n$H6\r\n\r\n" :
printf "$H1\r\n$H2\r\n$H3\r\n$H4\r\n$H5\r\n$H6\r\n\r\n" | timeout 1 nc -v $SIPSRV $PORT > $TMP
cat $TMP

#TODO check if we get 200 and then create the I/F

LIP=`dig $SA +short`

sudo ip link add $SA type dummy
sudo ifconfig $SA $LIP netmask 255.255.255.255 up
sudo ip route del $LIP/32 2>/dev/null
sudo ip route add $LIP/32 dev $TUN
