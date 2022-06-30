LIST=`ifconfig | grep 4163  | grep enx | awk '{print $1}' | sed "s/\://g" | sed ':a;N;$!ba;s/\n/ /g'`

for DEVICE in $LIST; 
do 
 sudo sudo tc qdisc del dev $DEVICE root handle 1:0 netem 
done
