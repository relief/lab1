#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<time.h>

#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */

#include <fcntl.h>

#define MAXLINE 4096
#define MAX_CLIENT_NUM 100     // how many pending connections queue will hold  

struct client_info_type{
	char* ip;
	int port;
};

int fd_A[MAX_CLIENT_NUM];    // accepted connection fd  
int conn_amount;    // current connection amount  
char    buff[MAXLINE];
struct client_info_type client_info[MAX_CLIENT_NUM];
char* dst;
enum content_type {HTML = 0, JPEG = 1, GIF = 2, PNG = 3, PDF = 4, CSS = 5, JS = 6, BMP = 7, TXT = 8, OTHER = -1};

int main(int argc, char** argv)
{
    int    listenfd, connfd,new_fd;
    struct sockaddr_in     servaddr;
    struct sockaddr_in client_addr; // connector's address information  

    int     n,i,ret;
    int     yes = 1;

    socklen_t sin_size; 

    if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
	printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
	exit(0);
    }

    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {  
        perror("setsockopt");  
        exit(1);  
    }  

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
	printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
	exit(0);
    }

    if( listen(listenfd, MAX_CLIENT_NUM) == -1){
	printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
	exit(0);
    }

    printf("======waiting for client to connect======\n");

    fd_set fdsr;  
    int maxsock;  
    struct timeval tv;  
   
    conn_amount = 0;  
    sin_size = sizeof(client_addr);  
    maxsock = listenfd;  
    while (1) {
        // initialize file descriptor set  
        FD_ZERO(&fdsr);
        FD_SET(listenfd, &fdsr);
   
        // timeout setting  
        tv.tv_sec = 300;  
        tv.tv_usec = 0;  
   
        // add active connection to fd set  
        for (i = 0; i < MAX_CLIENT_NUM; i++) {  
            if (fd_A[i] != 0) {  
                FD_SET(fd_A[i], &fdsr);  
            }  
        }  
   
        ret = select(maxsock + 1, &fdsr, NULL, NULL, &tv);  
        if (ret < 0) {  
            perror("select");  
            break;  
        } else if (ret == 0) {  
            printf("timeout\n");  
            continue;  
        }  

        // check every fd in the set  
        for (i = 0; i < conn_amount; i++) {  
            if (FD_ISSET(fd_A[i], &fdsr)) {  
                ret = recv(fd_A[i], buff, sizeof(buff), 0);  
                if (ret <= 0) {        // client close  
/*                    printf("client[%d] close\n", i);  */
/*                    close(fd_A[i]);  */
/*                    FD_CLR(fd_A[i], &fdsr);  */
/*                    fd_A[i] = 0;  */
/*                    conn_amount--;       */
                } else {        // receive data  
                    if (ret < MAXLINE)  
                          memset(&buff[ret], '\0', 1);
                          dostuff(fd_A[i]);
			  close(fd_A[i]);  
	                  FD_CLR(fd_A[i], &fdsr);  
        	          fd_A[i] = 0;
                }
            }
        }
        conn_amount = 0;
        // check whether a new connection comes  
        if (FD_ISSET(listenfd, &fdsr)) {  
            new_fd = accept(listenfd, (struct sockaddr *)&client_addr, &sin_size);  
            if (new_fd <= 0) {  
                perror("accept");  
                continue;  
            }  
            // add to fd queue  
            if (conn_amount < MAX_CLIENT_NUM) {  
                fd_A[conn_amount++] = new_fd;  
                printf("new connection client[%d] %s:%d\n", conn_amount,  
                        (char*)inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));  
		client_info[conn_amount-1].ip = (char*)inet_ntoa(client_addr.sin_addr);
		client_info[conn_amount-1].port = ntohs(client_addr.sin_port);
                if (new_fd > maxsock)  
                    maxsock = new_fd;  
            }  
            else {  
                printf("max connections arrive, exit\n");  
                send(new_fd, "bye", 4, 0);  
                close(new_fd);  
                break;  
            }  
        }
//        printf("client amount: %d\n", conn_amount);  
    }
    for (i = 0; i < MAX_CLIENT_NUM; i++) {  
        if (fd_A[i] != 0) {  
            close(fd_A[i]);  
        }  
    }  
   
    //exit(0);
}


/* Parses the filename requested from the client input */
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

/* Helper method: returns whether two strings are equal */
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

  /* Returns the corresponding file extension */
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

/* Builds a header into the header variable using information about the file (fileName and ctype)
   and date information */
void buildHeader(char* header,char* fileName, enum content_type ctype)
{
    time_t current_time;
    char* c_time_string;
    /* HTTP header */
    sprintf(header, "HTTP/1.1 200 OK\n");
    /* Content-Type */
    switch (ctype){
	case JPEG:
		strcat(header, "Content-Type: image/jpeg\n");
		break;
	case GIF:
		strcat(header, "Content-Type: image/gif\n");
		break;
	case PNG:
		strcat(header, "Content-Type: image/png\n");
		break;
	case PDF:
		strcat(header,"Content-Type: application/pdf\n");
		break;
	case BMP:
		strcat(header, "Content-Type: image/bmp\n");
		break;
	case CSS:
		strcat(header,"Content-Type: text/css\n");
		break;
	case JS:
		strcat(header,"Content-Type: text/javascript\n");
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
	sprintf(header, "%sDate: %s", header, c_time_string);
/* Last-Modified */
	struct stat attr;
	stat(fileName, &attr);
	sprintf(header, "%sLast-Modified: %s", header, (char *)ctime(&attr.st_mtime));
/* Server */
	strcat(header, "Server: Gabby\n");
/* Version */
	strcat(header, "Version: HTTP/1.1\n");
	strcat(header, "Connection: close\n");
/* End of header */
	strcat(header, "\n");

}

/* Sends the header and targeted file to the client socket */
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

/* Output an error when requested file doesn't exist */
void output_dne(int sock, char* fileName)
{
    char str[50];
    int n;
    
    sprintf(str, "The file %s does not exist\n", fileName);
    n = write(sock, str, strlen(str));
    if (n < 0) error("ERROR writing to socket");
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
int dostuff (int sock)
{
    int n;
    char buffer[1024];
    char header[1024];
    enum content_type ctype; 
    char fileName[256];
    char filePath[256];
    int resource;

    printf("--------------------------Start of a new request---------------------------\n");
    printf("Here is the message from the client: \n%s",buff);
    /* get file name */
    getFileName(buff,fileName);
    printf("Targeted file name: %s\n",fileName);
    /* get file path in the server */
    sprintf(filePath, "resource/%s",fileName);
    printf("Targeted file path in the server: %s\n",filePath);
    /* get file extension */
    ctype = getContentType(fileName);

    /* Check whether the requested file exists or not. If the file does not exist, print out an error */
    if ((resource = open(filePath, O_RDONLY)) > 0){
    	buildHeader(header,fileName, ctype);
    	printf("\nFile exists, therefore the following respond header will be sent: \n%s",header);
    	output_header_and_targeted_file_to_sock(sock, resource, header, filePath);
    }
    else{
    	printf("\nFile doesn't exist! \n");
    	output_dne(sock, fileName);
    }

    /* Shuts down and closes the socket */
    shutdown(sock, SHUT_RDWR);
//    close(sock);
    return ;
}
