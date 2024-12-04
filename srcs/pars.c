
#include "pars.h"

int
pars_args(struct s_ping * ping, int ac, char ** av)
{
	int		i;
	size_t	size;
	char **	hostname_buf;

	i = 1;
	size = 0;
	while (i < ac)
	{
		if (av[i][0] == '-')
		{
			if (strcmp(av[i], "-?") == 0)
			{
				printf("Usage: ping [OPTION...] HOST ...\n");
				printf("Send ICMP ECHO_REQUEST packets to network hosts.\n");
				printf("Options:\n");
				printf("-v, verbose output\n");
				printf("-?, give this help list\n");
				return (-1);
			}
			else if (strcmp(av[i], "-v") == 0)
			{
				ping->flag |= PING_VERBOSE;
			}
			else
			{
				printf("ping: unknow option \'%s\'\n", av[i]);
				return (-1);
			}
		}	
		else
		{
			hostname_buf = malloc(sizeof(char *) * (size + 2));
			if (hostname_buf == NULL)
				return (-1);

			for (size_t j = 0; j < size; j++)
			{
				hostname_buf[j] = ping->hostname[j];
			}
			hostname_buf[size] = av[i];
			hostname_buf[size + 1] = NULL;

			free(ping->hostname);
			ping->hostname = hostname_buf;
			size++;
		}
		i++;
	}
	return (0);
}
