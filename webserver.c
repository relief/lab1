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

enum content_type {HTML = 0, JPEG = 1, GIF = 2, PNG = 3, PDF = 4, CSS = 5, JS = 6, BMP = 7, TXT = 8, OTHER = -1};

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
     
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     /* Initialize the socket */
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
  	while (*tmp != ' '){
  		tmp += 1;
  	}
  	tmp += 2;
  	fntmp = fn;
  	
    while (*tmp != ' ')
  	{
  		*fntmp = *tmp;
  		tmp   += 1;
  		fntmp += 1;
  	}
  	*fntmp = '\0';
  	
    //if no file is specified
    if (*fn == '\0')
  		strcpy(fn, "index.html");
}

/* Returns whether two strings are equal */
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
  	while (*fileName != '.'){
  		if (*fileName == '\0')
  			return OTHER;
  		fileName++;
  	}
  	fileName++;

  	while (*fileName != '\0') {
  		*ext_tmp = *fileName;
  		ext_tmp++;
  		fileName++;
  	}
  	*ext_tmp = '\0';
  	printf("The extension is: %s\n",ext);

  	if (sameStr(ext,"html"))	return HTML;
    if (sameStr(ext,"jpg"))   return JPEG;
    if (sameStr(ext,"gif"))   return GIF;
    if (sameStr(ext,"png"))   return PNG;
    if (sameStr(ext,"pdf"))   return PDF;
  	if (sameStr(ext,"css"))		return CSS;
  	if (sameStr(ext,"js"))		return JS;
  	if (sameStr(ext,"bmp"))		return BMP;
    if (sameStr(ext,"txt"))   return TXT;
  	return OTHER;
}

void buildHeader(char* header,enum content_type ctype)
{
    //time_t mytime = time(NULL);
    sprintf(header, "HTTP/1.0 200 OK\n");
    header = strcat(header,"Connection: close\n");

    if (ctype == 0) 
      header = strcat(header,"Content-Type: text/html\n");
    else if (ctype == 1) 
      header = strcat(header,"Content-Type: image/jpg\n");
    else if (ctype == 2) 
      header = strcat(header,"Content-Type: image/gif\n");
    else if (ctype == 3) 
      header = strcat(header,"Content-Type: image/png\n");
    else if (ctype == 4) 
      header = strcat(header,"Content-Type: application/pdf\n");
    else if (ctype == 5)
      header = strcat(header,"Content-Type: application/css\n");
    else if (ctype == 6) 
      header = strcat(header,"Content-Type: application/javascript\n");
    else if (ctype == 7) 
      header = strcat(header,"Content-Type: image/bmp\n");
    else if (ctype == 8) 
      header = strcat(header,"Content-Type: text/txt\n");
    //printf("Header2: %s", header);

    //char *date = strcat("Date: ",ctime(&mytime));
    //header = strcat(header, date);
    header = strcat(header,"Server: 127.0.0.1\n");
    header = strcat(header,"Status: 200 OK\n");
    header = strcat(header, "\n");
}

void output_header_and_targeted_file_to_sock(int sock, int resource, char* header, char* fileName)
{
    int n;
    char data_to_send[1024];
    int bytes_read;

    printf("resource%d",resource);
    n = send(sock, header, strlen(header), 0);
    if (n < 0) error("ERROR writing header to socket");
    printf("\nheader %s\n",header);
    while ( (bytes_read=read(resource, data_to_send, 1024))>0 )
       n = write(sock, data_to_send, bytes_read);
    if (n < 0) error("ERROR writing filename to socket");
}

void output_dne(int sock, char* fileName)
{
    char str[50];
    int n;
    
    sprintf(str, "The file %s does not exist\n", fileName);
    n = write(sock, str, strlen(str));
    if (n < 0) error("ERROR writing to socket");
}

void dostuff (int sock)
{
    int n;
    char buffer[1024];
    char header[1024];
    enum  content_type ctype; 
    char fileName[256];
    int resource;

    bzero(buffer,1024);
    n = read(sock,buffer,1024);  
    if (n < 0) error("ERROR reading from socket");
    printf("Here is the message: %s\n",buffer);

    getFileName(buffer,fileName);
    printf("Targeted file: %s\n",fileName);
    ctype = getContentType(fileName);
    printf("Type: %d\n", ctype);
    char data_to_send[1024];
    int bytes_read;

    //***** This needs to be changed, because right now if there's no file specified, no header is produced
    if ((resource = open(fileName, O_RDONLY)) > 0) {
      buildHeader(header, ctype);
      printf("Response Header: %s\n", header);
      output_header_and_targeted_file_to_sock(sock, resource, header, fileName);
    }
    else {
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
