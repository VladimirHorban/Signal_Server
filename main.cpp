#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <error.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>
#include <thread>

void service( int aClientSocket )
{
    char* buffer = new char[1024];
    recv( aClientSocket, buffer, 1024, 0 );
    send( aClientSocket, buffer, 1024, 0 );
    close( aClientSocket );
}

int main( int argc, char *argv[] )
{
    int listenSocket   = 0;
    int newSocket      = 0;
    int port           = 0;
    int maxConnections = 10;
    int maxSocket      = 0;
    int on             = 1;
    int result         = 0;
    int nubmerOfSockets  = 0;
    int ready          = 0;

    sockaddr_in serverAddr;

    fd_set masterSet;
    fd_set workingSet;
//    fd_set errorSet;

//    if( argc < 2 )
//    {
//        perror( "Not enough arguments" );
//        exit( EXIT_FAILURE );
//    }

//    port = atoi( argv[1] );

    port = 54000;

    listenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if( listenSocket == -1 )
    {
        perror( "Can not create listen socket" );
        exit( EXIT_FAILURE );
    }

    result = setsockopt( listenSocket, SOL_SOCKET, SO_REUSEADDR, ( void* )&on, sizeof( on ) );
    if( result == -1 )
    {
        perror( "setsockopt() failed" );
        close( listenSocket );
        exit( EXIT_FAILURE );
    }

    result = ioctl( listenSocket, FIONBIO, (char *)&on );
    if( result == -1 )
    {
        perror( "ioctl() failed" );
        close( listenSocket );
        exit( EXIT_FAILURE );
    }

    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons( port );
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if( bind( listenSocket, ( sockaddr* )&serverAddr, ( socklen_t )sizeof( sockaddr_in ) ) == -1 )
    {
        perror( "Can not bind listen socket" );
        exit( EXIT_FAILURE );
    }

    if( listen( listenSocket, maxConnections ) == -1 )
    {
        perror( "Can not listen" );
        exit( EXIT_FAILURE );
    }

    FD_ZERO( &masterSet );
    FD_SET( listenSocket, &masterSet );
    maxSocket = listenSocket;    

    for( ;; )
    {
        memcpy( &workingSet, &masterSet, sizeof( masterSet ) );

        printf( "%s \n", "Wait for select" );
        nubmerOfSockets = select( maxSocket + 1, &workingSet, nullptr, nullptr, nullptr );

        ready = nubmerOfSockets;

        if( nubmerOfSockets < 0 )
        {
            perror( "Select failed" );
            break;
        }
        else
        {
            for( int i = 0; i <= maxSocket && ready > 0; i++ )
            {
                if( FD_ISSET( i, &workingSet ) )
                {
                    ready--;
                    if( i == listenSocket )
                    {
                        do
                        {
                           newSocket = accept( listenSocket, nullptr, nullptr );
                           if ( newSocket < 0 )
                           {
                              if (errno != EWOULDBLOCK)
                              {
                                 perror("  accept() failed");
                                 exit( EXIT_FAILURE );
                              }
                              break;
                           }

                           printf( "New incoming connection - %d\n", newSocket );
                           FD_SET( newSocket, &masterSet );

                           if ( newSocket > maxSocket )
                               maxSocket = newSocket;

                        } while ( newSocket != -1 );
                    }
                    else
                    {
                        char* buffer = new char[1024];
                        recv( i, buffer, 1024, 0 );
                        send( i, buffer, 1024, 0 );
                        close( i );
                        FD_CLR( i, &masterSet );
                        if ( i == maxSocket )
                        {
                           while ( FD_ISSET( maxSocket, &masterSet ) == false )
                              maxSocket -= 1;
                        }
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
