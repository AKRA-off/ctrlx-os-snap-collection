#!/bin/sh

#Wait until is solution plug is connected
while ! snapctl is-connected active-solution 
do 
  sleep 5 
done

export LC_ALL=C   

#Create folder
folder_name=AI_Detector
directory="$SNAP_COMMON/solutions/activeConfiguration/$folder_name"

if [ ! -d "$directory"]; then
	mkdir $directory
    echo $directory
fi

#Move to the working directory
cd $directory


if [ -e config.yaml ]
then
    echo "ok"
else
    cp $SNAP/bin/default/config.yaml ./config.yaml
fi

if [ -e config.json ]
then
    echo "ok"
else
    cp $SNAP/bin/default/config.json ./config.json
fi

if [ -e data0.yaml ]
then
    echo "ok"
else
    cp $SNAP/bin/default/data0.yaml ./data0.yaml
fi

if [ -e pre0.jpg ]
then
    echo "ok"
else
    cp $SNAP/bin/default/pre0.jpg ./pre0.jpg
fi

if [ -e yolov8n-seg.onnx ]
then
    echo "ok"
else
    cp $SNAP/bin/default/yolov8n-seg.onnx ./yolov8n-seg.onnx
fi

if [ -e yolov8n-seg.hef ]
then
    echo "ok"
else
    cp $SNAP/bin/default/yolov8n-seg.hef ./yolov8n-seg.hef
fi


# Make sure the configuration directory exists
#if [ ! -d "./Example" ]; then   #Attention with putting a ./ this will check if in the directory is a Target folder
#  mkdir ./Example  #Create target folder if it does not exist
#  cp $SNAP/bin/Example/yolov8n.pt ./Example/yolov8n.pt  #Copy target file that is located in the bin file when the snap is created to the $SNAP_COMMON/solutions/activeConfiguration/novnc/Target folder
#fi