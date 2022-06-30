function loss() {
  DEVICE=$1
  LOSS=$2
  LATENCY=$3

  echo "Apply loss of $LOSS to $DEVICE"

  sudo sudo tc qdisc del dev $DEVICE root handle 1:0 netem delay $LATENCY loss $LOSS
  sudo sudo tc qdisc add dev $DEVICE root handle 1:0 netem delay $LATENCY loss $LOSS
}

LIST=`ifconfig | grep 4163  | grep enx | awk '{print $1}' | sed "s/\://g" | sed ':a;N;$!ba;s/\n/ /g'`

LOSS=${1:-"10%"}
LATENCY=${2:-"0ms"}

for DEVICE in $LIST; 
do 
 loss "$DEVICE" "$LOSS" "$LATENCY"; 
done
