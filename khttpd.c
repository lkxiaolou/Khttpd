#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<string.h>
#include<pthread.h>
#include<stdarg.h>

#define DEFAULT_PORT 8080
#define DEFAULT_IS_DAEMON 1
#define TIME_PADDING 5
#define DEFAULT_DOC "index.html"
#define SERVER_NAME "Server: Khttpd\r\n"

/**
 * the config struct
 * read from command line
 * or read from the config file
 */
struct khttpd_config {
	int port;
	int is_daemon;
};

int get_port(int argc, char *argv[], struct khttpd_config *k_config);
int init(int port, struct sockaddr_in *addr);
void *deal_request(void *arg);
int get_method(char *str, char *method);
int get_url(char *str, char *url);
int send_result(int fd, char *url, int cgi);
int create_header(int fd, int status);

/**
 * get config from command line
 * eg -p 8080, -p8080
 * -d 0 -d 1
 */
void get_config(int argc, char *argv[], struct khttpd_config *k_config)
{
	int oc;
	int port = 0;
	int is_daemon = -1;
	while((oc = getopt(argc, argv, "p:d:")) != -1)
	{
		switch(oc)
		{
			case 'p' : port = atoi(optarg); break;
			case 'd' : is_daemon = atoi(optarg); break;
		}
	}
	k_config->port = port ? port : DEFAULT_PORT;
	k_config->is_daemon = (is_daemon != -1) ? is_daemon : DEFAULT_IS_DAEMON;
}

/**
 * initizlizze the socket
 * socket->bind->listen
 */
int init(int port, struct sockaddr_in *addr)
{
	int fd;
	fd  = socket(PF_INET, SOCK_STREAM, 0);
	if(fd == -1){
		perror("create socket failed");
		exit(-1);
	}
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(port);
	if(bind(fd,(struct sockaddr *)addr, sizeof(struct sockaddr_in)) == -1){
		perror("bind failed");
		exit(-1);
	}
	if(listen(fd, TIME_PADDING) == -1){
		perror("listen failed");
		exit(-1);
	}
	printf("khttpd listening on port %d...\n", port);
	return fd;
}

/**
 * deal the request
 */
void *deal_request(void *arg)
{
	char buf[1024];
	char method[7];
	char url[100];
	int newfd;
	int res1, res2, cgi = 0;
	newfd =  *((int *)arg);
	res1 = read_line(newfd, buf, sizeof(buf));
	get_url(buf, url);
	if(strlen(url) == 0){
		strcpy(url, DEFAULT_DOC);
	}
	if(res1){
		res2 = get_method(buf, method);
	}
	if(res2){
		//get
		if(strcmp(method, "get") == 0){
			send_result(newfd, url, cgi);
		}
		//post
		if(strcmp(method, "post") == 0){
			cgi = 1;
			printf("post\n");
		}
	}
	close(newfd);
}

/**
 * get the request url
 */
int get_url(char *str, char *url)
{
	int i;
	int count,sp,j = 0;
	for(i = 0; str[i] !='\r'; i++){
		if(str[i] == '/'){
			count++;
		}
		if(count == 1 && str[i] == ' '){
			sp++;
		}
		if(count == 1 && str[i] != '/' && sp == 0){
			url[j] = str[i];
			j++;
		}
	}
	url[j] = '\0';
	return 0;
}

/**
 * send a file or a result from the shell script
 * with the flag 'cgi' eq 1
 *
 */
int send_result(int fd, char *url, int cgi)
{
	char buf[1024];
	FILE *fp;
	buf[0] = 'A'; buf[1] = '\0';
	int numchars = 1;
	while ((numchars > 0) && strcmp("\n", buf))
	{
		numchars = read_line(fd, buf, sizeof(buf));
	}
	if(cgi == 0){
		fp = fopen(url, "r");
		if(fp == NULL){
			create_header(fd, 404);
		}else{
			create_header(fd, 200);
			//send file
			fgets(buf, sizeof(buf), fp);
			while(!feof(fp))
			{
				send(fd, buf, strlen(buf), 0);
				fgets(buf, sizeof(buf), fp);
			}
			fclose(fp);
		}
	}
	close(fd);
}

/**
 * create the http header
 */
int create_header(int fd, int status)
{
	char buf[1024];
	int res;
	switch(status)
	{
		case 200:
			sprintf(buf, "HTTP/1.0 200 OK\r\n");
			send(fd, buf, strlen(buf), 0);
			sprintf(buf, SERVER_NAME);
			send(fd, buf, strlen(buf), 0);
			sprintf(buf, "Content-type: text/html\r\n");
			send(fd, buf, strlen(buf), 0);
			sprintf(buf, "\r\n");
			send(fd, buf, strlen(buf), 0);
			break;
		case 404:
			strcpy(buf, "HTTP/1.0 404 NOT FOUND\r\n");
			send(fd, buf, strlen(buf), 0);
			strcpy(buf, SERVER_NAME);
			send(fd, buf, strlen(buf), 0);
			sprintf(buf, "Content-type: text/html\r\n");
			send(fd, buf, strlen(buf), 0);
			strcpy(buf, "\r\n");
			send(fd, buf, strlen(buf), 0);
			strcpy(buf, "<html><title>404 not found</title>\r\n");
			send(fd, buf, strlen(buf), 0);
			strcpy(buf, "<body><h1>Khttpd can not found the page</h1></body></html>\r\n");
			send(fd, buf, strlen(buf), 0);
			break;
	}
	return 0;
}

/**
 * find the method from the 
 * first line of http request head
 * and format it to lower case
 */
int get_method(char *str, char *method)
{
	int i = 0;
	for(i = 0; str[i] != ' '; i++){
		//all char format to lower
		if(str[i] >= 65 && str[i] <= 90){
			str[i] += 32;
		}
		method[i] = str[i];
	}
	method[i] = '\0';
	return i;
}

/**
 * read a line from socket
 * http request's line end with a '\r' and a '\n'
 * whitch you can see it using wireshark
 */
int read_line(int fd, char *buf, int size)
{
	char c;
	int i = 0;
	while(i < size){
		recv(fd, &c, 1, 0);
		if(c == '\r'){
			recv(fd, &c, 1, 0);
		}
		if(c == '\n'){
			break;
		}
		if(c == '\0'){
			break;
		}
		buf[i] = c;
		i++;
	}
	buf[i] = '\0';
	return i;
}

int main(int argc, char *argv[])
{
	int fd, newfd, addr_len,status;
	struct khttpd_config k_config;
	struct sockaddr_in addr;
	pthread_t pid;
	get_config(argc, argv, &k_config);
	fd = init(k_config.port, &addr);
	//is daemon ? 
	if(k_config.is_daemon){
		daemon(1, 0);
	}
	while(1)
	{
		addr_len = sizeof(addr);
		newfd = accept(fd, (struct sockaddr *)&addr, &addr_len);
		if(newfd == -1){
			exit(-1);
		}
		//deal with the accept with a new thread
		if(pthread_create(&pid, NULL, deal_request, &newfd) != 0){
			perror("create thread error");
		}
	}
	return 0;
}
