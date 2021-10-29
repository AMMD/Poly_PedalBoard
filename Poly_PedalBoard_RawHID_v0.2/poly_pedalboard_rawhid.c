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


int main(int argc, char *argv[])
{
  int i, r, num;
	int optch;
	extern int opterr;
	char *h=NULL, *p=NULL, *lp=NULL;
	char buf[64], old_buf[64];
	for(i=0; i<64; i++) {old_buf[i]=0;}


	// Gestion des options
	opterr = 1;

	while((optch = getopt(argc, argv, "h:p:l:")) != -1){
	  switch(optch){
	  case 'h':
	    h = optarg;
	    printf("Host: %s\n", optarg);
	    break;
	  case 'p':
	    p = optarg;
	    printf("UDP port: %s\n", optarg);
	    break;
    case 'l':
  	    lp = optarg;
  	    printf("Local UDP port: %s\n", optarg);
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



    // Port OSC d'origine
    lo_server server = lo_server_new(lp, NULL);

	// Port OSC de destination
	lo_address t = lo_address_new(h,p);

    lo_send_from(t, server, LO_TT_IMMEDIATE, "/test", "i", 1);

	while (1) {
		// check if any Raw HID packet has arrived
		num = rawhid_recv(0, buf, 64, 220);
		if (num < 0) {
			printf("\nerror reading, device went offline\n");
			rawhid_close(0);
            lo_server_free(server);
            server = NULL;
			return 0;
		}
		if (num > 0) {
		  //	     printf("\nrecv %d bytes:\n", num);
		  for (i=0; i<num; i++) {
		    //		printf("%02X ", buf[i] & 255);
		    if (i > 0 && i < 25) {
		      // Si bouton appuyé, on envoi le message OSC par défaut
		      if( buf[i*2+1] == 1 && old_buf[i*2+1] != buf[i*2+1]) {
			//			printf("/Pedalboard/button %d\n", i);
			if (lo_send_from(t, server, LO_TT_IMMEDIATE, "/pedalBoard/button", "i", i) == -1) {
			  printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
			}
		      }
		      // Si bouton relâché et type de bouton toggle, on envoie le message de release
		      else if( buf[i*2+1] == 0 && old_buf[i*2+1] != buf[i*2+1]) {
			if (lo_send_from(t, server, LO_TT_IMMEDIATE, "/pedalBoard/buttonRelease", "i", i) == -1) {
			  printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
			}
		      }
		      old_buf[i*2+1] = buf[i*2+1];
		    }
		  }
		}
	}

    lo_server_free(server);
    server = NULL;
}
