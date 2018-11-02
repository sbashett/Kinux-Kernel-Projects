#!/bin/bash

echo "Please make sure to remove the modules and reinsert them if the user.o program has already been run for the program to work correctly"


ls /sys/class/HCSR
sleep 1
ls /sys/class/HCSR/HSC_1
sleep 1
echo 8 > /sys/class/HCSR/HSC_1/TRIGGER
sleep 1
echo 3 > /sys/class/HCSR/HSC_1/ECHO
sleep 1
echo 5 > /sys/class/HCSR/HSC_1/NUMBER_SAMPLES
sleep 1
echo 60 > /sys/class/HCSR/HSC_1/SAMPLE_PERIOD
sleep 1
echo 1 > /sys/class/HCSR/HSC_1/ENABLE
sleep 2
cat /sys/class/HCSR/HSC_1/DISTANCE
sleep 1
echo 10 > /sys/class/HCSR/HSC_2/TRIGGER
sleep 1
echo 2 > /sys/class/HCSR/HSC_2/ECHO
sleep 1
echo 5 > /sys/class/HCSR/HSC_2/NUMBER_SAMPLES
sleep 1
echo 60 > /sys/class/HCSR/HSC_2/SAMPLE_PERIOD
sleep 1
echo 1 > /sys/class/HCSR/HSC_2/ENABLE
sleep 2
cat /sys/class/HCSR/HSC_2/DISTANCE



