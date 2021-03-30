#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "header/consts.h"
#include "header/utils.h"
#include "header/proc.h"

const int TYPE_COUNT = 6;
const char* TYPES[] = {"REG", "CHR", "DIR", "FIFO", "SOCK", "unknown"};

// 0: status, 1: -c, 2: -t, 3: -f
char** parse_args(int argc, char **argv)
{
	char** args = new char*[4];
	for(int i = 0; i < 4; i++) args[i] = new char[ARG_LEN];
	char opt;
	while((opt = getopt(argc, argv, "c:t:f:")) != -1)
	{
		switch(opt)
		{
			case 'c':
				args[1] = optarg;
				break;
			case 't':
				args[2] = optarg;
				break;
			case 'f':
				args[3] = optarg;
				break;
			case '?':
				strcpy(args[0], "invalid");
		}
	}
	return args;
}

void set_command(proc* p)
{
	char* sta;
	char* path = proc_path(p->pid);
	char* cmd  = new char[CMD_LEN];
	char* reg  = new char[REG_LEN];
	// int count;
	strcat(path, "/stat");
	strcpy(reg, "\\(.*\\)");
	sta = read_file(path);
	cmd = match_regex(sta, reg);
	++cmd; cmd[strlen(cmd) - 1] = 0;
	p->command = cmd;
}

void set_user(proc* p)
{
	struct stat sta;
	char* path = proc_path(p->pid);
	stat(path, &sta);
	char* user = get_username(sta.st_uid);
	p->user = user;
}

void set_fd(proc* p)
{
	char* path = proc_path(p->pid);
	char* cwd_path = new char[ENTRY_LEN];
	strcpy(cwd_path, path);
	strcat(cwd_path, "/cwd");
	char* root_path = new char[ENTRY_LEN];
	strcpy(root_path, path);
	strcat(root_path, "/root");
	char* exe_path = new char[ENTRY_LEN];
	strcpy(exe_path, path);
	strcat(exe_path, "/exe");
	char* mem_path = new char[ENTRY_LEN];
	strcpy(mem_path, path);
	strcat(mem_path, "/maps");
	char* fd_path = new char[ENTRY_LEN];
	strcpy(fd_path, path);
	strcat(fd_path, "/fd");

	int len;
	// === cwd ===
	char* cwd = new char[ENTRY_LEN];
	len = readlink(cwd_path, cwd, ENTRY_LEN);
	if(len == -1) // readlink fail
	{
		strcat(cwd_path, " (readlink: Permission denied)");
		add_entry(p, "cwd", "unknown", "", cwd_path);
	}
	else 
	{
		cwd[len] = 0;
		add_entry(p, "cwd", "DIR", get_inode(cwd), cwd);
	}

	// === root ===
	char* root = new char[ENTRY_LEN];
	len = readlink(root_path, root, ENTRY_LEN); 
	if(len == -1)
	{
		strcat(root_path, " (readlink: Permission denied)");
		add_entry(p, "root", "unknown", "", root_path);	
	}
	else
	{
		root[len] = 0;
		add_entry(p, "root", "DIR", get_inode(root), root);
	}

	// === exe ===
	char* exe = new char[ENTRY_LEN];
	len = readlink(exe_path, exe, ENTRY_LEN);
	if(len == -1)
	{
		strcat(exe_path, " (readlink: Permission denied)");
		add_entry(p, "exe", "unknown", "", exe_path);
	}
	else
	{
		exe[len] = 0;
		add_entry(p, "exe", "REG", get_inode(exe), exe);
	}

	// === mem ===
	FILE* fp = fopen(mem_path, "r");
	if(fp != NULL)
	{
		char* line = NULL;
		size_t size = 0;
		ssize_t read;
		char* reg = new char[REG_LEN];
		strcpy(reg, "/.*");
		char* prev = new char[REG_LEN];
		strcpy(prev, "");
		while((read = getline(&line, &size, fp)) != -1)
		{
			char* match = match_regex(line, reg);
			if(match == NULL) continue;
			match[strlen(match) - 1] = 0;
			if(strcmp(prev, match) == 0) continue;
			add_entry(p, "mem", "REG", get_inode(match), match);
			strcpy(prev, match);
		}
	}

	// === fd ===
	DIR *dir = opendir(fd_path);
	if(!dir)
	{
		strcat(fd_path, " (opendir: Permission denied)");
		add_entry(p, "NOFD", "", "", fd_path);
	}
	else
	{
		dirent* dent;
		char* fd = new char[FILE_LEN];
		char* full_path = new char[PATH_LEN];
		char* real_path = new char[PATH_LEN];
		struct stat sta;
		while((dent = readdir(dir)))
		{
			if(!is_number(dent->d_name)) continue;

			strcpy(full_path, fd_path);
			strcat(full_path, "/");
			strcat(full_path, dent->d_name);

			stat(full_path, &sta);
			len = readlink(full_path, real_path, PATH_LEN); real_path[len] = 0;

			strcpy(fd, "");
			strcat(fd, dent->d_name);
			strcat(fd, get_rwu(full_path));
			
			if(S_ISDIR(sta.st_mode))
				add_entry(p, fd, "DIR", get_inode(full_path), real_path);
			else if(S_ISREG(sta.st_mode))
				add_entry(p, fd, "REG", get_inode(full_path), real_path);
			else if(S_ISCHR(sta.st_mode))
				add_entry(p, fd, "CHR", get_inode(full_path), real_path);
			else if(S_ISFIFO(sta.st_mode))
				add_entry(p, fd, "FIFO", get_inode(full_path), real_path);
			else if(S_ISSOCK(sta.st_mode))
				add_entry(p, fd, "SOCK", get_inode(full_path), real_path);
			else 
				add_entry(p, fd, "unknown", get_inode(full_path), real_path);
		}
		closedir(dir);
	}
}

