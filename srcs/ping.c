
#include "ping.h"
#include "pars.h"

int	running = 1;

static void
sig_stop_running(int sig)
{
(void)sig;
	running = 0;
	return ;
}

static int
get_target_info(struct s_target * target, char * hostname)
{
	struct timeval			timeout;
	struct addrinfo			hints;
	struct addrinfo *		host;
	struct sockaddr_in *	addr;
	char *					ip;
	const int				on = 1;
	int						err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	err = getaddrinfo(hostname, NULL, &hints, &host);
	if (err != 0)
	{
		printf("ping: unknown host\n");
		return (-1);
	}

	target->socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
	if (target->socket_fd < 0)
	{
		printf("ping: Lacking privilege for icmp\n");
		free(host);
		return (-1);
	}

	// set socket option for timeout in case we recv nothing
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;
	err = setsockopt(target->socket_fd, IPPROTO_IP, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
	if (err != 0)
	{
		printf("ping: setsockopt() RCVTIMEO failed\n");
		close(target->socket_fd);
		free(host);
		return (-1);
	}
	
	// set socket option for timeout in case we send nothing
	err = setsockopt(target->socket_fd, IPPROTO_IP, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));
	if (err != 0)
	{
		printf("ping: setsockopt() SNDTIMEO failed\n");
		close(target->socket_fd);
		free(host);
		return (-1);
	}

	// set socket options for recv hops limit
	err = setsockopt(target->socket_fd, IPPROTO_IP, IP_RECVTTL, &on, sizeof(on));
	if (err != 0)
	{
		printf("ping: setsockopt() RECVTTL failed\n");
		close(target->socket_fd);
		free(host);
		return (-1);
	}

	// fill target struct
	addr = (struct sockaddr_in *)host->ai_addr;
	ip = strdup(inet_ntoa(addr->sin_addr));
	target->ip = ip;
	target->addr = *addr;
	target->hostname = hostname;
	free(host);

	if (ip == NULL)
	{
		printf("strdup() fail\n");
		close(target->socket_fd);
		return (-1);
	}
	return (0);
}

static int
ping_send(struct s_ping ping, struct s_target target, struct s_timer * timer, size_t * n_pckt_send)
{
	struct ping_pckt	pckt;
	int					err;

	// setup packet for a echo call
	bzero(&pckt, sizeof(pckt));
	pckt.hdr.type = ICMP_ECHO;
	pckt.hdr.un.echo.id = ping.pid;
	for (size_t i = 0; i < sizeof(pckt.msg) - 1; i++)
		pckt.msg[i] = i + '0';
	pckt.msg[sizeof(pckt.msg) - 1] = '\0';
	pckt.hdr.un.echo.sequence = *n_pckt_send % 0xffff;
	// The checksum will be calculated by the TCP/IP stack
	pckt.hdr.checksum = 0;

	*n_pckt_send += 1;
	// start timer and send pckt
	timer_start(timer);
	err = sendto(target.socket_fd, &pckt, sizeof(pckt), 0, (const struct sockaddr *)&target.addr, sizeof(const struct sockaddr));
	if (err == 0)
	{
		if ((ping.flag & PING_VERBOSE) == PING_VERBOSE)
			printf("Request send fail for icmp_seq %ld\n", *n_pckt_send - 1);
		return (-1);
	}
	return (0);
}

static int
ping_recv(struct s_ping ping, struct s_target target, struct s_timer * timer, double * sq_total_time_ms, const size_t n_pckt_send, size_t * n_pckt_recv)
{
	struct ping_pckt	pckt;
	double				time_ms;
	int					recv_value;
	int					hops;
	uint16_t			pckt_n;
	struct msghdr		msg;
	struct cmsghdr *	cmsg;
	struct iovec		iov[1];
	char				ctrl_buf[1024];
	char				recv_buf[1024];

	// setup msg handler
	iov[0].iov_base = recv_buf;
	iov[0].iov_len = sizeof(recv_buf);
	msg.msg_name = &(target.addr);
	msg.msg_namelen = sizeof(target.addr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = &ctrl_buf;
	msg.msg_controllen = sizeof(ctrl_buf);

	// zero buffer (not needed and take time)
	// memset(recv_buf, 0, sizeof(recv_buf));
	// memset(ctrl_buf, 0, sizeof(ctrl_buf));

	// recv pckt and end timer
	recv_value = recvmsg(target.socket_fd, &msg, 0);
	timer_end(timer);
	if (recv_value <= 0)
	{
		if ((ping.flag & PING_VERBOSE) == PING_VERBOSE)
			printf("Request timeout for icmp_seq %ld\n", n_pckt_send - 1);
		return (-1);
	}
	else if (recv_value < PING_PCKT_SIZE)
	{
		if ((ping.flag & PING_VERBOSE) == PING_VERBOSE)
			printf("Recv wrong packet size for icmp_seq %ld\n", n_pckt_send - 1);
		return (-1);
	}

	pckt = *(struct ping_pckt *)recv_buf;
	pckt_n = pckt.hdr.un.echo.sequence;

	if (pckt.hdr.type != 0 && pckt.hdr.type != 8)
	{
		if ((ping.flag & PING_VERBOSE) == PING_VERBOSE)
			printf("Recv wrong packet type for icmp_seq %d\n", pckt_n);
		return (-1);
	}
	else if (pckt.hdr.code != 0)
	{
		if ((ping.flag & PING_VERBOSE) == PING_VERBOSE)
			printf("Recv wrong packet code for icmp_seq %d\n", pckt_n);
		return (-1);
	}

	// check msg intergrity
	for (size_t i = 0; i < sizeof(pckt.msg) -1; i++)
	{
	 	if (pckt.msg[i] != (char)(i + '0'))
	 	{
	 		if ((ping.flag & PING_VERBOSE) == PING_VERBOSE)
				printf("Error in payload for icmp_seq %d\n", pckt_n);
			return (-1);
	 	}
	}

	// Calculate trip time
	timer_push_last(timer);
	time_ms = timer->last_time.tv_sec * 1000;
	time_ms += (double)timer->last_time.tv_nsec / 1000000.0;

	// add to sq_total_time_ms for Standard Deviation
	*sq_total_time_ms += time_ms * time_ms;

	// Get packet info, hops (ttl)
	hops = -1;
	cmsg = CMSG_FIRSTHDR(&msg);
	while (cmsg != NULL)
	{
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL)
		{
			hops = *(int *)CMSG_DATA(cmsg);
		}
		cmsg = CMSG_NXTHDR(&msg, cmsg);
	}
	
	// print echo request info
	printf("%d bytes from %s: ", recv_value, target.ip);
	printf("icmp_seq=%hu ttl=%d ", pckt_n, hops);
	printf("time=%.3lf ms\n", time_ms);
	*n_pckt_recv += 1;
	return (0);
}

