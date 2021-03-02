# Rootkit
Kernel Mode Rootkit &amp; Keylogger for Linux Systems. The rootkit is implemented on Linux Kernel 4.10.0-38-generic.
The job of this rootkit is to hide the process identifier of the keylogger and itself. The keylogger is the malicious program and write all input from the keyboard into a file.

### Use the rootkit:
If you would like to use this rootkit, make sure you run it on a virtual machine or on a computer without any important data, because it can damage the kernel of your system.
* Compile the file keylogger.c (i.e. gcc keylogger.c)
* Run the executable file of the keylogger with the argument of the file "logFile.text" (i.e. ./a.out logFile.text)
* Now search for the PID of the keylogger process (i.e ps aux | grep keylogger)
* Go to the Rootkit folder and open the file "hidePID.c" and search for the line: 
```C
char HidePID[]= "5033"; // Hier wird die zu verbergende Prozess-ID gespeichert
```
* Change the PID "5033" to the PID where your keylogger is running
* Save the file and run the makefile (i.e. make all)
* Load the Rootkit into the kernel of your linux system with "insmod hidePID.ko"
* Now your Process ID of the keylogger should be no more shown, check it with "ps aux | grep keylogger" and also the rootkit should be not listed in the lsmod output. lsmod shows you all acutal running kernel modules
* If you now check the "logFile.text" you should see all keyboard inputs you have done from the time you started the keylogger
* You can´t now remove the rootkit with the rmmod command, because the kernel module is not more listet in your system. To remove it, restart your system.

