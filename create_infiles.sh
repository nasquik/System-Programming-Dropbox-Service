#!/bin/bash

# ./create_infiles.sh dir_name num_of_files num_of_dirs levels

if ! [[ $2 =~ ^[0-9]+$ ]] || ! [[ $3 =~ ^[0-9]+$ ]] || ! [[ $4 =~ ^[0-9]+$ ]]; then
   echo "Error! Arguments 2, 3 and 4 must be integers."
fi

if [ -d "$1" ] 
then
    echo "Directory $1 exists." 
else
    mkdir "$1"
fi

cd "$1" || return

if [[ $3 -eq 0 ]] || [[ $4 -eq 0 ]]; then
    NumFiles=0
    for(( NumFiles = 0; NumFiles < "$2"; NumFiles++)); do
        rand=$(( ( RANDOM % 8 )  + 1 ))
        name=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $rand | head -n 1)
        touch "$name"
        echo "$name"
        rand=$(( (RANDOM % 128) + 1))
        rand=$(( "$rand" * 1024 )) 
        string=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $rand | head -n 1)
        echo "$string" >> "$name"
    done
    exit 0
fi 

i=0
NumDir=0
Level=0
declare -a ListOfDirs

while [ $i -lt "$3" ]
    do
    j=0
    while [ $j -lt "$4" ]
        do
        rand=$(( ( RANDOM % 8 )  + 1 ))
        string=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $rand | head -n 1)
        mkdir "$string"
        NumDir=$((i+1))
        i=$NumDir

        cd "$string" || return
        ListOfDirs+=("$PWD")
        echo "$PWD"

        if [ $NumDir -eq "$3" ]
            then break;
        fi

        Level=$((j+1))
        j=$Level
    done
    for ((t = Level; t > 0; t--)) do
        cd ..
    done
done

echo ""

NumFiles=0

while [ $NumFiles -lt "$2" ]
    do
    for dir in "${ListOfDirs[@]}"; do
        rand=$(( ( RANDOM % 8 )  + 1 ))
        name=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $rand | head -n 1)
        cd "$dir" || return
        touch "$name"
        echo "$name"
        rand=$(( (RANDOM % 128) + 1))
        rand=$(( rand * 1024 )) 
        string=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $rand | head -n 1)
        echo "$string" >> "$name"
        NumFiles=$((NumFiles+1))

        if [ $NumFiles -eq "$2" ] 
            then break;
        fi
    done
done