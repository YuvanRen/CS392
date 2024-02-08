#!/bin/bash
#rbin.sh
# *******************************************************************************
#  Author  : Yuvan Rengifo
#  Date    : Feb 7, 2024
#  Description: CS392 - Homework 1
#  Pledge  : I pledge my honor that I have abided by the Stevens Honor System.
# ******************************************************************************

# TODO: Fill the header above, and then complete rbin.sh below

flag_h=0
flag_p=0
flag_l=0

#heredoc for usage
msg=$(cat <<'EOF'
Usage: rbin.sh [-hlp] [list of files]
   -h: Display this help;
   -l: List files in the recycle bin;
   -p: Empty all files in the recycle bin;
   [list of files] with no other flags,
        these files will be moved to the
        recycle bin.
EOF
)
op_ov="Error: Too many options enabled."

# Keep track of which and how many times the flag was invoked
while getopts ":hlp" op; do
    case $op in
    h)
        flag_h=$((flag_h + 1));;
    l)
        flag_l=$((flag_l + 1));;    
    p)
        flag_p=$((flag_p + 1));;
    *)
        echo "Error: Unknown option '-$OPTARG'."  >&2
        echo "$msg"
        exit 1
    esac
done

# Error handling function
ErrorAndExit() {
    echo "$op_ov" >&2
    echo "$msg"
    exit 1
}
# Usage printing function
MsgAndExit(){
    echo "$msg"
    exit 0
}

# Instructions for Parse #
   # Usage message if no parameters
if [[ $(($flag_h + $flag_l + $flag_p)) -eq 0 && $# -eq 0 ]]; then 
    MsgAndExit
fi

    # Too many flags
if [[ $(($flag_h + $flag_l + $flag_p)) -gt 1 ]]; then 
    ErrorAndExit
fi

     # -h functionallity
if [[ $flag_h -eq 1 && $(($flag_l + $flag_p)) -eq 0 ]]; then
    MsgAndExit
elif [[ $flag_h -gt 1 ]];then 
    ErrorAndExit
fi

# Creating the Recycling bin #
path="$HOME/.recycle"
readonly path

# Check if path doesnt exist and create the new path
if [ ! -e "$path" ]; then
  mkdir "$path" 
fi
     # -l functionallity
if [[ $flag_l -eq 1 ]]; then
    ls -lAF $path
fi
     # -p functionallity
if [[ $flag_p -eq 1 ]]; then 
    rm -rf "$path"/{,.[!.],..?}*
fi

# If List of files passed then copy found files. else, error
if [[ $(($flag_h + $flag_l + $flag_p)) -eq 0 && $# -gt 0 ]]; then 
    for file in "$@"; do
        if [ -e "$file" ]; then 
            mv "$file" "$path"
        else
            echo "Warning: '$file' not found." >&2
        fi
    done
fi

exit 0

