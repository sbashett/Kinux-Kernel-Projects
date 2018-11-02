#!/bin/bash

echo -e "WARNING : Please make sure to remove the modules and reinsert them \nif the user.o program has already been run for the program to work correctly\n"


sudo chmod 777 ./interface.sh

print_help()
{
echo "Error...
	Usage : 
	-e : echo pin
	-t : trigger pin
	-n : number of samples
	-s : sampling period"
}

declare -a store_array=(ECHO TRIGGER NUMBER_SAMPLES ENABLE)

declare -a read_array=(ECHO TRIGGER NUMBER_SAMPLES SAMPLE_PERIOD DISTANCE)


sens_write()
{
attr=$1
val=$2
n=$3

if [ $# -gt 3 ];then
	echo "Invalid number of arguments to sens_write "
	exit 1
fi
echo "${val}" > "/sys/class/HCSR/"HSC_${n}"/${attr}"
if [ $? -ne 0 ];then
	echo "Write to "${attr}" of "HSC_${n}" was unsuccessful"
	exit 1
fi
}


sens_read()
{
attr=$1
n=$2

if [ $# -gt 3 ];then
	echo "Invalid number of arguments to sens_write "
	exit 1
fi
echo "sensor $n | $attr is :"
cat "/sys/class/HCSR/"HSC_${n}"/${attr}"

if [ $? -ne 0 ];then
	echo "Read of "${attr}" of "HSC_${n}" was unsuccessful"
	exit 1
fi
}

################################ MAIN #############################
 
declare -a sens_count

read -p "Please enter the command that you would like to do [show store]:" cmd

if [ "${cmd}" = "store" ];then

	read -p "Enter the sensors that you want to store to" -a sens_count
	if [ ${#sens_count[@]} -eq 0 ];then
		echo "Enter some vlaue for sensors"
		exit 1
	fi
	for i in "${sens_count[@]}";do
		echo "Enter the values for sensor ${i}:"
		for char in ${store_array[@]};do
			read -p ""$char":" val
			sens_write "${char}" "${val}" "${i}"
			sleep 1 
		done
	done
	

elif [ "${cmd}" = "show" ];then
	read -p "Enter the sensors that you want to display" -a sens_count
	if [ ${#sens_count[@]} -eq 0 ];then
		echo "Enter some vlaue for sensors"
		exit 1
	fi

		for i in "${sens_count[@]}";do
			for char in ${read_array[@]};do
				#read -p ""$char":" val
				sens_read "${char}" "${i}"
				sleep 1
			done
		done
	
else 
	echo "Error..."
fi
