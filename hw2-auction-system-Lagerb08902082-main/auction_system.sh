#!/bin/bash

# function 
make_combination()
{
    combination=()
    for ((i = 0; i < 8; i++))
    do 
        combination[$i]=$(($i+1))
    done

    while [ ${combination[7]} -le $player_num ]
    do 
        for ((k=0; k < 8; k++))
        do 
            combinations[${#combinations[@]}]=${combination[$k]}
        done
        t=7
        while [ t != 0 ] && [ ${combination[$t]} == $(($player_num - 7 + $t)) ]
        do 
            t=$(($t - 1))
        done   
        combination[$t]=$((${combination[$t]} + 1))
        for (( j=$(($t + 1)); j < 8; j++))
        do 
            combination[$j]=$((${combination[$(($j - 1))]} + 1))    
        done
    done
}

makefifo()
{
    if [ ! -e fifo_0.tmp ]
    then
        mkfifo fifo_0.tmp 
    fi

    for ((i = 1; i <= $host_num; i++))
    do
        if [ ! -e fifo_$i.tmp ]
        then
            mkfifo fifo_$i.tmp 
            sleep 100000 > fifo_$i.tmp &
        fi
    done
}

removefifo()
{
    rm fifo_0.tmp
    for ((i = 1; i <= $host_num; i++))
    do
        rm fifo_$i.tmp
    done
}

get_comb()
{
    for ((j = 0; j < 8; j++))
    do
        comb_choosed[$j]=${combinations[$(($pair_index * 8 + $j - 8))]}
    done 
}

init_score()
{
    score=()
    for ((i = 0; i <= $player_num; i++))
    do 
        score[$i]=0
    done
    rank_score_table=(0)
    for ((i = 1; i <= 8; i++))
    do 
        rank_score_table[$i]=$((8 - $i))
    done
}

calculate_score()
{
    player_id=${word[0]}
    player_rank=${word[1]}
    score[$player_id]=$((${score[$player_id]} + ${rank_score_table[$player_rank]}))
}

key_to_host_id()
{
    id=0
    for ((i = 0; i < 8; i++))
    do 
        if [ ${word[0]} == ${key_chain[$i]} ]
        then 
            id=$i
            break
        fi
    done
}

read_host()
{
    read -t 0.5 line < fifo_0.tmp
    # echo "$line"
    word=($line)
    if [ ${#word[@]} == 1 ]
    then
        # echo "$line"
        count=$(($count + 1))
        key_to_host_id
        ready_list[$id]=1
        for ((j = 0; j < 8; j++))
        do 
            read -t 0.5 line < fifo_0.tmp
            word=($line)
            calculate_score
            # echo "$line"
        done
    fi
}

make_end_msg()
{
    end_msg=()
    for ((i=0 ; i<8; i++))
    do
        end_msg[$i]=-1
    done
}

send_end()
{
    for ((i = 1; i <= $host_num; i++))
    do 
        echo ${end_msg[@]} > fifo_$i.tmp
    done
}

print_score()
{
    for ((i = 1; i <= $player_num; i++))
    do 
        echo $i ${score[$i]}
    done
}
# function

# ----------------------------------------------main process---------------------------------------------#

# get host and player number and init the score of players
host_num=$1
player_num=$2
init_score

# make fifo 
makefifo

# get the 8 out of N combination
combinations=()
make_combination
comb_num=$((${#combinations[@]}/8))

# get keys, ready list and execute ./host
key_chain=(0)
ready_list=(0)
for ((i = 1; i <= $host_num; i++))
do 
    # random key
    key=$(($RANDOM+$RANDOM))
    key=$(($key%65535))
    key_chain[${#key_chain[@]}]=$key 
    # ready list
    ready_list[$i]=1
    # execute host
    ./host $i ${key_chain[$i]} 0 &
done

# process every combinations
pair_index=1
comb_choosed=()
get_comb
# echo ${comb_choosed[@]}
while [ $pair_index -le $comb_num ]
do 
    for ((i = 1; i <= $host_num; i++))
    do 
        if [ ${ready_list[$i]} == 1 ] && [ $pair_index -le $comb_num ]
        then
            echo ${comb_choosed[@]} > fifo_$i.tmp 
            ready_list[$i]=0
            # sleep(1)
            pair_index=$(($pair_index + 1))
            get_comb
            read_host 
        fi
    done
done

# read_host 
# make end message
make_end_msg

# send end message
send_end

# print every player's score
print_score

# remove fifo
removefifo

# kill sleep
# killall -e sleep
