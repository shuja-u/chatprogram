The directory contains the following files:
chatServer.c
chatClient.c
helper.c
helper.h
Makefile

Instructions:
1. Ensure "gcc" is installed.
2. Ensure "make" is installed.
3. Open the terminal in the same directory as the Makefile and type "make".

To run the server:
1. Open the terminal in the same directory as the Makefile
2. Enter the command in the following format: ./chatServer <port>
   Example: ./chatServer 6500

To use the client:
1. Open the terminal in the same directory as the Makefile
2. Enter the command in the following format: ./chatClient <server_host> <server_port> <username>
   Example: ./chatClient localhost 6500 user1

Installation commands (for most Debian/Ubuntu based distros):
sudo apt install gcc
sudo apt install make

Resources consulted:
https://linux.die.net/man/3/pthread_mutex_lock
https://stackoverflow.com/questions/24194961/how-do-i-use-setsockoptso-reuseaddr
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/ptjoin.htm
https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.3.0/com.ibm.zos.v2r3.bpxbd00/rp0r0i.htm
https://stackoverflow.com/questions/19482648/reader-writer-lock-in-pthread
https://www.geeksforgeeks.org/how-to-append-a-character-to-a-string-in-c/
http://beej.us/guide/bgc/html/index-wide.html
https://pages.cs.wisc.edu/~remzi/OSTEP/threads-cv.pdf
https://stackoverflow.com/questions/12275381/strncpy-vs-sprintf
https://www.includehelp.com/c-programs/pass-an-array-of-strings-to-a-function.aspx
https://stackoverflow.com/questions/1431576/how-do-you-declare-string-constants-in-c
https://codeforwin.org/2017/09/global-variables-c.html
https://www.youtube.com/watch?v=Pg_4Jz8ZIH4
http://beej.us/guide/bgnet/html/index-wide.html#a-simple-stream-client
https://www.tutorialspoint.com/cprogramming/c_structures.htm
https://www.youtube.com/watch?v=2Ti5yvumFTU
https://www.geeksforgeeks.org/condition-wait-signal-multi-threading/
https://www.youtube.com/watch?v=3E-r4GfvWOI
https://stackoverflow.com/questions/20824229/when-to-use-pthread-exit-and-when-to-use-pthread-join-in-linux
https://www.thegeekstuff.com/2012/05/c-mutex-examples/
https://www.thegeekstuff.com/2012/04/terminate-c-thread/
https://stackoverflow.com/questions/2187474/i-am-not-able-to-flush-stdin
https://gcc.gnu.org/onlinedocs/gcc/Directory-Options.html#Directory-Options