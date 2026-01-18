#!/bin/sh
if [ "$#" -ne 2 ]
then
    echo "Failed: Not enough arguments"
    echo "Argument 1: Path to Search"
    echo "Argument 2: String to Search"
    exit 1
fi

filedir="$1"
searchstr="$2"

if [ ! -d "$filedir" ]
then
    echo "Failed: $filedir is not a valid path."
    exit 1
fi


numFiles=$(find "$filedir" -type f | wc -l)


numMatches=$(grep -r "$searchstr" "$filedir" | wc -l)

echo "The number of files are ${numFiles} and the number of matching lines are ${numMatches}"

exit 0
