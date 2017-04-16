#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <signal.h>
#include "reactor.h"
#define PORT        8787
#define LISTENQ     5
#define IPADDRESS   "127.0.0.1"
#define MAXSIZE 1024
using namespace std;
static void sig_handler(int signo)    /* argument is signal number */
{
    if(signo == SIGUSR1)
        printf("received SIGUSR1\n");
    else if (signo == SIGUSR2)
        printf("received SIGUSR2\n");
    else if(signo==SIGPIPE)
        printf("received SIGPIPE\n");
}
 void do_read(Reactor *reactor_ , int fd, void *privdata, int mask)
{
	char buffer[MAXSIZE];
	memset(buffer,0,MAXSIZE);
	int nread=read(fd,buffer,MAXSIZE);
    if(nread == -1)
    {
		/* you can catch ECONNRESET、EHOSTUNREACH、ENETUNREACH here */
        cout<<"read error:"<<strerror(errno)<<endl;
        close(fd);
		reactor_->reDeleteFileEvent(fd,EPOLLIN);
    }
    else if (nread == 0)
    {
        cout<<"client close.\n"<<endl;//means that the client fd has close
        close(fd);
		reactor_->reDeleteFileEvent(fd,EPOLLIN);
    }
    else
    {
        cout<<"read message is :"<<buffer<<endl;
        //修改描述符对应的事件，由读改为写
		//也可以直接往回读，一般业务采用此方法
		int nwrite;
		nwrite = write(fd,buffer,strlen(buffer));
		if (nwrite == -1)
		{
			perror("write error:");
			close(fd);
		}
        //modify_event(epollfd,fd,EPOLLOUT);
    }
	return;
}
 void do_write(Reactor *reactor_ , int fd, void *privdata, int mask)
{
	char buf[64]="abc";
    int nwrite;
    nwrite = write(fd,buf,strlen(buf));
    if (nwrite == -1)
    {
		/* you can catch ECONNRESET、EHOSTUNREACH、ENETUNREACH here */
        cout<<"write error:"<<strerror(errno)<<endl;
        close(fd);
    }
    memset(buf,0,MAXSIZE);
}
void do_accept(Reactor *reactor_,int fd, void *privdata, int mask){
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t  cliaddrlen;
    clifd = accept(fd,(struct sockaddr*)&cliaddr,&cliaddrlen);
    if (clifd == -1){
        cout<<"accpet error:"<<endl;
    }else
    {
        printf("accept a new client: %s:%d\n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
        //添加一个客户描述符和事件
        //add_event(epollfd,clifd,EPOLLIN);
		if(reactor_->reCreateFileEvent(clifd,RE_READABLE,do_read,NULL)==RE_ERROR){
			cout<<"set reader failed;"<<endl;
		}
    }
	return;
}
int socket_bind(const char* ip,int port)
{
	signal(SIGPIPE, sig_handler);
    int  listenfd;
    struct sockaddr_in servaddr;
    listenfd = socket(AF_INET,SOCK_STREAM,0);
    if (listenfd == -1)
    {
        perror("socket error:");
        exit(1);
    }
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port = htons(port);
	int flag=1,len=sizeof(int); 
	if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1) 
   { 
      cout<<("setsockopt")<<endl;
      exit(1); 
   }
    if (bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1)
    {
        cout<<("bind error: ")<<endl;;
        exit(1);
    }
    return listenfd;
}

int main(){
    int  listenfd;
    listenfd = socket_bind(IPADDRESS,PORT);
    listen(listenfd,LISTENQ);
	Reactor *reactor=new Reactor();
	/* set the listener */
	if (reactor->reCreateFileEvent(listenfd, RE_READABLE,
		do_accept,NULL) == RE_ERROR){
		 cout<<("set listener failed;")<<endl;
	}
	reactor->reMain();//start the reactor;
	return 0;
}