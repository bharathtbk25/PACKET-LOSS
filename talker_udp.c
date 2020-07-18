#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h>

#define SERVERPORT 4950	// the port users will be connecting to
#define SOURCEPORT 1234

#define VERSION "$Id $"

void usage()
{
  fprintf(stderr, "usage: talker_udp [-v] [-h] [-s source_port] [-d destination_port]  [-i start_seq] [-c count] [-t interval] hostname\n");
  fprintf(stderr, "       version: %s\n", VERSION);
  fprintf(stderr, "     -v  verbose \n");
  fprintf(stderr, "     -h  this page\n");
  fprintf(stderr, "     -s  source UDP port (default %d)\n", SOURCEPORT);
  fprintf(stderr, "     -d  destination UDP port (default %d)\n", SERVERPORT);
  fprintf(stderr, "     -i  first sequence number to send (default 0)\n");
  fprintf(stderr, "     -c  send 'count' packets (default 0 = unlimited)\n");
  fprintf(stderr, "     -t  send a packet every t usec (default 10000 = 10ms)\n");
  fprintf(stderr, "     -D  debug mode: suppress packets with 5<seq<10\n");

}


int main(int argc, char *argv[])
{
  int sockfd;
  struct sockaddr_in their_addr; // connector's address information
  struct sockaddr_in my_addr;     // my address information
  struct hostent *he;
  int numbytes;
  struct timeval tv;
  struct timeval timeout;
  char timestring[20];
  
  int count=0;
  int unlimited=1;
  long seq=0;
  long usectime=10000;
  long debug=0;
  
  unsigned short sport=SOURCEPORT; //the source port used to send the packet
  unsigned short dport=SERVERPORT;
  int verbose=0;
  
  int opt;
  extern char *optarg;
  extern int opterr;

  // make stdout unbuffered
  setvbuf(stdout, NULL, _IONBF, 0);
  
  
  while ((opt = getopt(argc, argv, "vhs:d:i:c:t:D")) != EOF) {
    switch (opt) {
    case 'h':
      usage();;
      exit(-1);
    case 'D':
      debug=1;
      break;
    case 's':
      sport=atoi(optarg);
      break;
    case 'd':
      dport=atoi(optarg);
      break;
    case 'c':
      count=atoi(optarg);
      if(count != 0)
	unlimited=0;
      break;
    case 'i':
      seq=atol(optarg);
      break;
    case 't':
      usectime=atol(optarg);
      break;
    case 'v':
      verbose++;
      break;
    default:
      usage();
      exit(-1);
    }
  }
  
  
  if(optind == argc){
    fprintf(stderr,"error: missing hostname \n");
    usage();
    exit(-1);
  }
  
  if ((he=gethostbyname(argv[optind])) == NULL) {  // get the host info
    perror("gethostbyname");
    exit(1);
  }
  
  
  // done with the options
  
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  
  my_addr.sin_family = AF_INET;            // host byte order
  my_addr.sin_port = htons(sport);        // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
  
  if (bind(sockfd, (struct sockaddr *)&my_addr,
	   sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }
  
  // send to host
  their_addr.sin_family = AF_INET;	 // host byte order
  their_addr.sin_port = htons(dport); // short, network byte order
  their_addr.sin_addr = *((struct in_addr *)he->h_addr);
  memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct
  
  while( unlimited || count-- ){
    
    timeout.tv_sec = 0;
    timeout.tv_usec = usectime; // default 100 msec
    
    if (select(0, NULL, NULL, NULL, &timeout)==-1){
      perror("select ");
      exit(-1);
    } 
    // timeout has triggered, or a signal has been received. We don't care...
    
    // marshalling
    snprintf(timestring, 19, "%ld", seq++);
    
    if( !debug || ((seq>10) || (seq<=5))){
      if ((numbytes = sendto(sockfd, timestring, strlen(timestring), 0,
			     (struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) {
	perror("sendto");
	exit(1);
      }
      
      if(verbose)
	printf("sent %s: %d bytes to %s\n", timestring, numbytes, inet_ntoa(their_addr.sin_addr));
    }
  }
  
  close(sockfd);
  
  return 0;
}
