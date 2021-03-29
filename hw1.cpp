#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "header/consts.h"
#include "header/utils.h"
#include "header/proc.h"

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

char* get_command(char* pid)
{
	char* sta;
	char* path = proc_path(pid);
	char* cmd  = new char[CMD_LEN];
	char* reg  = new char[REG_LEN];
	strcat(path, "/stat");
	strcpy(reg, "\\(.*\\)");
	sta = read_file(path);
	cmd = match_regex(sta, reg);
	++cmd; cmd[strlen(cmd) - 1] = 0;
	delete path;
	delete sta;
	delete reg;
	return cmd;
}

char* get_user(char* pid)
{
	struct stat sta;
	char* path = proc_path(pid);
	stat(path, &sta);
	char* user = get_username(sta.st_uid);
	delete path;
	return user;
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

	dirent* dent;
	DIR *dir = opendir("/proc");
	print_head();
	while((dent = readdir(dir)))
	{
		// search next if dir name isn't a number
		if(!is_number(dent->d_name)) continue;

		proc p;
		// === pid ===
		p.pid = dent->d_name;

		// === command ===
		p.command = get_command(p.pid);

		// === USER ===
		p.user = get_user(p.pid);

		// === FD ===

		// print
		print(p);
	}
    closedir(dir);
}

// command: cmdline
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