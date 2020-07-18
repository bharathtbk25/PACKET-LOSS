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

#define VERSION "$Id: listener_tcp.c 6 2007-05-31 16:28:28Z  $"

void usage()
{
  fprintf(stderr, "usage: listener_tcp [-v] [-h] [-l listening_port] [-s sensitivity] [-r recover] [-c stats_packets]\n");
  fprintf(stderr, "       version: %s\n", VERSION);
  fprintf(stderr, "     -v  verbose (up to 3 times)\n");
  fprintf(stderr, "     -h  this page\n");
  fprintf(stderr, "     -l  TCP port to listen to (default %d)\n", MYPORT);
  fprintf(stderr, "     -s  sensitivity: how many us should we wait more than usual before declaring a delay (default 5000 = 5ms)\n");
  fprintf(stderr, "     -r  recover > total of variance of trip time (us) to consider stream back in sync (default 5000 = 5ms)\n");
  fprintf(stderr, "     -c  # of packets to be collected in the three statistical bins (default 100, makes 3 sec synchro)\n");

}

char* sync_state(int sync){
  // state: 0 - syncing, 1 - synced, 2 - delay detected, 3 - stream is back
  switch(sync){
  case 0:
    return("synching");
  case 1:
    return("synched");
  case 2:
    return("delay detected");
  case 3:
    return("stream is back");
  }
}
    
