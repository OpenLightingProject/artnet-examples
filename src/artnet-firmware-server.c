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

#define START_SIZE 1048576
#define GROW_SIZE 1048576
#define LINE 80

//#define START_SIZE 1024
//#define GROW_SIZE 1024

typedef struct {
    char *ip_addr;
    char *firmware;
    int timeout;
    int verbose;
    int ubea;
    int size;
    uint16_t *buffer;
    int uploading;
} options_t;

void *Malloc(int len) {
  void *ptr = malloc(len);

  if(ptr == NULL) {
    fprintf(stderr,"malloc failed\n");
    exit(1);
  }
  return ptr;
}

void *Realloc(void *ptr, int len) {
  ptr = realloc(ptr, len);

  if(ptr == NULL) {
    fprintf(stderr,"realloc failed\n");
    exit(1);
  }
  return ptr;
}

/*
 * Parse command lines opts
 */
void parse_options(int argc, char *argv[], options_t *options ) {
  int optc;

  options->ip_addr = NULL;
  options->firmware = NULL;
  options->timeout = 2;
  options->verbose = 0;
  options->ubea = 0;
  options->uploading = 0;

  // parse options
  while ((optc = getopt (argc, argv, "uva:t:f:")) != EOF) {
    switch (optc) {
       case 'a':
        options->ip_addr = (char *) strdup(optarg);
            break;
      case 'f':
        options->firmware = (char *) strdup(optarg);
            break;
      case 't':
        options->timeout = atoi(optarg);
            break;
      case 'v':
        options->verbose = 1;
        break;
      case 'u':
        options->ubea = 1;
        break;
          default:
        break;
      }
  }
}

/*
 * Print the node list
 */
void print_node_list(artnet_node n) {
  artnet_node_list nl = artnet_get_nl(n);
  artnet_node_entry ent = artnet_nl_first(nl);
  int i = 1;

  printf("------------------------------\n");
  for( ent = artnet_nl_first(nl); ent != NULL; ent = artnet_nl_next(nl))  {
    printf("%i : %s\n", i++, ent->shortname);
  }
  printf("------------------------------\n");

  printf("Select a number, r to re-scan or q to quit\n");
}

/*
 * Reads the firmware file into memory and returns a point
 * this must be freed once you're done
 */
void read_firmware(char *firmware, options_t *options ) {
  int fd, len, size;
  uint16_t *buf;
  char *buf2;

  struct stat f_stat;

  // check if filename is specified
  // should fix this up
  if(firmware == NULL) {
    printf("Use -f to specify firmware file\n");
    exit(1);
  }

  // open file
  fd = open(firmware, O_RDONLY);

  if ( fstat(fd, &f_stat) ) {
    printf("Firmware file %s does not exist\n", firmware);
    exit(1);
  }

  if( ! fd) {
    printf("Could not open firmware file %s\n", firmware);
    exit(1);
  }

  len = START_SIZE;

  // read file
  for (;;) {
    buf = Malloc(len);
    size = read(fd, (void*) buf, len);

    if( size < len) {
      break;
    } else {
      len += GROW_SIZE;
      free(buf);
      lseek(fd,0,SEEK_SET);
    }
  }

  options->buffer = buf;
  options->size = size;

  buf2 = (char *) buf;

  if(buf2[size-1] == '\n') {
    printf("got char\n");
  }
}

/*
 * Scan for nodes on the network
 *
 */
void scan_for_nodes(artnet_node n) {
  #define WAIT_TIME 2
  time_t start;

  start = time(NULL);

  printf("Searching for nodes... (wait %i seconds)\n", WAIT_TIME);
  // wait for WAIT_TIME seconds before quitting
  while(time(NULL) - start < WAIT_TIME) {
    artnet_read(n,1);
  }

  print_node_list(n);

}

/*
 * This is called when a upload completes
 */
int firmware_complete_callback( artnet_node n, artnet_firmware_status_code  code, void *data) {
  options_t *options = (options_t*) data;

  printf("Upload Complete\n");
  options->uploading = 0;
  return 0;
}

/*
 * Read
 */
int server_handle_input(artnet_node n, options_t *options) {
  char line[LINE];
  artnet_node_entry ent;
  artnet_node_list nl = artnet_get_nl(n);
  int i = 0;
  int index;

  if (fgets(line, LINE, stdin) == NULL) {
    printf("Unable to read from stdin\n");
    return 0;
  }
  index = atoi(line);

  if(0 == strcmp( line ,"q\n") ) {
    return 1;
  } else if ( 0 == strcmp(line, "r\n") ) {
    scan_for_nodes(  n);
  } else if ( index > 0 && index <= artnet_nl_get_length(artnet_get_nl(n)) ) {
    // do upload
    printf("Starting upload...\n");

    for(ent = artnet_nl_first(nl); ent != NULL; ent = artnet_nl_next(nl) ) {
      if(++i == index)
        break;
    }

    if(ent != NULL)
      artnet_send_firmware(n, ent, options->ubea, options->buffer, options->size / sizeof(uint16_t), firmware_complete_callback, (void *) options);

  } else {
    printf("Invalid Command\n");
  }

  return 0;
}

/*
 *
 *
 */
void wait_for_input(artnet_node n, options_t *options) {
  int sd, maxsd;
  fd_set rset;
  struct timeval tv;

  sd = artnet_get_sd(n);
  maxsd = sd;

  while(1) {
    FD_ZERO(&rset);
    FD_SET(STDIN_FILENO, &rset);
    FD_SET(sd, &rset);

    tv.tv_usec = 0;
    tv.tv_sec = 1;

    switch (select( maxsd, &rset, NULL, NULL, &tv )  ) {
      case 0:
      // timeout
      break;
      case -1:
         printf("select error\n");
        break;
      default:
        if(FD_ISSET(STDIN_FILENO, &rset) && ! options->uploading) {
          if ( server_handle_input(n,options) )
            return;

        } else {
          artnet_read(n,0);
          break;
        }

    }
  }
}

int main(int argc, char *argv[]) {
  artnet_node node;
  options_t options;

  parse_options(argc, argv, &options);

  read_firmware(options.firmware, &options);

    node = artnet_new(options.ip_addr, options.verbose); ;

  artnet_set_short_name(node, "artnet-firmware");
  artnet_set_long_name(node, "ArtNet Firmware Test Server");
  artnet_set_node_type(node, ARTNET_SRV);
  artnet_start(node);
  artnet_send_poll(node, NULL, ARTNET_TTM_DEFAULT);

  scan_for_nodes(node);

  wait_for_input(node, &options);

  return 0;
}
