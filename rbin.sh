#!/bin/bash
#rbin.sh
# *******************************************************************************
#  Author  : Yuvan Rengifo
#  Date    : Feb 6, 2024
#  Description: CS392 - Homework 1
#  Pledge  : I pledge my honor that I have abided by the Stevens Honor System.
# ******************************************************************************

# TODO: Fill the header above, and then complete rbin.sh below

flag_h=0
flag_p=0
flag_l=0
help_msg="Usage: rbin.sh [-hlp] [list of files]
             -h: Display this help;
             -l: List files in the recycle bin;
             -p: Empty all files in the recycle bin;
            [list of files] with no other flags,
                these files will be moved to the
                recycle bin."


while getopts ":hlp" op; do
    case $op in
    h)
        flag_h=1;;
    l)
        flag_l=1;;
    p)
        flag_p=1;;
    *)
        echo "Unknown option" >&2
        echo $help_msg
        exit 1
    esac
done

# Prints help mesg if not op/-h/unkwn optn
if [[ $flag_h -eq 1 ]]; then
    echo $help_msg
fi

# Checks which flags were turned on
if [[ $flag_l -eq 1 ]]; then
    msg=${msg}"l"
fi

if [[ $flag_p -eq 1 ]]; then
    msg=${msg}"p"
fi

# Option if cases
if [[ $(($flag_h + $flag_l + $flag_p)) -gt 1 ]]; then 
    msg="Too many options enabled"
fi

