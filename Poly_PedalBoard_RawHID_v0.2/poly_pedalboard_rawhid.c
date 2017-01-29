#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(OS_WINDOWS)
#include <conio.h>
#endif

#include "hid.h"

#include "lo/lo.h"


static char get_keystroke(void);


int main(int argc, char *argv[])
{
	int i, r, num;
	int optch;
	extern int opterr;
	char *h=NULL, *p=NULL;

	char c, buf[64], old_buf[64];
	for(i=0; i<64; i++) {old_buf[i]=0;}


	// Gestion des options
	opterr = 1;

	while((optch = getopt(argc, argv, "h:p:")) != -1){
	  switch(optch){
	  case 'h':
	    h = optarg;
	    printf("Host: %s\n", optarg);
	    break;
	  case 'p':
	    p = optarg;
	    printf("UDP port: %s\n", optarg);
	    break;
	  }
	}


	// On connecte le rawhid device
	r = rawhid_open(1, 0x16C0, 0x0480, 0xFFAB, 0x0200);
	if (r <= 0) {
		printf("no rawhid device found\n");
		return -1;
	}
	printf("found rawhid device\n");


	// On connecte le port OSC
	lo_address t = lo_address_new(h,p);


	while (1) {
		// check if any Raw HID packet has arrived
		num = rawhid_recv(0, buf, 64, 220);
		if (num < 0) {
			printf("\nerror reading, device went offline\n");
			rawhid_close(0);
			return 0;
		}
		if (num > 0) {
		  //	     printf("\nrecv %d bytes:\n", num);
		  for (i=0; i<num; i++) {
		    //		printf("%02X ", buf[i] & 255);
		    if (i > 0 && i < 25) {
		      if( buf[i*2+1] == 1 && old_buf[i*2+1] != buf[i*2+1]) {
			//			printf("/Pedalboard/button %d\n", i);
			if (lo_send(t, "/pedalBoard/button", "i", i) == -1) {
			  printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
			}
		      }
		      old_buf[i*2+1] = buf[i*2+1];
		      /*		      else {
			printf("| %d :: new: %d - old: %d |", buf[i*2], buf[i*2+1], old_buf[i*2+1]);
			}*/
		    }
		    /*		    if (i % 16 == 15 && i < num-1) {
		      printf("\n");
		      char testdata[5] = "ABCDE";
		      lo_blob btest = lo_blob_new(sizeof(testdata), testdata);

		      if (lo_send(t, "/foo/bar", "ff", 0.12345678f, 23.0f) == -1) {
                        printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
		      }
		      
		      }*/
		  }
//		  		  		  printf("i\n");
		}
		// check if any input on stdin
		/*		while ((c = get_keystroke()) >= 32) {
			printf("\ngot key '%c', sending...\n", c);
			buf[0] = c;
			for (i=1; i<64; i++) {
				buf[i] = 0;
			}
//			rawhid_send(0, buf, 64, 100);
}*/
	}
}

#if defined(OS_LINUX) || defined(OS_MACOSX)
// Linux (POSIX) implementation of _kbhit().
// Morgan McGuire, morgan@cs.brown.edu
static int _kbhit() {
	static const int STDIN = 0;
	static int initialized = 0;
	int bytesWaiting;

	if (!initialized) {
		// Use termios to turn off line buffering
		struct termios term;
		tcgetattr(STDIN, &term);
		term.c_lflag &= ~ICANON;
		tcsetattr(STDIN, TCSANOW, &term);
		setbuf(stdin, NULL);
		initialized = 1;
	}
	ioctl(STDIN, FIONREAD, &bytesWaiting);
	return bytesWaiting;
}
static char _getch(void) {
	char c;
	if (fread(&c, 1, 1, stdin) < 1) return 0;
	return c;
}
#endif


static char get_keystroke(void)
{
	if (_kbhit()) {
		char c = _getch();
		if (c >= 32) return c;
	}
	return 0;
}


