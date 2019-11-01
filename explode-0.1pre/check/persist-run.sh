#!/bin/bash
echo "I'm running!!"

cd /home/junfeng/explode/check
#./monitor &
sudo -u junfeng ./check-fs -c ckpt >> ./out
