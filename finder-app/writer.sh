#!/bin/bash


writer() {
	local path=$1
	local word=$2

	if [[ -z $path ]] || [[ -z $word ]];
	then
		echo "Invalid parameters"
		exit 1
	fi

	# echo "writer"
	# echo $path
	# echo $word
	mkdir -p $(dirname $path)
	echo "$word" > $path
}

writer $1 $2