int main(int argc, char **argv)
{
	// parse arguments
	char** args = parse_args(argc, argv);

	// end and print usage on invalid input
	if(strcmp(args[0], "invalid") == 0)
	{
		print_usage(argv[0]);
		return 0;
	}

	// -t TYPE check
	if(strlen(args[2]) != 0)
	{
		bool good = false;
		for(int i = 0; i < TYPE_COUNT; i++)
			if(strcmp(TYPES[i], args[2]) == 0)
				good = true;
		if(!good)
		{
			printf("Invalid TYPE option.\n");
			return 0;
		}
	}

	dirent* dent;
	DIR* dir = opendir("/proc");
	print_head();
	while((dent = readdir(dir)))
	{
		// search next if dir name isn't a number
		if(!is_number(dent->d_name)) continue;

		proc p;
		// === pid ===
		p.pid = dent->d_name;

		// === command ===
		set_command(&p);

		// === USER ===
		set_user(&p);

		// initialize arrays
		p.fd   = new char*[ENTRY_ARRAY_LEN];
		p.type = new char*[ENTRY_ARRAY_LEN];
		p.node = new char*[ENTRY_ARRAY_LEN];
		p.name = new char*[ENTRY_ARRAY_LEN];
		p.e = 0;

		// === FD ===
		set_fd(&p);

		// print
		print(&p, args);
	}
    closedir(dir);
}

// command: proc/pid/stat
// pid: folder name
// user: owner of folder (uid -> loginuid)
// FD: The file descriptor. The value shown in FD field can be one of the following cases.
// 	cwd: The current working directory, can be read from /proc/[pid]/cwd.
// 	root: root directory, can be read from /proc/[pid]/root.
// 	exe: program file of this process, can be read from /proc/[pid]/exe.
// 	mem: memory mapping information, can be read from /proc/[pid]/maps.
// 	del: indicate that the file or link has been deleted. You should show this value if there is a (deleted) mark right after the filename in memory maps.
// 	[0-9]+[rwu]: file descriptor and opened mode.
// 		The numbers show the file descriptor number of the opened file.
// 		The mode "r" means the file is opened for reading.
// 		The mode "w" means the file is opened for writing.
// 		The mode "u" means the file is opened for reading and writing.
// 	NOFD: /proc/[pid]/fd not exist
// TYPE:
// 	DIR: a directory. cwd and root is also classified as this type.
// 	REG: a regular file
// 	CHR: a character special file, for example
// 	crw-rw-rw- 1 root root 1, 3 Mar 17 17:31 /dev/null
// 	FIFO: a pipe, for examle
// 	A link to a pipe, e.g.,
// 	lr-x------ 1 terrynini38514 terrynini38514 64 Mar 17 19:55 5 -> 'pipe:[138394]'
// 	A file with p type (FIFO)
// 	prw------- 1 root root 0 Mar 17 19:54 /run/systemd/inhibit/11.ref
// 	SOCK: a socket, for example
// 	lrwx------ 1 terrynini38514 terrynini38514 64 Mar 17 19:55 1 -> 'socket:[136975]'
// 	unknown: Any other unlisted types. Alternatively, if a file has been deleted or is not accessible (e.g., permission denied), this column can show unknown.
// NODE:
// 	The i-node number of the file
// 	It can be blank or empty if and only if /proc/[pid]/fd is not accessible.
// NAME:
// 	Show the opened filename if it is a typical file or directory.
// 	Show pipe:[node number] if it is a symbolic file to a pipe, e.g.,
// 	l-wx------ 1 ta ta 64 三 8 02:11 91 -> 'pipe:[2669735]'
// 	Show socket:[node number] if it is a symbolic file to a socket, e.g.,
// 	lrwx------ 1 ta ta 64 三 8 02:11 51 -> 'socket:[2669792]'
// 	Append  (deleted) (note the space before the message) to the end of the value if the value for TYPE is del.
// 	Append  (opendir: Permission denied) if the access to /proc/pid/fd is failed due to permission denied.
// 	Append  (readlink: Permission denied) if the access to /proc/pid/(cwd|root|exe) is failed due to permission denied.