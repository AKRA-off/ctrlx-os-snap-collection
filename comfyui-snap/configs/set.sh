#!/bin/sh

#Wait until is solution plug is connected
while ! snapctl is-connected active-solution 
do 
  sleep 5 
done

export LC_ALL=C   

#Create folder
folder_name=comfyui
directory="$SNAP_COMMON/solutions/activeConfiguration/$folder_name"
#directory="/snap/ai-detector/current/"

if [ ! -d "$directory" ]; then
	mkdir $directory
    echo $directory
fi

#Move to the working directory
cd $directory
