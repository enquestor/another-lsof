#include "../header/utils.h"
#include "../header/consts.h"
#include "../header/proc.h"
#include <cstdio>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <regex.h>
#include <unistd.h>
#include <assert.h>
#include <pwd.h>

void print_usage(char* prog_name)
{
	printf("Usage:\n");
	printf("  %s [ -c REGEX COMMAND filter ]\n", prog_name);
	printf("  %s [ -t TYPE filter ]\n", prog_name);
	printf("  %s [ -f REGEX NAME filter ]\n", prog_name);
}

void print_line(const char* command, const char* pid, const char* user, const char* fd, const char* type, const char* node, const char* name)
{
	printf("%-20s\t%-9s%-10s%-9s%-10s%-10s%s\n", command, pid, user, fd, type, node, name);
}

void print_head()
{
	print_line("COMMAND", "PID", "USER", "FD", "TYPE", "NODE", "NAME");
}

void print(proc p)
{
	for(int i = 0; i < p.e; i++)
		print_line(p.command, p.pid, p.user, p.fd[i], p.type[i], p.node[i], p.name[i]);
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

char* to_string(long unsigned int n, int len)
{
	char* str = new char[len];
	sprintf(str, "%lu", n);
	return str;
}

char* match_regex(char* str, char* reg)
{
	regex_t rt;
	regmatch_t rm;
	assert(regcomp(&rt, reg, REG_EXTENDED) == 0);
	assert(regexec(&rt, str, 1, &rm, 0) == 0);
	char* match = new char[REG_LEN];
	strncpy(match, str + rm.rm_so, rm.rm_eo - rm.rm_so);
	match[rm.rm_eo - rm.rm_so] = 0;
	return match;
}

// char** match_regex(char* str, char* reg, int* count) {
// 	regex_t rt;
// 	regmatch_t rm;
// 	assert(regcomp(&rt, reg, REG_EXTENDED) == 0);
//     // we just need the whole string match in this example    

//     // we store the eflags in a variable, so that we can make
//     // ^ match the first time, but not for subsequent regexecs
//     int eflags = 0;
// 	int i = 0;
//     size_t offset = 0;
//     size_t length = strlen(str);
// 	char* now_str = str;
// 	char** matches = new char*[REGEX_ARRAY_LEN];

//     while (regexec(&rt, now_str, 1, &rm, eflags) == 0)
// 	{
//         // do not let ^ match again.
//         eflags = REG_NOTBOL;
// 		matches[i] = new char[REG_LEN];
// 		strncpy(matches[i], now_str + rm.rm_so, rm.rm_eo - rm.rm_so);

//         // increase the starting offset
//         now_str += rm.rm_eo;

//         // a match can be a zero-length match, we must not fail
//         // to advance the pointer, or we'd have an infinite loop!
//         if (rm.rm_so == rm.rm_eo)
//             now_str += 1;
		
// 		i++;

//         // break the loop if we've consumed all characters. Note
//         // that we run once for terminating null, to let
//         // a zero-length match occur at the end of the string.
//         if (now_str >= str + length)
//             break;
//     }
// 	*count = i;
// 	return matches;
// }

bool exists(char* path)
{
	return access(path, R_OK);
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

char* get_inode(char* path)
{
	struct stat sta;
	stat(path, &sta);
	char* inode = to_string(sta.st_ino, ENTRY_LEN);
	return inode;
}

char* get_rwu(char* path)
{
	bool r = (access(path, R_OK) == 0);
	bool w = (access(path, W_OK) == 0);
	char* ret = new char[2];
	if(r && w) ret[0] = 'u';
	if(r) ret[0] = 'r';
	if(w) ret[0] = 'w';
	ret[1] = 0;
	return ret;
}

char* get_username(int uid)
{
	passwd *pw = getpwuid(uid);
	return pw->pw_name;
}

char* proc_path(char* pid)
{
	char* path = new char[PATH_LEN];
	strcpy(path, "/proc/");
	strcat(path, pid);
	return path;
}

void add_entry(proc* p, const char* fd, const char* type, const char* node, const char* name)
{
	char* tmp;
	tmp = new char[ENTRY_LEN];
	strcpy(tmp, fd);
	p->fd[p->e]   = tmp;
	tmp = new char[ENTRY_LEN];
	strcpy(tmp, type);
	p->type[p->e] = tmp;
	tmp = new char[ENTRY_LEN];
	strcpy(tmp, node);
	p->node[p->e] = tmp;
	tmp = new char[ENTRY_LEN];
	strcpy(tmp, name);
	p->name[p->e] = tmp;
	++p->e;
}