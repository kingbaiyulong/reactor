#ifndef __REACTOR_H__
#define __REACTOR_H__
#include <unistd.h>
#include <map>
#include <iostream>
#include <stdlib.h>
#include <string>
#define RE_OK 0
#define RE_ERROR -1

#define RE_NONE 0
#define RE_READABLE 1
#define RE_WRITABLE 2
//file event
#define RE_FILE_EVENTS 1
//time event
#define RE_TIME_EVENTS 2
// all event
#define RE_ALL_EVENTS (RE_FILE_EVENTS|RE_TIME_EVENTS)
// 
#define RE_DONT_WAIT 4

#define INIT_EVENTS 1024
/*
 * the reactor to hold kinds of events
 */
class Reactor;
/* Types and data structures 
 *
 */
typedef void reFileProc(Reactor *reactor_, int fd, void *clientData, int mask);
typedef void reTimeProc(Reactor *reactor_, long long id, void *clientData);
typedef void reEventFinalizerProc(Reactor *reactor_, void *clientData);
typedef void reBeforeSleepProc(Reactor *reactor_);
/* info for the Multiplex epoll */
class Multiplex {
public:
    // epoll fd
    int epfd;
	int eventSize;
    // see man epoll
    struct epoll_event *events;
	
	/* default:init INIT_EVENTS events*/
	Multiplex(){
		events=(struct epoll_event *)malloc(sizeof(struct epoll_event)*INIT_EVENTS);
		eventSize=INIT_EVENTS;
	}
	~Multiplex(){
		free(events);
		events=NULL;
	}
};

class FileEvent {
public:
	FileEvent();
	FileEvent(const FileEvent&);
	
public:
	int mask;//RE_READABLE,RE_WRITABLE or AE_READABLE | AE_WRITABLE
	reFileProc *rFileFunc;
	reFileProc *wFileFunc;
	Multiplex *multiplex;
	void *clientData;
};
class TimeEvent {
public:
	 long long id; /* time event identifier. */
    long when_sec; /* seconds */
    long when_ms; /* milliseconds */
	reTimeProc *timeimeProc;
	reEventFinalizerProc *finalizerProc;
	Multiplex *multiplex_;
	TimeEvent *next;  /* point to next timeEvent*/
	void *clientData;
};
class ActiveEvent {
public:
	int fd;
	int mask;//RE_READABLE,RE_WRITABLE or AE_READABLE | AE_WRITABLE
	ActiveEvent(){}
	ActiveEvent(int fd,int mask){
		this->fd=fd;
		this->mask=mask;
	}
};
class Reactor{
public:
	Reactor();
	~Reactor();
	bool initMultiplex();//mainly call epoll_crate
	int ResizeMultiplexEvents(int setsize);
	void deleteMultiplex();
	int multiplexAddEvent(int fd,int mask);
	void multiplexDelEvent(int fd,int delMask);
	int getMultiplexActiveEvents(struct timeval *tvp);
	std::string getMultiplexName();
	int getEventSize();
	void stopReactor();
	int reCreateFileEvent( int fd, int mask,
        reFileProc *proc, void *clientData);
	int reModifyFileEvent(int fd, int mask,
        reFileProc *proc, void *clientData);
	void reDeleteFileEvent( int fd, int mask);
	int getFileEventMask(int fd);
	int proccessEvents(int flags);
	void reMain();
	reBeforeSleepProc *beforProc;
private:
	std::map<int,FileEvent> events_map;
	std::map<int,ActiveEvent> active_map;
	int stop;
	Multiplex *multiplex;
};
#endif