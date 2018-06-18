make
sudo insmod message_slot.ko

sudo mknod /dev/slot1 c 200 1
sudo ./message_sender /dev/slot1 1 hello
sudo ./message_reader /dev/slot1 1
sudo ./message_sender /dev/slot1 3 blablabla
sudo ./message_reader /dev/slot1 3
sudo ./message_sender /dev/slot1 3 nextmessage
sudo ./message_reader /dev/slot1 3
sudo ./message_reader /dev/slot1 1
sudo rm /dev/slot1

sudo ./message_reader /dev/slot1 2
sudo ./message_reader /dev/slot2 3

sudo rmmod message_slot
