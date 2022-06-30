function setdelay() {
  DEVICE=$1
  LATENCY=$2

  echo "Apply delay of $DELAY to $DEVICE"

  sudo sudo tc qdisc del dev $DEVICE root handle 1:0 netem delay $LATENCY 
  sudo sudo tc qdisc add dev $DEVICE root handle 1:0 netem delay $LATENCY 
}

LIST=`ifconfig | grep 4163  | grep enx | awk '{print $1}' | sed "s/\://g" | sed ':a;N;$!ba;s/\n/ /g'`

LATENCY=${1:-"0ms"}

for DEVICE in $LIST; 
do 
 setdelay "$DEVICE" "$LATENCY"; 
done
