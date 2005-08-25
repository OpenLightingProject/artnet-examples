
#include <artnet/artnet.h>
#include "artnet-test-defaults.h"

typedef struct {
	char shortName[ARTNET_SHORT_NAME_LENGTH] ;
	char longName[ARTNET_LONG_NAME_LENGTH] ;
	uint8_t	inAddr[ARTNET_MAX_PORTS] ;
	uint8_t outAddr[ARTNET_MAX_PORTS] ;
	uint8_t	subAddr ;
	artnet_port_command_t cmd ;
} address_test_params_t ;


typedef struct {
	address_test_params_t send;
	address_test_params_t check;
} address_test_t ;

/*
typedef struct {
	int a;
	int b;
} address_test_t ;
*/

extern address_test_t address_tests[] ;
