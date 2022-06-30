#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

SIPSERVERNAME="sip.server.org"
UA="useragent1"
TUN="tunnel1"
MYIP=`ifconfig | grep inet | grep "192.168.0." | awk '{print $2}'` # try to discover the local interface
SIPSRV=$MYIP # registration will happen on current server which is typically the OG
PORT=15060

TMP=$(mktemp /tmp/REGISTER.XXXXXX)

H1=`printf "REGISTER sip:$SIPSERVERNAME SIP/2.0"`
H2=`printf "Contact: <sip:$UA>"`
H3=`printf "x-tunnel: $TUN"`
H4=`printf "CSeq: 1 REGISTER"`
H5=`printf "Content-Length: 0"`

printf "$H1\r\n$H2\r\n$H3\r\n$H4\r\n$H5\r\n$H6\r\n\r\n" 
sleep 1
printf "$H1\r\n$H2\r\n$H3\r\n$H4\r\n$H5\r\n$H6\r\n\r\n" | timeout 10 nc -D -v $SIPSRV $PORT > $TMP

echo ">>> Response"
dos2unix $TMP >/dev/null 2>/dev/null
cat $TMP
echo "<<<"

# TODO check if we got 200 OK otherwise stop with error

# Resolve local address by using DNS server
LIP=`dig $UA +short`

sudo ip link del $UA
sudo ip link add $UA type dummy
sudo ifconfig $UA $LIP netmask 255.255.255.255 up
sudo ip route del $LIP/32 2>/dev/null
sudo ip route add $LIP/32 dev $TUN

