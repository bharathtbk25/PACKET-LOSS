#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>


#define MYPORT 4950	// the port users will be connecting to

#define MAXBUFLEN 100

#define VERSION "$Id: listener_udp.c 6 2007-05-31 16:28:28Z  $"

void usage()
{
  fprintf(stderr, "usage: listener_udp [-v] [-h] [-l listening_port] [-i expected_seq]\n");
  fprintf(stderr, "       version: %s\n", VERSION);
  fprintf(stderr, "     -v  verbose (up to 3 times)\n");
  fprintf(stderr, "     -h  this page\n");
  fprintf(stderr, "     -l  UDP port to listen to (default %d)\n", MYPORT);
  fprintf(stderr, "     -i  first sequence number to be expected (default 0)\n");

}


int main(int argc, char** argv)
{
  int sockfd;
  struct sockaddr_in my_addr;	// my address information
  struct sockaddr_in their_addr; // connector's address information
  socklen_t addr_len;
  int numbytes;
  char buf[MAXBUFLEN];
  long seq, rseq;
  struct timeval tv, rtv, interval;
  char timebuf[30];

  unsigned short lport=MYPORT;
  int verbose=0;
  
  int opt;
  extern char *optarg;
  extern int opterr;
  
  seq=0;

  // make stdout unbuffered
  setvbuf(stdout, NULL, _IONBF, 0);
  
  while ((opt = getopt(argc, argv, "vhl:i:")) != EOF) {
    switch (opt) {
    case 'h':
      usage();;
      exit(-1);
    case 'l':
      lport=atoi(optarg);
      break;
    case 'i':
      seq=atol(optarg);
      break;
    case 'v':
      verbose++;
      break;
    default:
      usage();
      exit(-1);
    }
  }
  
  
  // actual main
  
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }
  
  
  my_addr.sin_family = AF_INET;		 // host byte order
  my_addr.sin_port = htons(lport);	 // short, network byte order
  my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
  memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
  
  if (bind(sockfd, (struct sockaddr *)&my_addr,
	   sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }
  
  if(gettimeofday(&tv, NULL)==-1){
    perror("gettimeofday");
    exit(-1);
  }
     
  
  buf[0]=0;
  while(strncmp(buf, "exit", 4)){
    
    addr_len = sizeof(struct sockaddr);
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			     (struct sockaddr *)&their_addr, &addr_len)) == -1) {
      perror("recvfrom");
      exit(1);
    }

    // capture receive time from local time
    if(gettimeofday(&rtv, NULL)==-1){
      perror("gettimeofday");
      exit(-1);
    }
    ctime_r(&rtv.tv_sec,timebuf);
    timebuf[strlen(timebuf)-1]=0;        // remove \n

    
    if(verbose>2)
      printf("got packet from %s:%d on my port %d\n",inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port),lport);
    if(verbose>2)
      printf("packet is %d bytes long\n",numbytes);
    buf[numbytes] = '\0';
    if(verbose>1)
      printf("packet contains rseq \"%s\" looking for seq %ld\n", buf, seq);
    

    rseq=atol(buf);
    
    if(rseq==seq){
      //expected packet is here
      seq++;
      memcpy((void*)&tv, (void*)&rtv, sizeof(struct timeval));
      
    } else if (rseq>seq) {
    //} else if (rseq> (seq+1) ) {
      // remote is farther as us, some packets must have been lost
      // but we are receiving them again
      timersub (&rtv, &tv, &interval);
      
      if(verbose)
	printf("%s - lost %ld packets from %ld.%.6ld to %ld.%.6ld (%ld.%.6ld sec)\n", timebuf, rseq - seq, tv.tv_sec, tv.tv_usec, rtv.tv_sec, rtv.tv_usec,interval.tv_sec, interval.tv_usec);
      else if ((rseq - seq)>1)
	printf("%s - lost:%ld;%ld.%.6ld;%ld.%.6ld;%ld.%.6ld\n", timebuf, rseq - seq, tv.tv_sec, tv.tv_usec, rtv.tv_sec, rtv.tv_usec,interval.tv_sec, interval.tv_usec);
      
      seq=rseq+1;
      memcpy((void*)&tv, (void*)&rtv, sizeof(struct timeval));
      
    } else {
      //rseq is < than seq, out of order?
      
      if(verbose)
	printf("got %ld at %ld.%.6ld, but was waiting for %ld\n", rseq, rtv.tv_sec, rtv.tv_usec, seq);
      else
	printf("OoO:%ld.%.6ld;%ld;%ld\n", rtv.tv_sec, rtv.tv_usec, rseq, seq);
    }
    
  }
  
  close(sockfd);
  
  return 0;
}
