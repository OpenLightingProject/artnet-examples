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
 *
 *  Copyright 2004-2005 Simon Newton
 *  nomis52@westnet.com.au
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <artnet/artnet.h>

#define DEFAULT_DEVICE "/dev/dmx0"
#define DEFAULT_DEVICE2 "/dev/dmx1"
#define DEFAULT_DEVICE3 "/dev/dmx2"
#define DEFAULT_DEVICE4 "/dev/dmx3"

#define UNCONNECTED -1
#define SHORT_NAME "NWAN"
#define LONG_NAME "Netgear Wireless Artnet Node"


typedef struct {
  int verbose ;
  char *dev[ARTNET_MAX_PORTS];
  char *ip_addr ;
  int subnet_addr ;
  int port_addr ;
  int persist ;
  int num_ports;
  int fd[ARTNET_MAX_PORTS] ;
  uint8_t dmx[ARTNET_MAX_PORTS][ARTNET_DMX_LENGTH +1] ;
  char *short_name ;
  char *long_name ;
  char *config_file ;
} opts_t;

typedef struct {
  opts_t *ops ;
  int port_id ;
} thread_args_t ;


pthread_mutex_t mem_mutex = PTHREAD_MUTEX_INITIALIZER ;

int do_write(opts_t *ops, int pid, uint8_t *buf, int length) {
  int res ;

  res = write(ops->fd[pid], buf, length);

  if (res < 0){
    /* if you unplug devices from the dongle
     */
    if(ops->verbose)
      perror("Error writing to device");

    res = close(ops->fd[pid]) ;
    if(res < 0)
      perror("close ") ;
    else
      ops->fd[pid] = UNCONNECTED ;

    return -1 ;
  }

  if(res > 0 && ops->verbose)
    printf("p%d: 0x%02hhx, 0x%02hhx, 0x%02hhx, 0x%02hhx length %d\n", pid, buf[1], buf[2], buf[3], buf[4] , length) ;

  return 0;
}


void *thread_run(void *arg) {
  thread_args_t *args = (thread_args_t*) arg ;
  uint8_t buf[ARTNET_DMX_LENGTH+1] ;
  int *pid = &args->port_id ;

//  int len ;

  args->ops->fd[*pid] = open(args->ops->dev[*pid],O_WRONLY) ;

  while(1) {

    if(args->ops->fd[*pid] == UNCONNECTED) {
      sleep(1) ;

      args->ops->fd[*pid] = open(args->ops->dev[*pid],O_WRONLY) ;

      if(args->ops->fd[*pid] == -1 && args->ops->verbose)
        printf("open %s: %s\n",args->ops->dev[*pid], strerror(errno) ) ;

    } else {
      pthread_mutex_lock(&mem_mutex) ;
      memcpy(buf, args->ops->dmx[*pid], ARTNET_DMX_LENGTH+1) ;
      pthread_mutex_unlock(&mem_mutex) ;

      do_write(args->ops, *pid, buf, ARTNET_DMX_LENGTH+1) ;
  //    sleep(1) ;
    }
  }

/*      for(i = 513; i >=0; i--) {
        if(buff[i] != buf[i])
          break ;
        }

      if(i>0) {
*/
  return NULL ;

}

/*
 * Called when we have dmx data pending
 */
int dmx_handler(artnet_node n, int port, void *d) {
  uint8_t *data ;
  opts_t *ops = (opts_t *) d ;
  int len ;

  if(port < ops->num_ports) {
    data = artnet_read_dmx(n, port, &len) ;
    pthread_mutex_lock(&mem_mutex) ;
    memcpy(&ops->dmx[port][1], data, len) ;
    pthread_mutex_unlock(&mem_mutex) ;
  }
  return 0;
}



/*
 * saves the settings to the config file
 */
int save_config(opts_t *ops) {
  FILE *fh ;
  printf("in save config\n") ;
  if ((fh = fopen(ops->config_file, "w")) == NULL ) {
    perror("fopen") ;
    return -1 ;
  }

  fprintf(fh, "# artnet_usb config file\n") ;
  fprintf(fh, "Shortname=%s\n", ops->short_name) ;
  fprintf(fh, "Longname=%s\n", ops->long_name) ;
  fprintf(fh, "Subnet=%i\n", ops->subnet_addr) ;
  fprintf(fh, "Port=%i\n", ops->port_addr);

  fclose(fh) ;

  return 0 ;
}

/*
 * Load the settings from config file
 */
int load_config(opts_t *ops) {
  #define BUF_SIZE 1024
  FILE *fh ;
  char buf[1024], *c ;
  char *key, *data ;

  if(ops->config_file == NULL)
    return -1;

  if ((fh = fopen(ops->config_file, "r")) == NULL ) {
    perror("fopen") ;
    return -1 ;
  }

  while (  fgets(buf, 1024, fh) != NULL) {
    if(*buf == '#')
      continue ;

    // strip \n
    for(c = buf ; *c != '\n' ; c++) ;
      *c = '\0' ;

    key = strtok(buf, "=") ;
    data = strtok(NULL, "=") ;

    if(key == NULL || data == NULL)
      continue ;

    if(strcmp(key, "Shortname") == 0) {
      free(ops->short_name) ;
      ops->short_name = strdup(data) ;
    } else if(strcmp(key, "Longname") == 0) {
      free(ops->long_name) ;
      ops->long_name = strdup(data) ;
    } else if(strcmp(key, "Subnet") == 0) {
      ops->subnet_addr = atoi(data) ;
    } else if(strcmp(key, "Port") == 0 ) {
      ops->port_addr = atoi(data) ;
    }

  }

  fclose(fh) ;
  return 0 ;
}

