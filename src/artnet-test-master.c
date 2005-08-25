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
#include <sys/types.h> 
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <artnet/artnet.h>
#include <artnet/packets.h>


#include "artnet-test-defaults.h"
#include "artnet-test-defns.h" 

#define START_SIZE 1048576
#define GROW_SIZE 1048576
#define LINE 80

#define NEW_SHORT_NAME "New short name"
#define NEW_LONG_NAME "New long name"

typedef enum {
	PRE_TEST,
	TEST_ADDRESS_1,
	TEST_ADDRESS_2,
	TEST_ADDRESS_3,
	TEST_ADDRESS_4,

	TEST_ERROR,


} test_state_e;

typedef struct {
	char *ip_addr;
	int verbose ;
	artnet_node_entry peer ;
	test_state_e state;
} options_t ;


/*
 * Parse command lines opts
 */
void parse_options(int argc, char *argv[], options_t *options ) {
	int optc ;

	options->ip_addr = NULL ;
	options->verbose = 0 ;
	options->peer = NULL ;
	options->state = PRE_TEST ;

	// parse options 
	while ((optc = getopt (argc, argv, "va:")) != EOF) {
		switch (optc) {
 			case 'a':
				options->ip_addr = (char *) strdup(optarg) ;
		        break;
			case 'v':
				options->verbose = 1 ;
				break; 
      		default:
				break;
    	}
	}
}




/*
 * Scan for nodes on the network
 *
 */
void scan_for_nodes(artnet_node n, options_t *options ) {
	artnet_node_list nl = artnet_get_nl(n) ;
	artnet_node_entry ent = artnet_nl_first(nl) ;
		
	#define WAIT_TIME 2
	int i, sd[2] ;
	time_t start ;

	sd[0] = artnet_get_sd(n,0) ;
	sd[1] = artnet_get_sd(n,1) ;

	start = time(NULL) ;
	
	printf("Searching for nodes... (waiting %i seconds)\n", WAIT_TIME) ;
	// wait for WAIT_TIME seconds before quitting
	while(time(NULL) - start < WAIT_TIME) {
		artnet_read(n,1) ;
	}

	i = 1 ;
	
	for( ent = artnet_nl_first(nl) ; ent != NULL; ent = artnet_nl_next(nl))  {
		if(0 == strncmp(ent->longname , LONG_NAME, ARTNET_LONG_NAME_LENGTH) &&
			0 == strncmp(ent->shortname , SHORT_NAME, ARTNET_SHORT_NAME_LENGTH) )

				options->peer = ent;
	} 
	
}


int poll_reply_handler(artnet_node n, void *pp, void *d) {
	options_t *options = (options_t* ) d;
//	uint8_t in_addr[ARTNET_MAX_PORTS] ;
	uint8_t out_addr[ARTNET_MAX_PORTS] ;
	int i ;
	
	if(options->state == TEST_ADDRESS_1 ) {

		if( strncmp(options->peer->longname, NEW_LONG_NAME, ARTNET_LONG_NAME_LENGTH) ||
			strncmp(options->peer->shortname, NEW_SHORT_NAME, ARTNET_SHORT_NAME_LENGTH) ) {

			printf(" node names incorrectly set\n") ;
			options->state = TEST_ERROR ;
		}

		for(i=0; i < ARTNET_MAX_PORTS; i++) {
			if(options->peer->swin[i] == i && options->peer->swout[i] == i) {
				
				out_addr[i] = 0x0F - i ;
			}
		}

		
		options->state = TEST_ADDRESS_2 ;
		
	} else if (options->state == TEST_ADDRESS_3 ) {
		if( strncmp(options->peer->longname, LONG_NAME, ARTNET_LONG_NAME_LENGTH) ||
			strncmp(options->peer->shortname, SHORT_NAME, ARTNET_SHORT_NAME_LENGTH) ) {

			printf(" node names incorrectly set\n") ;
			options->state = TEST_ERROR ;
		}

		options->state = TEST_ADDRESS_4 ;

	}
	


	return 0;
}




int wait_for_test(artnet_node n, options_t *options) {
	test_state_e current_state = options->state ;	
	
	#define WAIT_TIME 2
	int sd[2] ;
	time_t start ;

	sd[0] = artnet_get_sd(n,0) ;
	sd[1] = artnet_get_sd(n,1) ;

	start = time(NULL) ;
	
	// wait for WAIT_TIME seconds before quitting
	while(time(NULL) - start < WAIT_TIME && options->state == current_state) {
		artnet_read(n,1) ;
	}

	if(options->state == current_state) 
		// got no response
		return -1 ;

	if(options->state == TEST_ERROR)
		// tests returned an error
		return -1 ;

	return 0;
}


int run_address_tests(artnet_node n, options_t *options) {
	int  numTests ;
//	uint8_t in_addr[ARTNET_MAX_PORTS] ;
//	uint8_t out_addr[ARTNET_MAX_PORTS] ;
	
	numTests =  sizeof(address_test_t) ;
	numTests =  sizeof(address_tests) ;

	printf("%i tests\n", numTests) ;

/*
	// send an ArtAddress first
	for(i=0; i < ARTNET_MAX_PORTS; i++) {
		in_addr[i] = i ;
		out_addr[i] = i ;
	}
	
	printf("Sending an ArtAddress packet...") ;
	fflush(stdout) ;

	artnet_send_address(n, options->peer, NEW_SHORT_NAME , NEW_LONG_NAME,
							in_addr , out_addr, 0x01, ARTNET_PC_NONE ) ;

	options->state = TEST_ADDRESS_1 ;
	if( wait_for_test(n, options) )
		return -1 ;

	printf("\tok\n") ;
	
	// change port and addresses
	for(i=0; i < ARTNET_MAX_PORTS; i++) {
		in_addr[i] = 0x0F - i ;
		out_addr[i] = 0x0F - i ;
	}
	
	printf("Sending a second ArtAddress packet...") ;
	fflush(stdout) ;

	artnet_send_address(n, options->peer, SHORT_NAME , LONG_NAME,
							in_addr , out_addr, 0x01, ARTNET_PC_NONE ) ;

	options->state = TEST_ADDRESS_3 ;
	if( wait_for_test(n, options) )
		return -1 ;

	printf("\tok\n") ;

	//


*/
	return 0;





}
int run_tests(artnet_node n, options_t *options) {
	
	// art poll reply
	run_address_tests(n, options) ;

//	run_dmx_tests(n,options) ;

	// art poll reply
//	run_input_tests(n, options) ;

	//
//	run_firmware_tests(n,options) ;

	return 0;
}

int main(int argc, char *argv[]) {
	artnet_node node ;
	options_t options ;

	parse_options(argc, argv, &options) ;

    node = artnet_new(options.ip_addr, options.verbose) ; ;

	artnet_set_short_name(node, "artnet-test") ;
	artnet_set_long_name(node, "ArtNet Test Master") ;
	artnet_set_node_type(node, ARTNET_SRV) ;

	artnet_set_handler(node, ARTNET_REPLY_HANDLER, poll_reply_handler, (void*) &options) ;
	
	artnet_start(node) ;
	artnet_send_poll(node, NULL, ARTNET_TTM_DEFAULT) ;


	scan_for_nodes(node, &options) ;

	if(options.peer == NULL) {
		printf("Could not locale peer node, exiting...\n") ;
		exit(1) ;
	}

	printf("Good, found a test peer at \n") ;

	if( run_tests(node, &options) ) {
		printf("\nERROR: Timeout, aborting tests\n") ;
	}
	return 0 ;	
}
