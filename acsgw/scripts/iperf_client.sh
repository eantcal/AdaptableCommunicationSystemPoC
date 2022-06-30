LOCALIP=`ifconfig | grep "10.0.0" | awk '{ print $2 }'`
REMOTEIP=`route -n | grep tunnel | grep -v $LOCALIP | head -n 1 | awk '{ print $1 }'` 
iperf -B $LOCALIP -c $REMOTEIP
