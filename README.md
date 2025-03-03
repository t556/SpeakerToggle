# SpeakerToggle
Small program to toggle sound devices. I have a shortcut on my windows desktop. This is to enable/disable my speakers quickly/easily. There is nothing worse than being in the office and my speakers are on and start playing sound, so I like to keep them disabled. However, sometimes I need them enabled for meetings. This program allows me to toggle it quickly.

Running it: 
Run as administrator
It will look for sound devices and list them out in the console.
Input the corresponding number (1,2,3...) for the device you want to disable, or 0 to exit.
Press enter
The device should be disabled.

These are commands I used to compile it:

g++ Disable.cpp -lsetupapi -o disable.exe
