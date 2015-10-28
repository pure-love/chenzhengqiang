#include "ortp_recv.h"
#include <ortp/ortp.h>
#include <cstdlib>

static const char *help="usage: rtprecv  filename local_port\n";
static const int BUFFER_SIZE = 160;
void check_cmd_args(int argc, char **argv)
{
    if ( argc<3 )
    {
         printf("%s",help);
	  exit(EXIT_FAILURE);
    }
    if (atoi(argv[2]) <=0) 
    {
	   printf("%s",help);
	  exit(EXIT_FAILURE);
    }
}

int main(int argc, char*argv[])
{
	RtpSession *session0;
	unsigned char buffer[BUFFER_SIZE];
	FILE *outfile;
	check_cmd_args(argc,argv);
	outfile=fopen(argv[1],"wb");
	if (outfile==NULL)
      {
	    perror("Cannot open file for writing");
	    return -1;
      }
      int total_bytes = 0;
      unsigned int timestamp0 = 0;
      session0 = oRTP_recv_init(atoi(argv[2]),atoi(argv[2])+1,0);
      while( true)
      {
          total_bytes = oRTP_recv_now(session0,buffer,BUFFER_SIZE,timestamp0);
          if( total_bytes == -1 )
          {
                    printf("buffer size is too small\n");
                    continue;
          }
          else if( total_bytes > 0)
          printf("session0:received %d bytes timestamp:%u\n",total_bytes,timestamp0);
      }
      /*
      SessionSet *session_set = session_set_new();
	session0 = oRTP_recv_init(54320,54321,0);
       session13=oRTP_recv_init(54322,54323,0);
       int total_bytes = 0;
       int timestamp0 = 0;
       int timestamp13 = 0;
       session_set_set(session_set,session0);
       session_set_set(session_set,session13);
	while(true)
	{
	     
           int ret =session_set_select(session_set,NULL,NULL);
           if (ret <=0)
           continue;
           if (session_set_is_set(session_set,session0))
           {
                total_bytes = oRTP_recv_now(session0,buffer,BUFFER_SIZE,timestamp0);
                if( total_bytes == -1 )
                {
                    printf("buffer size is too small\n");
                    continue;
                }
                else if( total_bytes > 0)
                printf("session0:received %d bytes\n",total_bytes);
           }
           else if( session_set_is_set(session_set,session13) )
           {
                total_bytes = oRTP_recv_now(session13,buffer,BUFFER_SIZE,timestamp13);
                if( total_bytes == -1 )
                {
                    printf("buffer size is too small\n");
                    continue;
                }
                else if( total_bytes > 0)
                printf("session13:received %d bytes\n",total_bytes);
           }
	}
	session_set_destroy(session_set);*/
	oRTP_recv_close(session0);
       //oRTP_recv_close(session13);
	return 0;
}
