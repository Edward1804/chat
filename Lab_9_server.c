#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <fcntl.h>
#include <sys/poll.h>

#define MAXPENDING 5
#define RCVBUFSIZE 256

int SocSerD;
    
struct sockaddr_in SerAddr;
    
unsigned short SerPort; 
unsigned int clntLen;



typedef struct Clients
{
	int ClientD;
	char *Name;
	struct sockaddr_in CliAddr;
	struct Clients *Next;
	
}Clients;

Clients *ClientList = NULL;


void HandleTCPClient(int clntSocket)
{
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
   
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE , 0)) < 0)
        printf("recv() failed\n");

    while (recvMsgSize > 0)  
    {
        if (send(clntSocket, echoBuffer, recvMsgSize, MSG_DONTWAIT ) != recvMsgSize)
            printf("send() failed\n");
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            printf("recv() failed\n");
        printf("%s",echoBuffer);
    }

    printf("\n");
}

void  CloseClientList( Clients *list )
{
	for( ; list != NULL ; list = list->Next )
	{
		close( list->ClientD );
	}
}

Clients* ClientExit( Clients *Start , Clients *Now , Clients *Old  )
{
	printf("Client out - %s \n" , inet_ntoa(Now->CliAddr.sin_addr));
	if( Now == Start )
	{
		Start = Start->Next;
		free( Now );
		Now = Start;
	}
	else
	{
		Clients *Tmp = Now;
		Old->Next = Now->Next;
		free( Now );
		Now = Old->Next;
	}
	return Start;
}

void ClientWrite( Clients *Start , Clients *Writer , char *str )
{
	printf("%s - %s \n" , inet_ntoa(Start->CliAddr.sin_addr) , str );
	char *Msg;
	for( Start ; Start->Next != NULL ; Start = Start->Next )
	{
		if( Start != Writer )
		{	
			Msg = malloc( sizeof( 255 ) ); 
			strcpy( Msg , inet_ntoa( Start->CliAddr.sin_addr ));
			strcat( Msg , " : " );
			strcat( Msg , str );
			send( Start->ClientD , Msg , 255 , 0 );
			free( Msg );
		}
	}
}

void Who( Clients *Start , Clients *Now )
{
	printf("%s - <<WHO \n" , inet_ntoa(Start->CliAddr.sin_addr));
	char *Msg;
	Msg = malloc( sizeof( 255 ) ); 
	strcpy( Msg , "\tIN CHAT NOW\n" );
	send( Now->ClientD , Msg , 255 , 0 );
	free( Msg );
	for( Start ; Start->Next != NULL ; Start = Start->Next )
	{	
			Msg = malloc( sizeof( 255 ) ); 
			strcpy( Msg , "\t\t" );
			strcat( Msg , inet_ntoa( Start->CliAddr.sin_addr ));
			send( Now->ClientD , Msg , 255 , 0 );
			free( Msg );
	}
}


int main( int av , char **ag )
{
    SerPort = 6500;
    
    if ((SocSerD = socket( PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        printf("socket() failed\n");

	ClientList = malloc( sizeof( Clients )  );
	ClientList->Next = NULL;
	
    memset( &SerAddr , 0 , sizeof(SerAddr) );
    SerAddr.sin_family = AF_INET;
    SerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    SerAddr.sin_port = htons( SerPort );
    
    if (bind( SocSerD , (struct sockaddr *) &SerAddr,
        sizeof( SerAddr )) < 0)
        printf("bind() failed\n");
    if ( listen( SocSerD, MAXPENDING ) < 0)
        printf("listen() failed\n");
	int flags = fcntl( SocSerD , F_GETFL );
	fcntl( SocSerD , F_SETFL , flags | O_NONBLOCK );
	
	Clients *End = ClientList;
	Clients *Buf , *Old;

	
    for (;;)
    {
        clntLen = sizeof( End->CliAddr );
		if (( End->ClientD = accept( SocSerD, (struct sockaddr *) &(End->CliAddr), 
                               &clntLen)) < 0)
		{
			
		}
		else
		{
			printf("Handling client %s\n", inet_ntoa(End->CliAddr.sin_addr));	
			int flags = fcntl( End->ClientD , F_GETFL );
			fcntl( End->ClientD , F_SETFL , flags | O_NONBLOCK );
			End->Next = malloc( sizeof( Clients ) );
			End = End->Next;
			End->Next = NULL;
		}

		for( Buf = ClientList ; Buf != NULL && Buf->Next != NULL ; Buf = Buf->Next )
		{
			char *echoBuffer = malloc( RCVBUFSIZE );
			if ( recv( Buf->ClientD , echoBuffer, RCVBUFSIZE , 0) != -1 )
			{
				if( strcmp( echoBuffer , "<<EXIT" ) == 0 )
					ClientList = ClientExit( ClientList , Buf , Old );
				else
				{ 	if( strcmp( echoBuffer , "<<WHO" ) == 0 )
						Who( ClientList , Buf );	
					else
						ClientWrite( ClientList , Buf , echoBuffer );
					send( Buf->ClientD , ">>GETMSG" , 255 , 0 );
				}
			}
			free( echoBuffer );
			Old = Buf;
		}
		usleep( 10 );
		/*
        printf("%d\n" , fcntl( *SocCliD , F_GETFL ));
        HandleTCPClient( *SocCliD );
        printf("%d\n" , fcntl( *SocCliD , F_GETFL ));
		*/
        
    }
    
    CloseClientList( ClientList );
    
    return 0;
}
