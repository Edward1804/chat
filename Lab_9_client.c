#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <fcntl.h>

#define RCVBUFSIZE 256   /* Size of receive buffer */

void DieWithError(char *errorMessage)  /* Error handling function */
{
  perror(errorMessage); fflush(stdout);
  exit(0);
}

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char *echoString;                /* String to send to echo server */
    char echoBuffer[RCVBUFSIZE];     /* Buffer for echo string */
    unsigned int echoStringLen;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv() 
                                        and total bytes read */

    if ( argc != 3 )    /* Test for correct number of arguments */
    {
       fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    
    echoServPort = atoi(argv[2]); /* Use given port, if any */
    
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");
	    
    memset(&echoServAddr, 0, sizeof(echoServAddr));     
    echoServAddr.sin_family      = AF_INET;             
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   
    echoServAddr.sin_port        = htons(echoServPort); 

    if (connect( sock , (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");
    
	int flags = fcntl( sock , F_GETFL );
	fcntl( sock , F_SETFL , flags | O_NONBLOCK );
	
    for( ; ; ){
    
	int HaveMsg = 0;
	while( !HaveMsg )
	{
		echoString = malloc( 256 );
		printf("You: ");
    	char *buf;
		for( buf = echoString ; (*buf=getchar()) != '\n' ; buf++ )
			if( *buf == '>' )
				buf--;
		*buf = 0;
		if( *echoString != 0 )
			HaveMsg = 1;
	}
	
    if( strcmp( echoString , "<<EXIT" ) == 0 )
	{
		if( send( sock, "<<EXIT", 6 , 0 ) == -1 )
		printf("ERROR\n");
		break;
	}
	echoStringLen = strlen(echoString);          /* Determine input length */
    if ( send( sock, echoString, echoStringLen, 0 ) != echoStringLen )
        DieWithError("send() sent a different number of bytes than expected ");
	free( echoString );
		
	char *GetMsg = malloc( RCVBUFSIZE );
	while( strcmp( GetMsg , ">>GETMSG" ) != 0 )
		if ( recv(sock, GetMsg , RCVBUFSIZE - 1, 0) > 0 )
		{
			if( strcmp( GetMsg , ">>GETMSG" ) != 0  )
				printf( "%s\n" , GetMsg );	
		}
    }
    
	shutdown( sock , SHUT_WR );
    close(sock);
    
    exit(0);
}

