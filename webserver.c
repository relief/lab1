/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */
#include <string.h>
#include <fcntl.h>

enum content_type {HTML = 0, JPEG = 1, GIF = 2, PNG = 3, PDF = 4, CSS = 5, JS = 6, BMP = 7, OTHER = -1};
void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */
void error(char *msg)
{
    perror(msg);
    exit(1);
}
int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;          // for signal SIGCHLD
     
     printf("1232131231");
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     printf("1231231");
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);
     
     clilen = sizeof(cli_addr);
     
     /****** Kill Zombie Processes ******/
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }
     /*********************************/
     
     while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         pid = fork(); //create a new process
         if (pid < 0)
             error("ERROR on fork");
         
         if (pid == 0)  { // fork() returns a value of 0 to the child process
             close(sockfd);
             dostuff(newsockfd);
             exit(0);
         }
         else //returns the process ID of the child process to the parent
             close(newsockfd); // parent doesn't need this 
     } /* end of while */
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void getFileName(char* input,char* fn)
{
	char* fntmp,*tmp = input;
	/* Pinpoint the starting poing */
	while (*tmp != ' '){
		tmp += 1;
	}
	tmp += 2;
	if (*tmp == '\0')
	{
		strcpy(fn, "index.html");
		return ;
	}
	while (*tmp != ' ')
	{
		*fn = *tmp;
		tmp   += 1;
		fn += 1;
	}
	*fntmp = '\0';
}
int sameStr(char* s1,char* s2)
{
	return strncmp(s1, s2, 256) == 0? 1:0;
}
/* Returns the content type of the file based on the filename given */
enum content_type getContentType(char *fileName)
{
	char ext[10];
	char *ext_tmp;

	ext_tmp = ext;
	while (*fileName != '.')
		fileName++;
	fileName++;
	while (*fileName != '\0') {
		*ext_tmp = *fileName;
		ext_tmp++;
		fileName++;
	}
	*ext_tmp = '\0';
	printf("The extension is: %s\n",ext);

	if (sameStr(ext,"html"))	return HTML;
	if (sameStr(ext,"css"))		return CSS;
	if (sameStr(ext,"js"))		return JS;
	if (sameStr(ext,"jpg"))		return JPEG;
	if (sameStr(ext,"png"))		return PNG;
	if (sameStr(ext,"bmp"))		return BMP;
	return OTHER;
}

void buildHeader(char* header,enum content_type ctype)
{
   time_t mytime = time(NULL);
   header = "HTTP/1.0 200 OK\n\n";
/*"Connection: close\n";*/
/*   char *contentLength = "Content-Length: ";*/
/*   char *contentType = "Content-Type: ";*/
/*   char *date = strcat("Date:",ctime(&mytime));*/
/*   */
/*   strcat(header, contentLength);*/
/*   strcat(header, contentType);*/
/*   strcat(header, date);*/
/*   printf("Header: %s", header);*/
}

void output_header_and_targeted_file_to_sock(int sock, int resource, char* header, char* fileName)
{
    int n;
    char data_to_send[1024];
    int rcvd, fd, bytes_read;

    n = send(sock, header, sizeof(header), 0);
    if (n < 0) error("ERROR writing header to socket");

    while ( (bytes_read=read(resource, data_to_send, 1024))>0 )
        write(sock, data_to_send, bytes_read);
    if (n < 0) error("ERROR writing filename to socket");
}

void output_dne(int sock, char* fileName)
{
    char str[50];
    int n;
    
    sprintf(str, "The file %s does not exist\n", fileName);
    n = write(sock, str, 23);
    if (n < 0) error("ERROR writing to socket");
}

void dostuff (int sock)
{
   int n;
   char buffer[1024];
   char header[1024];
   enum  content_type ctype;  // Make some int standing for some types
   char fileName[256];
   int resource;
   printf("12321312");
   bzero(buffer,1024);
   n = read(sock,buffer,1024);  
   if (n < 0) error("ERROR reading from socket");

   getFileName(buffer,fileName);
   printf("Targeted file: %s\n",fileName);
   ctype = getContentType(fileName);
   printf("Type: %d\n", ctype);
   if ((resource = open(fileName, O_RDONLY)) > 0){
	buildHeader(header, ctype);
	printf("Header: %d\n", ctype);
	output_header_and_targeted_file_to_sock(sock, resource, header, fileName);
   }
   else{
	output_dne(sock, fileName);
   }
/*   if (ctype == -1) {*/
/*      error("ERROR requested filetype not supported");*/
/*      printf("Filetype not supported\n");*/
/*   }*/

   // if (n < 0) error("ERROR writing to socket");
    shutdown(sock, SHUT_RDWR);         //All further send and recieve 
    close(sock);
    return ;
}
