#ifndef __PROC_H__
#define __PROC_H__

struct proc
{
	char* command; // status
	char* pid;
	char* user;
	char** fd;
	char** type;
	char** node;
	char** name;
	int e;
};

#endif