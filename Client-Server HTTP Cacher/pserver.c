#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define MAXSIZE 100000

//break message into host and url
void breakdown(char* buf, char* host)
{
	int i,j,check=0;	

	memset(host,'\0',256);
//	memset(url,'\0',MAXSIZE-256);

	if(buf[strlen(buf)-2]!='/')
	{
		//set last character to '/'
		buf[strlen(buf)-1]='/';
		check=1; 
	}
	for (i = 0; buf[i] != '/'; i++) //get host
	{
		host[i] = buf[i]; //eliminates new line character
	}
}
bool CheckCache(FILE *u_list, char *url) //check to see if cache exists for a webpage
{
	char check[512];
	check[strcspn(check, "\n")] = '\0'; //replace newline character with null
	fflush(u_list);
	while(fgets(check,512,u_list) != NULL) //get list
        {
        	if(strstr(check, url) != NULL)  //if list matches input then true
        	{
        		return 1;
        	}
        }
	return 0;
}
//concatenate string for http request
void Connect(char* buf, char* host)
{
	memset(buf,'\0',MAXSIZE);
	strcpy(buf,"GET /");
	strcat(buf," HTTP/1.1\r\nHost:");    //GET / HTTP/1.1\r\nHOST:"URL"\r\n\r\n
	strcat(buf,host);
	strcat(buf,"\r\n\r\n");
}
int main (int argc, char *argv[])
{
	
	int sockfd, portno, newsockfd, clilen, webfd, size, i, j, check, tick, count=0, count2=1;
	struct sockaddr_in serv_addr, cli_addr, pserver;
	bool match;
	char help[5][500], hold[500], buf[MAXSIZE], ip_addr[50], url[MAXSIZE-256], host[256], buffer[MAXSIZE], err[500], *HTTPcode, filename[100];
	struct hostent* Con;
	struct in_addr** addr_list;
	FILE *URL_List, *Cache, *temp;

        if(argc < 2) //arg check
        {

                printf("\nPort number is missing...\n");
                exit(0);
        }
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if (sockfd < 0)
	{
		printf ("Failed to create socket.\n");
		return 1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family=AF_INET;            //connect to client
	serv_addr.sin_addr.s_addr=INADDR_ANY;
	serv_addr.sin_port=htons(portno);

	//bind to port
	if(bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf ("Bind failed.\n");
		close (sockfd);
		return 1;
	}
	printf ("Bind successful.\n");

	//listen for connections
	listen(sockfd,5);
	while(1) //keep socket open
	{
		printf("\nReady for incoming connection.\n");
		clilen=sizeof(cli_addr);
		newsockfd=accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if(newsockfd < 0)
		{
			printf("Connection not accepted.\n");
			close(newsockfd);
			return 1;
		}
		printf("Client Connected\n");
		printf("\n***************WELCOME*****************\n");
		tick = 1;
		while(tick) //keep client socket open 
		{
			memset(buffer,'\0',MAXSIZE); //set buffer
			memset(buf,'\0',MAXSIZE); //set message
			read(newsockfd, buf, 256); //read url from client
			if((strncmp(buf, "quit", 4)) != 0) //if message isn't "quit"
			{
			        URL_List = fopen("list.txt", "a+");
				printf("\nClient has requested for the webpage %s", buf);
                                breakdown(buf,host);  //breakdown message into u
				if(CheckCache(URL_List, host)) //if cache exists for webpage
				{
					memset(buffer,'\0',MAXSIZE);
					Cache = fopen(host, "a+");  //open file with name of url
                                        fread(buffer,1,MAXSIZE,Cache); 
                                        printf("Cache Found....SENDING\n");
					write(newsockfd, buffer, strlen(buffer));  //write cache to client
					fclose(Cache);
					fclose(URL_List);
					memset(buffer,'\0',MAXSIZE);
				}
				else //cache doesnt exist
				{
					printf("Cache Not Found: Fetching webpage from web server...\n");
					if((Con = gethostbyname(host)) == NULL) //see if url exists, if not error
					{
						printf("Gethostbyname unsuccessful\n"); 
						memset(err,'\0',500);
						strcpy(err, "Error: Gethostbyname Unsuccessful. Reconnect to server");
						write(newsockfd, err, strlen(err)); 
						close(newsockfd);
						tick = 0;
					}
					else
					{
						addr_list = (struct in_addr **) Con->h_addr_list; //establish connection with url host server
						for (i=0; addr_list[i]!=NULL;i++)
						{
							strcpy (ip_addr,inet_ntoa(*addr_list[i]));
						}
						webfd = socket(AF_INET,SOCK_STREAM,0); //open web socket
						if(webfd < 0)
						{
							printf("Failed to create socket.\n");
						}
						pserver.sin_family = AF_INET;
						pserver.sin_addr.s_addr = inet_addr(ip_addr);
						pserver.sin_port=htons(80);
						if(connect(webfd,(struct sockaddr*)&pserver, sizeof(pserver)) < 0)
						{
							printf("Connection failed.\n");
							close(webfd);
						}
						Connect(buf,host); //concatenate http request
						if(send(webfd,buf,strlen(buf),0) < 0) //send http request
						{
							printf("Request failed.\n");
							close(webfd);
						}
						if(recv(webfd,buffer,MAXSIZE,0) < 0) //error check
						{
							printf("No reply from webserver.\n");
							close(webfd);
						}
						else //write http response to client
						{
							printf("Checking http code...\n");
							HTTPcode = strstr(buffer, "200"); //check for http code
							if(HTTPcode)
							{
								printf("HTTP CODE 200!\n");
								if(count < 5)  //if list has less than 5 entries
								{
									fprintf(URL_List,"%s\n", host); //append url to file
									count++;
								}
								else //if more than 5 entries
								{
									
									fseek(URL_List,0,SEEK_SET); //reset file pointer
									while(fscanf(URL_List,"%s",help[i]) == 1) //get all urls and store in help
									{
										i++;
									}
									
									
									remove(help[1]); //delete cache file
									strcpy(help[1], help[2]);
									strcpy(help[2], help[3]);
									strcpy(help[3], help[4]);  //move all array elements down 1 position
									strcpy(help[4], help[5]);
									strcpy(help[5], host); //put new url in list
									puts(help[1]);
									fclose(URL_List);
									URL_List = fopen("list.txt", "w+");
									for(i=1;i<6;i++)  //rewrite list after deleting oldest url and adding newest
									{
										fprintf(URL_List,"%s\n", help[i]);
									}	
								}
								printf("Cache stored in filename: %s\n", host); //file name
								Cache = fopen(host,"a+"); 
								if(Cache == NULL)
								{
									perror("FAILED: ");
									return 1;
								}	
								fprintf(Cache,"%s\n", buffer); //write webpage to file
								close(webfd);
								fclose(Cache);
								fclose(URL_List);
								write(newsockfd,buffer,strlen(buffer));  //send webpage to client after cached
								memset(buffer,'\0',MAXSIZE);
								printf("Webpage has been sent to client\n");
							}
							else //http code is not 200
							{
								printf("HTTP CODE NOT 200...Webpage not cached...Fowarding HTTP Response to Client\n");
								close(webfd);			
								write(newsockfd,buffer,strlen(buffer));	//send http response to client					
							}	
						}
					}
				}
			}
			else //if "quit"
			{
				memset(buf,'\0',MAXSIZE); //reset message
				printf("Client has disconnected\n"); 
				close(newsockfd);//close client socket
				tick = 0; //exit loop
			}
		}		
	}
	return 0;
}