static void
print_ping_stats(struct s_target target, struct s_timer timer, double sq_total_time_ms, size_t n_pckt_send, size_t n_pckt_recv)
{
	double	min_time_ms;
	double	avg_time_ms;
	double	max_time_ms;
	double	stddev_time_ms;

	min_time_ms = timer.min_time.tv_sec * 1000;
	min_time_ms += (double)timer.min_time.tv_nsec / 1000000.0;

	max_time_ms = timer.max_time.tv_sec * 1000;
	max_time_ms += (double)timer.max_time.tv_nsec / 1000000.0;

	avg_time_ms = timer.total_time.tv_sec * 1000;
	avg_time_ms += (double)timer.total_time.tv_nsec / 1000000.0;
	if (n_pckt_recv != 0)
		avg_time_ms /= n_pckt_recv;
	else
		avg_time_ms = 0;

	//	Standard deviation
	if (n_pckt_recv != 0)
		stddev_time_ms = sqrt(sq_total_time_ms / (double)n_pckt_recv - avg_time_ms * avg_time_ms);
	else
		stddev_time_ms = 0;

	printf("--- %s ping statistics ---\n", target.hostname);
	printf("%ld packets transmitted, %ld packets received, ", n_pckt_send, n_pckt_recv);
	if (n_pckt_send != 0)
		printf("%u%% packet loss\n", (unsigned int)((n_pckt_send - n_pckt_recv) * 100 / n_pckt_send));
	else
		printf("0%% packet loss\n");
	printf("round-trip min/avg/max/stddev = ");
	printf("%.3lf/%.3lf/%.3lf/%.3lf ms\n", min_time_ms, avg_time_ms, max_time_ms, stddev_time_ms);
	return ;
}

static void
ping_run(struct s_ping ping, struct s_target target)
{
	struct s_timer	timer;
	int				err;
	size_t			n_pckt_send;
	size_t			n_pckt_recv;
	double			sq_total_time_ms;

	// init count
	n_pckt_send = 0;
	n_pckt_recv = 0;
	sq_total_time_ms = 0;

	// init timer
	timer_init(&timer);

	// print ping info
	if ((ping.flag & PING_VERBOSE) == PING_VERBOSE)
	{
		printf("PING %s (%s): ", target.hostname, target.ip);
		printf("%ld data bytes, ", ping.pckt_size - sizeof(struct icmphdr));
		printf("id 0x%x = %u\n", ping.pid, ping.pid);
	}
	else
	{
		printf("PING %s (%s): ", target.hostname, target.ip);
		printf("%ld data bytes\n", ping.pckt_size - sizeof(struct icmphdr));
	}

	// ping loop
	do
	{
		err = ping_send(ping, target, &timer, &n_pckt_send);
		if (err != 0)
			continue ;

		if (running == 0)
			break ;

		ping_recv(ping, target, &timer, &sq_total_time_ms, n_pckt_send, &n_pckt_recv);

		// sleep betwen echo request
		usleep(PING_SLEEP_TIME);

	} while (running == 1);

	print_ping_stats(target, timer, sq_total_time_ms, n_pckt_send, n_pckt_recv);
	return ;
}


static void
ping_init(struct s_ping * ping)
{
	ping->hostname = NULL;
	ping->pid = getpid() & 0xffff;
	ping->pckt_size = PING_PCKT_SIZE;
	ping->flag = 0;
	return ;
}

int
main(int ac, char ** av)
{
	if (ac < 2)
	{
		printf("ping: missing host operand\n");
		printf("Try 'ping -?' for more information.\n");
		return (64);
	}

	struct s_target	target;
	struct s_ping	ping;
	int				err;
	size_t			i;

	ping_init(&ping);

	err = pars_args(&ping, ac, av);
	if (err != 0)
		return (0);

	signal(SIGINT, sig_stop_running);

	i = 0;
	while (ping.hostname != NULL && ping.hostname[i] != NULL)
	{
		err = get_target_info(&target, ping.hostname[i]);
		if (err != 0)
		{
			free(ping.hostname);
			return (1);
		}
		ping_run(ping, target);
		close(target.socket_fd);
		free(target.ip);
		i++;
	}
	free(ping.hostname);
	return (0);
}
