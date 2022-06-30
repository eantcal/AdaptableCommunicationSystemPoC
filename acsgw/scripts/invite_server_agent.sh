#!/bin/bash
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

SIPGW="sipproxy"
UA="useragent1"
SA="serveragent1"
TUN="tunnel1"
SIPSERVERNAME="sip.server.org"
MYIP=`ifconfig | grep inet | grep "192.168.0." | awk '{print $2}'` # try to discover the local interface
SIPSRV=$MYIP # registration will happen on current server which is typically the OG
PORT=15060

TMP=$(mktemp /tmp/INVITE.XXXXXX)
H1=`printf "INVITE sip:$SIPSERVERNAME SIP/2.0"`
H2=`printf "From: <sip:$UA>"`
H3=`printf "To: <sip:$SA>"`
H4=`printf "x-tunnel: $TUN"`
H5=`printf "CSeq: 1 INVITE"`
H6=`printf "Call-Id: $RANDOM$RANDOM$RANDOM"`
H7=`printf "Content-Length: 0"`

printf "$H1\r\n$H2\r\n$H3\r\n$H4\r\n$H5\r\n$H6\r\n$H7\r\n\r\n" 
printf "$H1\r\n$H2\r\n$H3\r\n$H4\r\n$H5\r\n$H6\r\n$H7\r\n\r\n" | timeout 10 nc -D -v $SIPSRV $PORT > $TMP
echo ">>> Response"
dos2unix $TMP >/dev/null 2>/dev/null
cat $TMP
echo "<<<"

# TODO check response error code 

LIP=`dig $SA +short`
GW=`dig $SIPGW +short`
sudo ip route del $LIP/32 2>/dev/null
sudo ip route add $LIP/32 gw $GW

