#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include "pthread.h"
#include <err.h>
#include <netdb.h>

#define UDP_MAXIMUM_SIZE 1316 /* theoretical maximum size */
#define TS_PACKET_SIZE 188
#define TIME_STAMP_SIZE 5
#define SYSTEM_CLOCK_FREQUENCY 27000000
const long long unsigned system_frequency = 27000000.0;

void print_cmdline()
{
  printf(
    "Usage: APP [<In_IP>] [<In_Port>] [<InterfaceIP>] [<InterfaceName>] \n"
    "\n"
    );
  printf("ts2es Dev. Praveen Khare");
  
}

int main(int argc, char* argv[]) 
{
	if (argc < 4)
	{
		print_cmdline();
		return 0;
	}
	char* multi_address ;
	char* multi_port ;
	char* interface_address ;
	char* interface_name ;
	unsigned short pid;
	unsigned short payload_pid;
	unsigned int i;
	unsigned char* packet_buffer;
	unsigned char* current_packet;

	unsigned long long int pcr[2]={0,0};
	unsigned long long int pcr_base = 0;
	unsigned long long int ts_packet_count=0;
	int pcr_ext = 0;
	int PCR_found = 0;
	double pcrtime1 = 0;
	double pcrtime2 = 0;
	double bitrate_cal = 0;

	multi_address = argv[1];
	multi_port = argv[2];
	interface_address = argv[3];
	interface_name = argv[4];

	#define BUFFERSIZE (1000)
	char buffer[BUFFERSIZE];
	socklen_t opt_length;
  
    int sockfd;
    struct sockaddr_in addr;
    struct ip_mreq mgroup;
    int reuse;
    unsigned int addrlen;
    int len;
    unsigned char udp_packet[UDP_MAXIMUM_SIZE];

	memset((char *) &mgroup, 0, sizeof(mgroup));
	mgroup.imr_multiaddr.s_addr = inet_addr(multi_address);         
	mgroup.imr_interface.s_addr = inet_addr(interface_address);//INADDR_ANY;
	memset((char *) &addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(atoi(multi_port));
	addr.sin_addr.s_addr = inet_addr(multi_address);
	addrlen = sizeof(addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
		perror("socket(): error ");
		return 0;
    }
    
    reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
		perror("setsockopt() SO_REUSEADDR: error ");
    }

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("bind(): error");
		close(sockfd);
		return 0;
    }
    
     /* bind the socket to one network device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, interface_name, sizeof(interface_name)) != 0) {
		printf ("%s: could not set SO_BINDTODEVICE (%s)\n",
		argv[0], strerror(errno));
		exit (EXIT_FAILURE);
	}
	else{
		printf ("SO_BINDTODEVICE set\n");
	}
	
	/* verify SO_BINDTODEVICE setting */
	if (getsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)buffer, &opt_length) != 0) {
		printf ("%s: could not get SO_BINDTODEVICE (%s)\n",
		argv[0], strerror(errno));
//		exit (EXIT_FAILURE);
	}
	else{
		printf("SO_BINDTODEVICE is: %s\n", buffer);
	}
   
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mgroup, sizeof(mgroup)) < 0) {
		perror("setsockopt() IPPROTO_IP: error ");
		close(sockfd);
	return 0;
    }
    while(1) {
		len = recvfrom(sockfd, udp_packet, UDP_MAXIMUM_SIZE, 0, (struct sockaddr *) &addr,&addrlen);
	if (len < 0) 
	{
	    perror("recvfrom(): error ");
		} else {
		   packet_buffer = udp_packet;
			/* check packets pid */
			for (i = 0; i < len ; i+=TS_PACKET_SIZE) {
				current_packet = packet_buffer+i;
				memcpy(&pid, current_packet + 1, 2);	
				pid = ntohs(pid);
				pid = pid & 0x1fff;
				ts_packet_count++; //TS bitrate
				
				if(PCR_found == 0)
				{	
					if ( (current_packet[3] & 0x20) && (current_packet[4] != 0) && (current_packet[5] & 0x10))  /* there is PCR */ 
						 { 
							 PCR_found = 1;
							 fprintf(stderr, " first PCR found at PID=%d\n", pid); 
						 }
					payload_pid = pid;	
				}
				else
				{
					if (pid == payload_pid) /* got the pid we are interested into */
					{ 
						if ( (current_packet[3] & 0x20) && (current_packet[4] != 0) && (current_packet[5] & 0x10))  /* there is PCR */ 
						 { 
							pcr_base = (((unsigned long long int)current_packet[6]) << 25) + (current_packet[7] << 17) + (current_packet[8] << 9) + (current_packet[9] << 1) + (current_packet[10] >> 7);
							pcr_ext = ((current_packet[10] & 1) << 8) + current_packet[11];
							pcr[0] = pcr[1];
							pcr[1] = (pcr_base * 300 + pcr_ext);
							pcrtime1 = ((double)(pcr[0]) / SYSTEM_CLOCK_FREQUENCY);
							pcrtime2 = ((double)(pcr[1]) / SYSTEM_CLOCK_FREQUENCY);
							bitrate_cal = (ts_packet_count * 188 * 8) / (pcrtime2 - pcrtime1);
							fprintf(stderr, " Bitrate =%f \r", bitrate_cal/1000);
							fflush(stdout);
							ts_packet_count = 0;
						}
					}//if-pid
				}//pcrfound-else
			}//for
		}//if-else
    }//while
}//main