/*
 * called when to node configuration changes,
 * we need to save the configuration to a file
 */
int program_handler(artnet_node n, void *d) {
  opts_t *ops = (opts_t*) d ;
  artnet_node_config_t config ;

  artnet_get_config(n, &config) ;

  free(ops->short_name) ;
  free(ops->long_name) ;

  ops->short_name = strdup(config.short_name) ;
  ops->long_name = strdup(config.long_name) ;
  ops->subnet_addr = config.subnet ;
  ops->port_addr = config.out_ports[0] ;

  save_config(ops) ;
  return 0 ;
}


/*
 * Main function
 *
 */
int run(opts_t *ops) {
  artnet_node node ;
  pthread_t tid[ARTNET_MAX_PORTS] ;
  thread_args_t targ[ARTNET_MAX_PORTS] ;
  int i ;

  load_config(ops) ;

  // create new artnet node, and set config values
    node = artnet_new(ops->ip_addr, ops->verbose) ; ;

  artnet_set_short_name(node, ops->short_name) ;
  artnet_set_long_name(node, ops->long_name) ;
  artnet_set_node_type(node, ARTNET_NODE) ;

  artnet_set_subnet_addr(node, ops->subnet_addr) ;

  // we want to be notified when the node config changes
  artnet_set_program_handler(node, program_handler, (void*) ops) ;
  artnet_set_dmx_handler(node, dmx_handler, (void*) ops) ;

  for(i=0; i < ops->num_ports ; i++) {
    // set the first port to output dmx data
    artnet_set_port_type(node, i, ARTNET_ENABLE_OUTPUT, ARTNET_PORT_DMX) ;

    // set the universe address of the first port
    artnet_set_port_addr(node, i, ARTNET_OUTPUT_PORT, ops->port_addr+i) ;

    targ[i].ops = ops ;
    targ[i].port_id = i ;

    if( pthread_create(&tid[i], NULL, &thread_run, (void*) &targ[i]) ) {
      printf("pthread failed\n") ;
    }
  }

  artnet_start(node) ;

  while(1) {
    // set a 1 second timeout on the read
    // this way we send a DMX frame every second
    // even if we don't get any ArtNet packets
    artnet_read(node, 1) ;

  }

  return 0 ;
}


/*
 * Set our default options, command line args will overide this
 */
void init_ops(opts_t *ops) {
  int i ;

  ops->verbose = 0;

  ops->ip_addr = NULL ;
  ops->subnet_addr = 0 ;
  ops->port_addr = 0 ;
  ops->persist = 0;
  ops->num_ports = 1;

  for(i=0; i < ARTNET_MAX_PORTS; i++) {
    ops->fd[i] = UNCONNECTED ;
    memset(ops->dmx[i], 0x00, ARTNET_DMX_LENGTH+1) ;
  }
  // dodgy but saves using vasprintf
  ops->dev[0] = strdup(DEFAULT_DEVICE) ;
  ops->dev[1] = strdup(DEFAULT_DEVICE2) ;
  ops->dev[2] = strdup(DEFAULT_DEVICE3) ;
  ops->dev[3] = strdup(DEFAULT_DEVICE4) ;

  ops->short_name = strdup(SHORT_NAME) ;
  ops->long_name = strdup(LONG_NAME) ;
  ops->config_file = NULL ;
}

/*
 * Parse command lines options and save in opts_s struct
 *
 */
void parse_args(opts_t *ops, int argc, char *argv[]) {
  // parse args
  int optc, port_addr, subnet_addr, num_ports ;

  // parse options
  while ((optc = getopt (argc, argv, "c:s:p:d:a:vn:z")) != EOF) {
    switch (optc) {
       case 'a':
        free(ops->ip_addr) ;
        ops->ip_addr = (char *) strdup(optarg) ;
            break;
       case 'c':
        free(ops->config_file) ;
        ops->config_file = (char *) strdup(optarg) ;
            break;
      case 'v':
        ops->verbose = 1 ;
            break;
      case 'd':
        free(ops->dev) ;
        ops->dev[0] = (char *) strdup(optarg) ;
            break;
      case 's':
        subnet_addr = atoi(optarg) ;

        if(subnet_addr < 0 && subnet_addr > 15)
          printf("Subnet address must be between 0 and 15\n") ;
        else
          ops->subnet_addr = subnet_addr ;
        break ;
      case 'n':
        num_ports = atoi(optarg) ;

        if(num_ports < 1 || num_ports > ARTNET_MAX_PORTS) {
          printf("number of ports must be between 1 and %d\n", ARTNET_MAX_PORTS) ;
        } else
          ops->num_ports = num_ports ;
        break ;
      case 'p':
        port_addr = atoi(optarg) ;

        if(port_addr < 0 && port_addr > 15)
          printf("Port address must be between 0 and 15\n") ;
        else
          ops->port_addr = port_addr ;
        break ;
      case 'z':
        ops->persist = 1 ;
          default:
        break;
      }
  }
}


int main(int argc, char *argv[]) {
  int statloc, pid ;
  opts_t ops ;

  init_ops(&ops) ;
  parse_args(&ops, argc, argv) ;

  // we don't load ops here
  // else if we restart and the options have been remote
  // programed we'll have to old ones

  if(ops.persist) {
    while(1) {
      if( (pid = fork() ) == 0) {
        // child ;
        run(&ops) ;
        exit(1) ;
      } else {
        // parent
        pid = wait(&statloc) ;
        printf("Child %i terminated, restarting\n", pid) ;
      }
    }
  } else
    run(&ops) ;

  return 0;
}

