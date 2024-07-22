#!/bin/bash

set -e
set -u

finder() {
	local path=$1
	local search=$2

	if [[ -z $path ]] || [[ -z $search ]] || [[ ! -d $path ]];
	then
		echo "Invalid parameters"
		exit 1
	fi

	# echo "finder"
	# echo $path
	# echo $search
	local num_files=$(ls -R $path | tail -n +2 | wc -l)
	local num_lines=$(grep -rs $search $path | wc -l)

	echo "The number of files are $num_files and the number of matching lines are $num_lines"
}

finder $1 $2
