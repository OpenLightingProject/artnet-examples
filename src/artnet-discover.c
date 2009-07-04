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

#include <artnet/artnet.h>

int verbose = 0;
int nodes_found = 0;


void print_node_config(artnet_node_entry ne) {
  printf("--------- %d.%d.%d.%d -------------\n", ne->ip[0], ne->ip[1], ne->ip[2], ne->ip[3]);
  printf("Short Name:   %s\n", ne->shortname);
  printf("Long Name:    %s\n", ne->longname);
  printf("Node Report:  %s\n", ne->nodereport);
  printf("Subnet:       0x%hhx\n", ne->sub);
  printf("Numb Ports:   %d\n", ne->numbports);
  printf("Input Addrs:  0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx\n", ne->swin[0], ne->swin[1], ne->swin[2], ne->swin[3] );
  printf("Output Addrs: 0x%hhx, 0x%hhx, 0x%hhx, 0x%hhx\n", ne->swout[0], ne->swout[1], ne->swout[2], ne->swout[3] );
  printf("----------------------------------\n");
}

int reply_handler(artnet_node n, void *pp, void *d) {
  artnet_node_list nl = artnet_get_nl(n);

  if (nodes_found == artnet_nl_get_length(nl)) {
    // this is not a new node, just a previously discovered one sending
    // another reply
    return 0;
  } else if(nodes_found == 0) {
    // first node found
    nodes_found++;
    print_node_config( artnet_nl_first(nl));
  } else {
    // new node
    nodes_found++;
    print_node_config( artnet_nl_next(nl));
  }

  return 0;
}

int main(int argc, char *argv[]) {
  artnet_node node;
  char *ip_addr = NULL;
  int optc, sd, maxsd, timeout = 2;
  fd_set rset;
  struct timeval tv;
  time_t start;

  // parse options
  while ((optc = getopt (argc, argv, "va:t:")) != EOF) {
    switch (optc) {
      case 'a':
        ip_addr = (char *) strdup(optarg);
        break;
      case 't':
        timeout = atoi(optarg);
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        break;
    }
  }

  if ((node = artnet_new(ip_addr, verbose)) == NULL) {
    printf("new failed %s\n" , artnet_strerror());
    goto error;
  }

  artnet_set_short_name(node, "artnet-discovery");
  artnet_set_long_name(node, "ArtNet Discovery Node");
  artnet_set_node_type(node, ARTNET_SRV);

  // set poll reply handler
  artnet_set_handler(node, ARTNET_REPLY_HANDLER, reply_handler, NULL);

  if( artnet_start(node) != ARTNET_EOK) {
    printf("Failed to start: %s\n", artnet_strerror());
    goto error_destroy;
  }

  sd = artnet_get_sd(node);

  if (artnet_send_poll(node, NULL, ARTNET_TTM_DEFAULT) != ARTNET_EOK) {
    printf("send poll failed\n");
    exit(1);
  }

  start = time(NULL);
  // wait for timeout seconds before quitting
  while (time(NULL) - start < timeout) {
    FD_ZERO(&rset);
    FD_SET(sd, &rset);

    tv.tv_usec = 0;
    tv.tv_sec = 1;
    maxsd = sd;

    switch (select(maxsd+1, &rset, NULL, NULL, &tv)) {
      case 0:
        // timeout
        break;
      case -1:
        printf("select error\n");
        break;
      default:
        artnet_read(node,0);
        break;
    }
  }

error_destroy :
  artnet_destroy(node);

error:
  free(ip_addr);
  exit(1);
}
