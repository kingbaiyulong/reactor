#include <sys/epoll.h>
#include <assert.h>
#include "reactor.h"
#include <iostream>
using namespace std;
FileEvent::FileEvent(){
	this->mask=0;
}
FileEvent::FileEvent(const FileEvent &c){
	this->mask=c.mask;
	this->rFileFunc=c.rFileFunc;
	this->wFileFunc=c.wFileFunc;
	this->multiplex=c.multiplex;
	this->clientData=c.clientData;
}
/* the constructor mainly call init(0 to init epoll */
Reactor::Reactor(){
	this->stop=0;//not stop
	if(!initMultiplex()){
		this->multiplex==NULL;
		std::cout<<"failed building epoll object"<<std::endl;
	}
}
Reactor::~Reactor(){
	/*delete*/
}
/* create an epoll object to multiplex*/
bool Reactor::initMultiplex(){
	Multiplex *tem=new Multiplex();
	tem->epfd=epoll_create(1024);/* 1024 is just a hint for the kernel */
	//cout<<"EPOLL_FD:"<<tem->epfd<<endl;
    if (tem->epfd == -1) {
        return false;
    }
	this->multiplex=tem;
	return true;
}
int Reactor::ResizeMultiplexEvents(int setsize){
	Multiplex *tem=this->multiplex;
	assert(tem);
	tem->events=(epoll_event*)realloc(tem->events, sizeof(struct epoll_event)*setsize);
	return 0;
}
void Reactor::deleteMultiplex(){
	Multiplex *tem=this->multiplex;
	delete tem;
	tem==NULL;
}
int Reactor::multiplexAddEvent(int fd,int mask){
	struct epoll_event ee;
	int op=this->events_map.find(fd)==this->events_map.end()?
				EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    // 注册事件到 epoll
    ee.events = 0;
    //mask |= this->events_map.at(fd).mask; /* Merge old events */
    if (mask & RE_READABLE) ee.events |= EPOLLIN;
    if (mask & RE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (epoll_ctl(this->multiplex->epfd,op,fd,&ee) == -1) return -1;
    return 0;
}
void Reactor::multiplexDelEvent(int fd,int del_mask){
    struct epoll_event ee;
	if(this->events_map.find(fd)==this->events_map.end())
		return;
    int mask = this->events_map.at(fd).mask & ( ~ del_mask);
    ee.events = 0;
    if (mask & RE_READABLE) ee.events |= EPOLLIN;
    if (mask & RE_WRITABLE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0; /* avoid valgrind warning */
    ee.data.fd = fd;
    if (mask != RE_NONE) {
        epoll_ctl(this->multiplex->epfd,EPOLL_CTL_MOD,fd,&ee);
    } else {
        /* Note, Kernel < 2.6.9 requires a non null event pointer even for
         * EPOLL_CTL_DEL. */
        epoll_ctl(this->multiplex->epfd,EPOLL_CTL_DEL,fd,&ee);
    }
}
//
int Reactor::getMultiplexActiveEvents(struct timeval *tvp){
    int retval, numevents = 0;
    /* Specifying a timeout of -1 makes epoll_wait() wait indefi-
       nitely, while specifying a timeout equal to zero makes epoll_wait() to return immediately even if no events are  avail-
       able (return code equal to zero).*/
	this->active_map.clear();
    retval = epoll_wait(this->multiplex->epfd,this->multiplex->events,this->multiplex->eventSize,
            tvp ? (tvp->tv_sec*1000 + tvp->tv_usec/1000) : -1);

    // 有至少一个事件就绪？
    if (retval > 0) {
        int j;

        // 为已就绪事件设置相应的模式
        // 并加入到 eventLoop 的 fired 数组中
        numevents = retval;
        for (j = 0; j < numevents; j++) {
            int mask = 0;
            struct epoll_event *e = this->multiplex->events+j;

            if (e->events & EPOLLIN) mask |= RE_READABLE;
            if (e->events & EPOLLOUT) mask |= RE_WRITABLE;
            if (e->events & EPOLLERR) mask |= RE_WRITABLE;
            if (e->events & EPOLLHUP) mask |= RE_WRITABLE;
			
			
            //eventLoop->fired[j].fd = e->data.fd;
            //eventLoop->fired[j].mask = mask;
			
			this->active_map.insert(
				std::pair<int,ActiveEvent>(j,ActiveEvent(e->data.fd,mask)));
        }
    }
    
    return numevents;	
}
std::string Reactor::getMultiplexName() {
    return std::string("epoll");
}
int Reactor::getEventSize(){
	return this->events_map.size();
}
/*
 * stop reactor
 */
void Reactor::stopReactor() {
    this->stop = 1;
}
/* only for adding new fd to reactor;for modification, use reModifyFileEvent*/
int Reactor::reCreateFileEvent( int fd, int mask,
        reFileProc *proc, void *clientData)
{
	/* if find,we think the this old fd is now expired,just delete it.*/
 	if(this->events_map.find(fd)!=this->events_map.end()){
		reDeleteFileEvent(fd,events_map[fd].mask);
	}
	FileEvent tem_ev;
	//call epoll_ctl
	if(multiplexAddEvent(fd,mask)==-1)
		return RE_ERROR;
	tem_ev.mask |= mask;
    if (mask & RE_READABLE) tem_ev.rFileFunc = proc;
    if (mask & RE_WRITABLE) tem_ev.wFileFunc = proc;
	tem_ev.clientData = clientData;
	this->events_map[fd]=tem_ev;
	//cout<<"successfully add event:"<<fd<<" event_map size:"<<events_map.size()<<endl;
	return RE_OK;
}
int Reactor::reModifyFileEvent(int fd, int mask,
        reFileProc *proc, void *clientData){
 	if(this->events_map.find(fd)==this->events_map.end()){
		return RE_ERROR;
	}
	FileEvent tem_ev;
	//call epoll_ctl
	if(multiplexAddEvent(fd,mask)==-1)
		return RE_ERROR;
	tem_ev.mask |= mask;
    if (mask & RE_READABLE) tem_ev.rFileFunc = proc;
    if (mask & RE_WRITABLE) tem_ev.wFileFunc = proc;
	tem_ev.clientData = clientData;
	this->events_map[fd]=tem_ev;
	//cout<<"successfully add event:"<<fd<<" event_map size:"<<events_map.size()<<endl;
	return RE_OK;
}
void Reactor::reDeleteFileEvent( int fd, int mask){
	if(this->events_map.find(fd)==this->events_map.end())return;
	if(this->events_map.at(fd).mask==RE_NONE) return;
	this->events_map.at(fd).mask &= (~mask);
	multiplexDelEvent(fd,mask);
	if(this->events_map.at(fd).mask==RE_NONE)
		this->events_map.erase(fd);
}
int Reactor::getFileEventMask(int fd){
	if(this->events_map.find(fd)==this->events_map.end())
		return RE_NONE;
	return this->events_map.at(fd).mask;
}
/* return nums of events that reactor proccess */
int Reactor::proccessEvents(int flags){
	int processed = 0, numevents;
	struct timeval *tvp=NULL;//this should be set if timer is set 
	numevents = getMultiplexActiveEvents(tvp);
	for (int j = 0; j < numevents; j++) {
		//aeFileEvent *fe = &eventLoop->events[eventLoop->fired[j].fd];
		
		int mask = this->active_map.at(j).mask;//eventLoop->fired[j].mask;
		int fd = this->active_map.at(j).fd;//eventLoop->fired[j].fd;
		int rfired = 0;

	   /* note the fe->mask & mask & ... code: maybe an already processed
		 * event removed an element that fired and we still didn't
		 * processed, so we check if the event is still valid. */
		// 
		FileEvent &file_ev=this->events_map[fd];
		if (file_ev.mask & mask & RE_READABLE) {
			rfired = 1;
			file_ev.rFileFunc(this,fd,file_ev.clientData,mask);
		}
		if (file_ev.mask & mask & RE_WRITABLE) {
			if (!rfired || file_ev.wFileFunc != file_ev.rFileFunc)
				file_ev.wFileFunc(this,fd,file_ev.clientData,mask);
		}
		processed++;
	}
	return processed;
}
/* the main reactor loop */
void Reactor::reMain(){
    this->stop = 0;
    while (!this->stop) {
		//std::cout<<"run"<<std::endl;
        if (this->beforProc != NULL)
            this->beforProc(this);
		//begin to proccess events;
        this->proccessEvents(RE_ALL_EVENTS);
		//std::cout<<"run"<<std::endl;
    }
}