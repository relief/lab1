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
	printf("File extension is: %s\n",ext);
/* Check the corresponding file extension */
	if (sameStr(ext,"html"))	return HTML;
	if (sameStr(ext,"jpg"))		return JPEG;
	if (sameStr(ext,"gif"))		return GIF;
	if (sameStr(ext,"png"))		return PNG;
	if (sameStr(ext,"pdf"))		return PDF;
	if (sameStr(ext,"css"))		return CSS;
	if (sameStr(ext,"js"))		return JS;
	if (sameStr(ext,"bmp"))		return BMP;
	if (sameStr(ext,"txt"))		return TXT;
  	return OTHER;
}

void buildHeader(char* header,char* fileName, enum content_type ctype)
{
    time_t current_time;
    char* c_time_string;
/* HTTP header */
    sprintf(header, "HTTP/1.1 200 OK\n");
/* Content-Type */
    switch (ctype){
	case JPEG:
		strcat(header, "Content-Type:image/jpeg\n");
		break;
	case GIF:
		break;
	case PNG:
		strcat(header, "Content-Type:image/png\n");
		break;
	case PDF:
		strcat(header,"Content-Type: application/pdf\n");
		break;
	case BMP:
		strcat(header, "Content-Type:image/bmp\n");
		break;
	case CSS:
		strcat(header,"Content-Type: application/css\n");
		break;
	case JS:
		strcat(header,"Content-Type: application/javascript\n");
		break;
	case HTML:
		strcat(header,"Content-Type: text/html\n");
		break;
	case TXT:
		strcat(header,"Content-Type: text/txt\n");
		break;
	}
/* Date */
	current_time = time(NULL);
	c_time_string = (char *)ctime(&current_time);
	sprintf(header, "%sDate:%s", header, c_time_string);
/* Last-Modified */
	struct stat attr;
	stat(fileName, &attr);
	sprintf(header, "%sLast-Modified:%s", header, (char *)ctime(&attr.st_mtime));
/* Server */
	strcat(header, "Server:Gabby\n");
/* Version */
	strcat(header, "version:HTTP/1.1\n");
/* End of header */
	strcat(header, "\n");

}

void output_header_and_targeted_file_to_sock(int sock, int resource, char* header, char* fileName)
{
    int n;
    char data_to_send[1024];
    int bytes_read;

/*Output header */
    n = send(sock, header, strlen(header), 0);
    if (n < 0) error("ERROR writing header to socket");

/*Output requested file */
    while ( (bytes_read=read(resource, data_to_send, 1024))>0 )
       n = write(sock, data_to_send, bytes_read);
    if (n < 0) error("ERROR writing filename to socket");
}
/* Content to output when requested file doesn't exist */
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
    char filePath[256];
    int resource;

    bzero(buffer,1024);
    n = read(sock,buffer,1024);  
    if (n < 0) error("ERROR reading from socket");
    printf("--------------------------Start of a new request---------------------------\n");
    printf("Here is the message from the client: \n%s",buffer);
/* get file name */
    getFileName(buffer,fileName);
    printf("Targeted file name: %s\n",fileName);
/* get file path in the server */
    sprintf(filePath, "resource/%s",fileName);
    printf("Targeted file path in the server: %s\n",filePath);
/* get file extension */
    ctype = getContentType(fileName);

/* Check whether the requested file exists or not and behave correspondingly */
    if ((resource = open(filePath, O_RDONLY)) > 0){
	buildHeader(header,fileName, ctype);
	printf("\nFile exists, therefore the following respond header will be sent: \n%s",header);
	output_header_and_targeted_file_to_sock(sock, resource, header, filePath);
    }
    else{
	printf("\nFile doesn't exist! \n");
	output_dne(sock, fileName);
    }
    shutdown(sock, SHUT_RDWR);         //All further send and recieve 
    close(sock);
    return ;
}
