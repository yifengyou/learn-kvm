#!/bin/bash
# file: ksm-test.sh

echo "----stoping services: ksm and ksmtuned ..."
service ksm stop
service ksmtuned stop

echo "----'free -m' command output before starting any guest."
free -m

# start 4 Win7 guest
for i in $(seq 1 4)
do
	qemu-img create -f qcow2 -o backing_file="/images/Windows/ia32e_win7_ent.img" win7-${i}.qcow2
	echo "starting the No. ${i} guest..."
	qemu-system-x86_64 win7-${i}.qcow2 -smp 2 -m 1024 -net nic -net tap -daemonize
	sleep 20
done

echo "----'free -m' command output with several guests running ."
free -m

echo "----starting services: ksm and ksmtuned ..."
service ksm start
service ksmtuned start

sleep 600

echo "----'free -m' command output with ksm and ksmtuned running."
free -m

