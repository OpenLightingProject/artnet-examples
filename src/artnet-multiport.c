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
#include <fcntl.h>

#include <artnet/artnet.h>
#include <artnet/packets.h>


int8_t buff[ARTNET_DMX_LENGTH +1];
int length;
int verbose = 0;


int dmx_callback(artnet_node n, int port, void *d) {
  uint8_t *data;

  data = artnet_read_dmx(n, port, &length);
  memset(buff, 0x00, ARTNET_DMX_LENGTH+1);
  memcpy(&buff[1], data, length);

  printf("got dmx on port %i\n", artnet_get_universe_addr(n, port, ARTNET_OUTPUT_PORT) );
  return 0;
}



int main(int argc, char *argv[]) {
  artnet_node node1, node2;
  char *ip_addr = NULL;
  int i, optc, subnet_addr = 0, port_addr = 0;
  int sd,  maxsd;
  fd_set rset;
  struct timeval tv;

  // parse options
  while ((optc = getopt (argc, argv, "s:p:a:v")) != EOF) {
    switch (optc) {
       case 'a':
        ip_addr = (char *) strdup(optarg);
            break;
      case 'v':
        verbose = 1;
            break;
      case 's':
        subnet_addr = atoi(optarg);

        if(subnet_addr < 0 && subnet_addr > 15) {
          printf("Subnet address must be between 0 and 15\n");
          exit(1);
        }
        break;
      case 'p':
        port_addr = atoi(optarg);

        if(port_addr < 0 && port_addr > 15) {
          printf("Port address must be between 0 and 15\n");
          exit(1);
        }
        break;
          default:
        break;
      }
  }

  node1 = artnet_new(ip_addr, verbose);;
  node2 = artnet_new("192.168.0.100", verbose);;
//  node2 = artnet_new("192.168.0.101", verbose);;

  artnet_join(node1, node2);

  artnet_set_short_name(node1, "Artnet -> DMX (1)");
  artnet_set_long_name(node1, "ArtNet to DMX convertor");
  artnet_set_node_type(node1, ARTNET_NODE);
  artnet_set_dmx_handler(node1, dmx_callback, NULL);

  artnet_set_short_name(node2, "Artnet -> DMX (2)");
  artnet_set_long_name(node2, "ArtNet to DMX convertor");
  artnet_set_node_type(node2, ARTNET_NODE);
  artnet_set_dmx_handler(node2, dmx_callback, NULL);

  // set the first port to output dmx data
  for(i=0; i< 9; i++) {
    if(i < 5) {
      artnet_set_port_addr(node1, i%4, ARTNET_OUTPUT_PORT, i);
      artnet_set_subnet_addr(node1, subnet_addr);
    } else {
      artnet_set_subnet_addr(node2, subnet_addr);
      artnet_set_port_addr(node2, i%4, ARTNET_OUTPUT_PORT, i);
    }
  }

  // we need to share sd[1] to node2 here
  artnet_start(node1);
  artnet_start(node2);

  sd = artnet_get_sd(node1);
  maxsd = sd;

  while(1) {
    FD_ZERO(&rset);
    FD_SET(sd, &rset);

    tv.tv_usec = 0;
    tv.tv_sec = 1;

    switch (select( maxsd+1, &rset, NULL, NULL, &tv )  ) {
      case 0:
      // timeout
      break;
      case -1:
         printf("select error\n");
        break;
      default:

        // these are the same descriptors anyway
        if(FD_ISSET(sd, &rset) ) {
          artnet_read(node1, 0);
        } else {
          printf("random!!!\n");
        }
        break;
    }

  }
  return 0;
}
