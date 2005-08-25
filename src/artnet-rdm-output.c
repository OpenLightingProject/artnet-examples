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

#include <stdint.h>
#include <artnet/artnet.h>

#define UID_WIDTH 6

#define UID_COUNT 198
int verbose = 0 ;


uint8_t *generate_rdm_tod(int count, int iteration) {
	uint8_t *ptr = malloc(count * UID_WIDTH) ;
	int i ;
	
	if(ptr == NULL) {
		printf("malloc failed\n") ;
		exit(1) ;
	}

	memset(ptr, 0x00, UID_WIDTH * count ) ;
	for(i = 0 ; i < count; i++) {
		ptr[i * UID_WIDTH +5] = i ;
		ptr[i * UID_WIDTH +4] = iteration ;
	}

	return ptr;

}

/*
 * this is hell dodgy, i can't find the rdm spec so I don't know much about the packet structure
 * looking at AL's header files it looks like a RDM packet it 278 bytes. 512 bytes have been allocated
 *
 * In summary: it's up to you to decode the rdm packet and deal with it.
 * 
 * TODO: helper library for rdm (grab the spec from ...?) 
 *
 */
int rdm_handler(artnet_node n, int address, uint8_t *rdm, int length, void *d) {

	printf("got rdm data for address %d, of length %d\n", address, length) ;

	return 0;
}

int rdm_initiate(artnet_node n, int port, void *d) {
	uint8_t *tod ;
	int *count = (int*) d ;
	uint8_t uid[UID_WIDTH] ;
	
	memset(uid, 0x00, UID_WIDTH) ;
		
	tod = generate_rdm_tod(UID_COUNT, *count) ;
	artnet_add_rdm_devices(n, 0, tod, UID_COUNT) ;
	free(tod) ;

	uid[5] = 0xFF ;
	uid[4] = *count ;
	artnet_add_rdm_device(n,0, uid) ;

	uid[5] = 0x03 ;
	artnet_remove_rdm_device(n,0, uid) ;
	
	uid[5] = 0x06 ;
	artnet_remove_rdm_device(n,0, uid) ;

	(*count)++ ;

	return 0;
}

int main(int argc, char *argv[]) {
	artnet_node node ;
	char *ip_addr = NULL ;
	int optc ;
	uint8_t *tod ;
	int tod_refreshes = 0 ;
	
	// parse options 
	while ((optc = getopt (argc, argv, "va:")) != EOF) {
		switch  (optc) {
 			case 'a':
				ip_addr = (char *) strdup(optarg) ;
		        break;
			case 'v':
				verbose = 1 ;
				break; 
      		default:
				break;
    	}
	}

    node = artnet_new(ip_addr, verbose) ; ;

	artnet_set_short_name(node, "artnet-rdm") ;
	artnet_set_long_name(node, "ArtNet RDM Test, Output Node") ;
	artnet_set_node_type(node, ARTNET_NODE) ;

	// set the first port to output dmx data
	artnet_set_port_type(node, 0, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;
	artnet_set_subnet_addr(node, 0x00) ;

	// set the universe address of the first port
	artnet_set_port_addr(node, 0, ARTNET_OUTPUT_PORT, 0x00) ;

	// set poll reply handler
//	artnet_set_handler(node, ARTNET_REPLY_HANDLER, reply_handler, NULL) 
	artnet_set_rdm_initiate_handler(node, rdm_initiate , &tod_refreshes ) ;
	artnet_set_rdm_handler(node, rdm_handler, NULL ) ;
		
	
	tod = generate_rdm_tod(UID_COUNT, tod_refreshes++) ;
	artnet_add_rdm_devices(node, 0, tod, UID_COUNT) ;
	
	
	artnet_start(node) ;

	artnet_send_tod_control(node, 0x10, ARTNET_TOD_FLUSH) ;
	artnet_send_rdm(node, 0x00 , tod, UID_COUNT* UID_WIDTH) ;
	free(tod) ;
	// loop until control C
	while(1) {
		artnet_read(node, 1) ;
	}
	// never reached
	artnet_destroy(node) ;

	return 0 ;	
}
