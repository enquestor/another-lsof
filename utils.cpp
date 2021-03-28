#include "utils.h"
#include "consts.h"
#include "proc.h"
#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <regex.h>
#include <unistd.h>
#include <assert.h>

void print_line(const char* command, const char* pid, const char* user, const char* fd, const char* type, const char* node, const char* name)
{
	printf("%-26s\t%-9s%-10s%-9s%-10s%-10s%s\n", command, pid, user, fd, type, node, name);
}

void print_head()
{
	print_line("COMMAND", "PID", "USER", "FD", "TYPE", "NODE", "NAME");
}

void print(proc p)
{
	print_line(p.command, p.pid, p.user, "FD", "TYPE", "NODE", "NAME");
}

// checks if s is a number
bool is_number(char* s)
{
	char* p;
	strtol(s, &p, 10);
	return (*p == 0);
}

// converts s to int
int to_number(char* s)
{
	char* p;
	return strtol(s, &p, 10);
}

// returns first match of reg in str
char* match_regex(char* str, char* reg)
{
	regex_t rt;
	regmatch_t rm[1];
	assert(regcomp(&rt, reg, REG_EXTENDED) == 0);
	assert(regexec(&rt, str, 1, rm, 0) == 0);
	char* match = new char[REG_LEN];
	strncpy(match, str + rm[0].rm_so, rm[0].rm_eo - rm[0].rm_so);
	return match;
}

char* read_file(char* path)
{
	int fd;
	char* buf = new char[FILE_LEN];
	fd = open(path, O_RDONLY);
	read(fd, buf, FILE_LEN);
	close(fd);
	return buf;
}