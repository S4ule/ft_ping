#ifndef __PING_H__
# define __PING_H__ 1

#include "timer.h"

#include <stdio.h>	// printf family
#include <string.h>	// string family
#include <signal.h>	// signal()
#include <stdlib.h>	// free()
#include <stdbool.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>	// inet_ntoa

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#include <netinet/ip_icmp.h>
#include <unistd.h>

#include "sqrt.h"

# define PING_PCKT_SIZE 64
# define TIMEOUT 10
# define PING_SLEEP_TIME 1000000
# define DEFAULT_TTL 64

/*


- 32bits

Header
(20 bytes)
8 Version/IHL | 8 Type of service (ToS) | 16 Length
16 Identification | 16 flags and offset
8 Time to live (TTL) | 8 Protocol | 16 Header checksum 
32 Source IP address
32 Destination IP address

ICMP header
(8 bytes) 
8 Type of message | 8 Code | 16 Checksum
16 Identifier | 16 Sequence Number 


Payload data *

*/

# define PING_VERBOSE 1


struct s_target
{
	int					socket_fd;
	char *				ip;
	char *				hostname;
	struct sockaddr_in	addr;
};

struct s_ping
{
	char **				hostname;
	size_t				hostname_size;
	int					pid;
	size_t				pckt_size;
	unsigned int		flag;
};

struct ping_pckt
{
	struct icmphdr	hdr;
	char			msg[PING_PCKT_SIZE - sizeof(struct icmphdr)];
};


#endif	// __PING_H__
