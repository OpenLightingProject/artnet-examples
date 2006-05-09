/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>

#include <artnet/artnet.h>

int verbose = 0;
int use_select = 0 ;

int delay = 25000;


#include <sys/select.h>

int main(int argc, char *argv[]) {
	artnet_node node ;
	char *ip_addr = NULL ;
	int optc,  subnet_addr = 0, port_addr = 0 ;
	uint8_t data[ARTNET_DMX_LENGTH] ;
	struct timeval tv;
	int sd, maxsd ;
	fd_set rset;
	struct timespec ts;

	// parse options 
	while ((optc = getopt (argc, argv, "s:p:d:a:ve")) != EOF) {
		switch (optc) {
 			case 'a':
				ip_addr = (char *) strdup(optarg) ;
		        break;
			case 'v':
				verbose = 1 ;
		        break;
			case 'e':
				use_select = 1 ;
		        break;
			case 's':
				subnet_addr = atoi(optarg) ;

				if(subnet_addr < 0 && subnet_addr > 15) {
					printf("Subnet address must be between 0 and 15\n") ;
					exit(1) ;
				}
				break ;
			case 'd':
				delay = atoi(optarg) ;
				break ;
			case 'p':
				port_addr = atoi(optarg) ;

				if(port_addr < 0 && port_addr > 15) {
					printf("Port address must be between 0 and 15\n") ;
					exit(1) ;
				}
				break ;
      		default:
				break;
    	}
	}

    node = artnet_new(ip_addr, verbose) ; ;

	artnet_set_short_name(node, "Artnet -> DMX ") ;
	artnet_set_long_name(node, "ArtNet Flood RX") ;
	artnet_set_node_type(node, ARTNET_NODE) ;

	// set the first port to input dmx data
	artnet_set_port_type(node, 0, ARTNET_ENABLE_INPUT, ARTNET_PORT_DMX) ;
	artnet_set_subnet_addr(node, subnet_addr) ;

	// set the universe address of the first port
	artnet_set_port_addr(node, 0, ARTNET_INPUT_PORT, port_addr) ;
	artnet_start(node) ;
	
	sd = artnet_get_sd(node) ;

	if(use_select) {
		while(1) {
			FD_ZERO(&rset) ;
			FD_SET(sd, &rset) ;

			tv.tv_usec = delay ;
			tv.tv_sec = 0 ;
		
			maxsd = sd ;
	
			switch ( select( maxsd+1, &rset, NULL, NULL, &tv ) ) {
				case 0:
					artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	
					artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	
					artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	
					artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	

					break ;
				case -1:
				 	printf("select error\n") ;
					break ;
				default:
					artnet_read(node,0) ;
					break;
			}
		}
	} else {
			
//		sched_setscheduler(0, )

		while(1) {
			ts.tv_nsec = delay ;
			ts.tv_sec = 0 ;
			artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	
				
			artnet_read(node,0) ;
			artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	
			
			nanosleep(&ts, NULL) ;
			artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	
			artnet_send_dmx(node,0, ARTNET_DMX_LENGTH, data) ;	

		}
	}
	
	return 0 ;	
}
