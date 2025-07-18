#!/bin/bash
declare -i should_run=0
declare -i quick_crash_count=0
declare -i start_timestamp=0
declare -i return_code=0
declare -i run_time=0
while [ $should_run -eq 0 ]
do
	echo "Starting EdgeSoft."
	dmesg -n 1
	if [ ! -e /root/passive.fifo ]; then
		mkfifo /root/passive.fifo
        mkfifo /root/miso.fifo
		mkfifo /root/mosi.fifo
    fi

	start_timestamp=$(date +%s)
	/root/current/edgesoft
	return_code=$?
	echo "EdgeSoft exited."
	run_time=$(( $(date +%s) - start_timestamp ))
	#update if there's anything to update
	cp -a /root/new/. /root/current/
	rm -rf /root/new/*
	#then either exit edge_launcher, reboot the system, shutdown, or relaunch edgesoft
	if [ $return_code -eq 8 ]
	then
		should_run=$1
	elif [ $return_code -eq 9 ]
	then
		reboot
	elif [ $return_code -eq 10 ]
	then
		shutdown -h now
	#if the soft crashes often and quickly, we should consider reverting to a previous version
	elif [ $run_time -le 120 ]
	then
		echo "EdgeSoft seems to have crashed."
		(( quick_crash_count++ ))
		if [ $quick_crash_count -eq 10 ]
		then
			echo "Crashed too many times, restoring backup version."
			cp -a /root/backup/. /root/current/
		elif [ $quick_crash_count -eq 20 ]
		then
			echo "Crashed too many times, restoring factory version."
			cp -a /root/factory/. /root/current/
		fi
	elif [ $run_time -ge 3600 ]
	then
		(( quick_crash_count = 0 ))
	fi	
done
exit 0