int main(int argc, char** argv)
{
  int sockfd;
  struct sockaddr_in my_addr;	// my address information
  struct sockaddr_in their_addr; // connector's address information
  socklen_t addr_len;
  int numbytes;
  char buf[MAXBUFLEN];
  fd_set rfds;
  int retval;

  char *parser;
  long seq, rseq;
  struct timeval delaytime, tv, otv, rtv, timeout, oldtimeout, temptrip, tempinter, trip[3], inter[3];
  char timebuf[30];
  long stat, dev;
  int i, statc;
  int trip_count[3], inter_count[3]; // sliding window, maintain two stat: trip time (or clock diff) and interarrival time
  int trip_index, inter_index;
  int sync; // state: 0 - syncing, 1 - synced, 2 - delay detected, 3 - stream is back
 
  int sensitivity=5000;
  unsigned short lport=MYPORT;
  int verbose=0;
  long recover=5000;
  int packet_count=100;
  
  int opt;
  extern char *optarg;
  extern int opterr;

  // make stdout unbuffered
  setvbuf(stdout, NULL, _IONBF, 0);
  
  seq=0;
  
  while ((opt = getopt(argc, argv, "vhl:s:r:c:")) != EOF) {
    switch (opt) {
    case 'h':
      usage();;
      exit(-1);
    case 'l':
      lport=atoi(optarg);
      break;
    case 's':
      sensitivity=atoi(optarg);
      break;
    case 'c':
      packet_count=atoi(optarg);
      break;
    case 'r':
      recover=atol(optarg);
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

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
  
  if(listen(sockfd, 1)==-1){
    perror("listen");
    exit(-1);
  }

  if(verbose)
    fprintf(stdout, "waiting for connection...\n");
  else
    fprintf(stderr, "waiting for connection...\n");
  
  
  addr_len = sizeof(struct sockaddr);
  if((sockfd=accept(sockfd, (struct sockaddr *)&their_addr, &addr_len)) == -1) {  // we don't reuse the original socket
    perror("accept");
    exit(-1);
  }


  if(verbose)
    printf("got connection from %s:%d on my port %d\n",inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port),lport);
  else
    fprintf(stderr, "connected!\n");
      
  if(gettimeofday(&otv, NULL)==-1){
    perror("gettimeofday");
    exit(-1);
  }

  for(i=0;i<3;i++){
    timerclear(&trip[i]);
    timerclear(&inter[i]);
    trip_count[i]=0;
    inter_count[i]=0;
  }

  trip_index=0;
  inter_index=0;

  sync=0; // state: 0 - syncing, 1 - synced, 2 - delay detected, 3 - stream is back

  if(verbose)
    printf("state: %s\n", sync_state(sync));

  buf[0]=0;
  while(strncmp(buf, "exit", 4)){

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);

    if(sync==0){
      //initial timeout, basically not relevant
      timeout.tv_sec=3600;
      timeout.tv_usec=0;
    }

    retval = select(sockfd+1, &rfds, NULL, NULL, &timeout);

    if (retval == -1){
      perror("select()");
      exit(-1);

    } else if (retval == 0) {
      // timeout has triggered, we stay in that state until we receive packets again
      // reset timeout
      memcpy(&timeout, &oldtimeout, sizeof(struct timeval));

      if(sync==1){
	// first time we enter this state
	if(gettimeofday(&delaytime, NULL)==-1){
	  perror("gettimeofday");
	  exit(-1);
	}
        sync=2;

	ctime_r(&delaytime.tv_sec,timebuf);
	timebuf[strlen(timebuf)-1]=0;        // remove \n
	
	printf("%s - delay_detected:%ld.%.6ld\n", timebuf, delaytime.tv_sec, delaytime.tv_usec);
	if(!verbose)
	  fprintf(stderr, "state: %s\n", sync_state(sync));
      }

      if(verbose)
	printf("state: %s\n", sync_state(sync));
      
      
    } else if (retval){
      // data is available now
      /* FD_ISSET(sockfd, &rfds) will be true. */
    
      addr_len = sizeof(struct sockaddr);
      if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			       (struct sockaddr *)&their_addr, &addr_len)) == -1) {
	perror("recvfrom");
	exit(1);
      } else if(numbytes == 0){
	if(gettimeofday(&tv, NULL)==-1){
	  perror("gettimeofday");
	  exit(-1);
	}
	
	ctime_r(&tv.tv_sec,timebuf);
	timebuf[strlen(timebuf)-1]=0;        // remove \n
	
	printf("%s - connection has been closed: %ld.%.6ld\n", timebuf, tv.tv_sec, tv.tv_usec);
	exit(0);
      }
      
      // capture receive time from local time
      if(gettimeofday(&tv, NULL)==-1){
	perror("gettimeofday");
	exit(-1);
      }
      ctime_r(&tv.tv_sec,timebuf);
      timebuf[strlen(timebuf)-1]=0;        // remove \n
      
      // extract sent time from packet
      parser = index(buf, '.');
      rtv.tv_usec = atol(parser+1);
      *parser = 0;
      rtv.tv_sec = atol(buf);
      *parser = '.';
      
      if(verbose>1)
	printf("%s - got packet from %s:%d on my port %d\n", timebuf, inet_ntoa(their_addr.sin_addr),ntohs(their_addr.sin_port),lport);
      if(verbose>1)
	printf("%s - packet is %d bytes long\n", timebuf, numbytes);
      buf[numbytes] = '\0';
      if(verbose>2)
	printf("%s - packet contains \"%s\" and is interpreted as %ld.%.6ld\n", timebuf, buf, rtv.tv_sec, rtv.tv_usec);      
      
      timersub(&tv, &rtv, &temptrip);
      timersub(&tv, &otv, &tempinter);
      
      memcpy(&otv, &tv, sizeof(struct timeval));

      // maintain trip stat
      timeradd(&trip[trip_index], &temptrip, &trip[trip_index]);
      trip_count[trip_index]++;
      
      if(trip_count[trip_index]==packet_count){
	if(trip_index<2){
	  trip_index++;
	} else {
	  trip_index=0;
	}
	// we have switch to a new bin, reset it.
	timerclear(&trip[trip_index]);
	trip_count[trip_index]=0;
      }
      
      // maintain inter stat
      timeradd(&inter[inter_index], &tempinter, &inter[inter_index]);
      inter_count[inter_index]++;
      
      if(inter_count[inter_index]==packet_count){
	if(inter_index<2){
	  inter_index++;
	} else {
	  inter_index=0;
	  // first time we have completed our stat, we can change the state
	  if(sync==0){
	    sync=1;
	    if(verbose){
	      printf("state: %s\n", sync_state(sync));
	    } else
	      fprintf(stderr, "state: %s\n", sync_state(sync));
	    
	  }
	}
	// we have switch to a new bin, reset it.
	timerclear(&inter[inter_index]);
	inter_count[inter_index]=0;
      }
      
      if(verbose>1){
	printf("%d: inter \t\t", sync);
	for(i=0;i<3;i++){
	  printf("%.7ld (%d)\t\t ", inter_count[i]?(inter[i].tv_sec*1000000+inter[i].tv_usec) / inter_count[i]:0, inter_count[i]);
	}
	printf("\n%d: trip \t\t", sync);
	for(i=0;i<3;i++){
	  printf("%.7ld (%d)\t\t ", trip_count[i]?(trip[i].tv_sec*1000000+trip[i].tv_usec) / trip_count[i]:0, trip_count[i]);
	}
	printf("\n");

      }
      
      switch(sync){
	
      case 0:
	//synching
	break;
	
      case 1:
	//synched, waiting for timeout to trigger, updating it based on the stat
	stat=0;
	statc=0;
	timerclear(&timeout);
	for(i=0;i<3;i++){
	  //	  printf("%d.%.6d (%d) + %d.%.6d = ", timeout.tv_sec, timeout.tv_usec, statc, inter[i].tv_sec, inter[i].tv_usec);
	  timeradd(&timeout, &inter[i], &timeout);
	  //	  printf("%d.%.6d ", timeout.tv_sec, timeout.tv_usec);
	  statc += inter_count[i];
	  //	  printf("(%d) \n", statc);
	}
	
	stat = (timeout.tv_sec * 1000000 + timeout.tv_usec)/statc;
	timeout.tv_sec = stat / 1000000;
	timeout.tv_usec = (stat - timeout.tv_sec * 1000000) + sensitivity;
	if(timeout.tv_usec > 1000000){
	  timeout.tv_sec++;
	  timeout.tv_usec -= 1000000;
	}
	memcpy(&oldtimeout, &timeout, sizeof(struct timeval));

	if(verbose)
	  printf("updated timeout to %ld.%.6ld\n", timeout.tv_sec, timeout.tv_usec);
	break;
	
      case 2:
	// we have detected a delay, and we are back as we have received a packet

	memcpy(&timeout, &oldtimeout, sizeof(struct timeval));

	timersub(&tv, &delaytime, &temptrip);

	printf("%s - initial_recover:%ld.%.6ld:%ld:(%ld.%.3ld)\n", timebuf, tv.tv_sec, tv.tv_usec, temptrip.tv_sec*1000000+temptrip.tv_usec, temptrip.tv_sec, temptrip.tv_usec/1000);

	sync=3;
	if(verbose){
	  printf("state: %s\n", sync_state(sync));
	} else
	  fprintf(stderr, "state: %s\n", sync_state(sync));
	
	break;

      case 3:
	// stream back, waiting for trip to come back
	// monitor three bins, when all almost equal we are back

	//don't forget to reset timeout

	memcpy(&timeout, &oldtimeout, sizeof(struct timeval));

	stat=0;
	statc=0;
	for(i=0;i<3;i++){
	  stat += trip[i].tv_sec*1000000 + trip[i].tv_usec;
	  statc += trip_count[i];
	}
	// get mean trip time
	stat= stat / statc;

	dev=0;
	for(i=0;i<3;i++){
	  if(trip_count[i])
	    dev += labs((trip[i].tv_sec*1000000 + trip[i].tv_usec)/trip_count[i] - stat);
	}
	
	if(dev < recover){
	  // total deviation is less than 5 ms (i.e. sum of dev to the mean)
	  // we are back in sync
	  sync=1;

	  timersub(&tv, &delaytime, &temptrip);
	  printf("%s - full_recover:%ld.%.6ld:%ld\n", timebuf, tv.tv_sec, tv.tv_usec, temptrip.tv_sec*1000000+temptrip.tv_usec);
	  fprintf(stderr, "state: %s\n", sync_state(sync));
	  if(verbose){
	    printf("state: %s\n", sync_state(sync));
	  } else
	    fprintf(stderr, "state: %s\n", sync_state(sync));
	  
	}	  
	break;
      
	
      } //end switch
      
      
    } // end received packet      
    
  } // end endless loop
  
  close(sockfd);
  
  return 0;
}
