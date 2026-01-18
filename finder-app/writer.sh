#!/bin/sh

if [ "$#" -lt 2 ]
 then
     echo "Failed:Not enough arguments"
     echo "Argument 1: Path to Search"
     echo "Argument 2: String to Search "
     exit 1
fi

writefile="$1"
writestr="$2"

mkdir -p "$(dirname $writefile)" && touch "$writefile"


if [ "$?" -ne 0 ]
then
	echo "Failed: Directory not found"
	exit 1
fi

echo "$writestr" >> "$writefile"
