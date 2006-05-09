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
#include <time.h>
#include <string.h>

#include <artnet/artnet.h>
#include <artnet/packets.h>

int length ;
int verbose = 0;

int dmx_callback(artnet_node n, void *p, void *d) {
	static time_t last = 0 ;
	static int counter = 0 ;
	time_t now ;
	static int last_seq = 0;
	
	artnet_packet pack = (artnet_packet) p ;
	
//	printf("seq %d\n", pack->data.admx.sequence ) ;

	// first time
	if(last ==0) {
		time(&last) ;
		last_seq = pack->data.admx.sequence ;
	}

	time(&now) ;

	if(pack->data.admx.sequence - last_seq > 1) {
			printf("lost %d packets %d %d \n", pack->data.admx.sequence - last_seq, pack->data.admx.sequence , last_seq  ) ;
	}
	
	if(last == now) 
		counter++ ;
	else {
		printf("Got %d packets last second\n", counter) ;
		counter = 0;
		last = now ;
	}
	last_seq = pack->data.admx.sequence ;
	

	return 0;
}



int main(int argc, char *argv[]) {
	artnet_node node ;
	char *ip_addr = NULL ;
	int optc, subnet_addr = 0, port_addr = 0 ;
	
	// parse options 
	while ((optc = getopt (argc, argv, "s:p:d:a:v")) != EOF) {
		switch (optc) {
 			case 'a':
				ip_addr = (char *) strdup(optarg) ;
		        break;
			case 'v':
				verbose = 1 ;
		        break;
			case 's':
				subnet_addr = atoi(optarg) ;

				if(subnet_addr < 0 && subnet_addr > 15) {
					printf("Subnet address must be between 0 and 15\n") ;
					exit(1) ;
				}
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

    node = artnet_new(ip_addr, verbose) ;

	artnet_set_short_name(node, "Artnet -> DMX ") ;
	artnet_set_long_name(node, "ArtNet Flood RX") ;
	artnet_set_node_type(node, ARTNET_NODE) ;

	artnet_set_handler(node, ARTNET_DMX_HANDLER, dmx_callback, NULL) ;

	// set the first port to output dmx data
	artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;
	artnet_set_subnet_addr(node, subnet_addr) ;

	// set the universe address of the first port
	artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, port_addr) ;
	artnet_start(node) ;

	while(1) {
		artnet_read(node, 1) ;
	}

	return 0 ;	
}
