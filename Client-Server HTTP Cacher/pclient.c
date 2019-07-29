#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <error.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define MAXSIZE 100000

int main (int argc, char *argv[])
{
        int sockfd, portno, n, tick;
        struct sockaddr_in serv_addr;
        struct hostent *server;
	char buf[MAXSIZE];
	char check[MAXSIZE];

        portno = atoi(argv[1]);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
                error(EXIT_FAILURE, 0, "ERROR opening socket");
        server = gethostbyname("129.120.151.94"); //IP address of server
//      server = gethostbyname("localhost"); //Both in the same machine [IP address 127.0.0.1]

        if (server == NULL)
        {
                printf("\nERROR, no such host...\n");
                exit(0);
        }

        //Connecting with the server
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(portno);
        memcpy(&serv_addr.sin_addr, server->h_addr, server->h_length);
        if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0)
                error(EXIT_FAILURE, 0, "ERROR connecting the server...");

	printf ("Connected.\n");
	tick = 1;
	while(1) //for sending multiple client url requests
	{
		memset(check,'\0',MAXSIZE);
		memset(buf,'\0',MAXSIZE);
		printf("\nURL(Enter 'quit' if you wish to exit program): ");
		fgets(buf, MAXSIZE, stdin); //get url input
		strcpy(check, buf);
		write(sockfd, buf, strlen(buf)); //send url to the proxy serrver
                if((strncmp(check, "quit", 4) != 0))
                {
			memset(buf, '\0', MAXSIZE); 
			if((recv(sockfd, buf, MAXSIZE, 0)) < 0) //error check
			{
				printf ("Failed to receive message from server.\n");
				return 1;
			}
			else
			{
				printf ("%s\n",buf); //print http response onto terminal
			}
		}
		else
		{
			tick = 0;
			close(sockfd);
			return 1;
		}
	} 
	close(sockfd);//close socket
	return 0;
}
