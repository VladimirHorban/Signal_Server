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
#include <thread>

void service( int aClientSocket )
{
    char* buffer = new char[1024];
    recv( aClientSocket, buffer, 1024, 0 );
    send( aClientSocket, buffer, 1024, 0 );
    close( aClientSocket );
}

void handler( int aListenSocket )
{
    int         ndfs           = 10; // Number of file descriptors
    int         clientSocket   = 0;
    sockaddr_in clientAddr;
    socklen_t   clientAddrSize = sizeof( clientAddr );

    fd_set      readSet;
    fd_set      errorSet;

    FD_ZERO( &readSet );
    FD_ZERO( &errorSet );

    FD_SET( aListenSocket, &readSet );
    FD_SET( aListenSocket, &errorSet );

    for( ;; )
    {
        int nread   = 0;
        int readyFd = select( ndfs + 1, &readSet, nullptr, &errorSet, nullptr );
        if( readyFd == -1 )
        {
            perror( "select" );
            exit( EXIT_FAILURE );
        }
        else if( readyFd == 0 )
        {
            printf( "%s \n" , "Server is wayting" );
            continue;
        }
        else
        {
            for( int i = 0; i < ndfs + 1; i++ )
            {
                if( FD_ISSET( i, &readSet ) )
                {
                    if( i == aListenSocket )
                    {
                        clientSocket = accept( aListenSocket, ( sockaddr* )&clientAddr, &clientAddrSize );
                        FD_SET( clientSocket, &readSet );
                    }
                    else
                    {
                        ioctl( i, FIONREAD, &nread );
                        if( nread == 0 )
                        {
                            close( i );
                            FD_CLR( i, &readSet );
                            printf( "Client %d has removed \n", i );
                        }
                        else
                        {
                            std::thread thread( service, i );
                            thread.detach();
                            FD_CLR( i, &readSet );
                        }
                    }

                }

                else if( FD_ISSET( i, &errorSet ) )
                {
                    FD_CLR( i, &errorSet );
                    printf( "Socket %d has failed \n", i );
                }
            }
        }
    }
}

int main( int argc, char *argv[] )
{
    int listenSocket   = 0;
    int port           = 0;
    int maxConnections = 10;

    sockaddr_in serverAddr;

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

    handler( listenSocket );

    return EXIT_SUCCESS;
}
