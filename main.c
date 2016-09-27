#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/wait.h>

#define CLONEDEV "/dev/net/tun"

/*
 * code for tun_alloc shamelessly copied from here:
 * http://backreference.org/2010/03/26/tuntap-interface-tutorial/
 */
int tun_alloc(char *dev, int flags)
{
	struct ifreq ifr;
	int fd, err;
	
	/* open the clone device */
	fd = open(CLONEDEV, O_RDWR);
	if (fd < 0)
		return fd;
	
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */
	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	
	err = ioctl(fd, TUNSETIFF, (void *)&ifr);
	if (err < 0)
	{
		close(fd);
		return err;
	}
	
	strcpy(dev, ifr.ifr_name);
	
	return fd;
}

int main(int argc, char **argv)
{
	int fd;
	char tun_name[IFNAMSIZ];
	
	if (argc < 3)
	{
		fprintf(stderr, "usage: std2tun <ifconfig args> <callee>\n");
		exit(EXIT_FAILURE);
	}
	
	/* tun */
	{
		strcpy(tun_name, "");
		
		fd = tun_alloc(tun_name, IFF_TUN | IFF_NO_PI);
		if (fd < 0)
		{
			perror("tun_alloc");
			exit(EXIT_FAILURE);
		}
	}
	
	//printf("using tun device: %s\n", tun_name);
	
	/* ifconfig */
	{
		char ifconfig_cmd[strlen("ifconfig ") + IFNAMSIZ + strlen(" ") + strlen(argv[1]) + 1];
		
		sprintf(ifconfig_cmd, "ifconfig %s %s", tun_name, argv[1]);
		if (system(ifconfig_cmd) < 0)
		{
			perror("system(ifconfig)");
			exit(EXIT_FAILURE);
		}
	}
	
	
	if (dup2(fd, STDIN_FILENO) < 0 || dup2(fd, STDOUT_FILENO) < 0)
	{
		perror("dup2");
		exit(EXIT_FAILURE);
	}
	
	execvp(argv[2], &argv[2]);
	/* execvp failed if this point is reached */
	perror("execvp");
	
	return EXIT_FAILURE;
}
