
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
							main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "keyboard.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"
#include "minesweeper.h"
#include "snake.h"



/*======================================================================*
							kernel_main
 *======================================================================*/
PUBLIC int kernel_main() {
	disp_str("-----\"kernel_main\" begins-----\n");
	struct task* p_task;
	struct proc* p_proc = proc_table;
	char* p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16   selector_ldt = SELECTOR_LDT_FIRST;
	u8    privilege;
	u8    rpl;
	int   eflags;
	int   i, j;
	int   prio;

	char running_string[] = "running ";
	char dead_string[] = "  dead  ";
	char paused_string[] = " paused ";

	// start the process
	for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
		if (i < NR_TASKS) {
			/* task */
			p_task = task_table + i;
			privilege = PRIVILEGE_TASK;
			rpl = RPL_TASK;
			eflags = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1   1 0010 0000 0010(2)*/
			prio = 20;     //set the priority to 15
		}
		else {
			/* user process */
			p_task = user_proc_table + (i - NR_TASKS);
			privilege = PRIVILEGE_USER;
			rpl = RPL_USER;
			eflags = 0x202; /* IF=1, bit 2 is always 1              0010 0000 0010(2)*/
			prio = 10;     //set the priority to 5
		}

		strcpy(p_proc->name, p_task->name); /* set prio name */
		p_proc->pid = i;            /* set pid */

		p_proc->run_count = 0;

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			sizeof(struct descriptor));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			sizeof(struct descriptor));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs = (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss = (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->p_flags = 0;
		p_proc->p_msg = 0;
		p_proc->p_recvfrom = NO_TASK;
		p_proc->p_sendto = NO_TASK;
		p_proc->has_int_msg = 0;
		p_proc->q_sending = 0;
		p_proc->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p_proc->filp[j] = 0;

		p_proc->ticks = p_proc->priority = prio;
		memcpy(p_proc->run_state, running_string, 8);
		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	//memcpy(proc_table[4].run_state, dead_string, 8);
	//memcpy(proc_table[5].run_state, dead_string, 8);

	// init process
	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	init_clock();
	init_keyboard();

	restart();

	while (1) {}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}

PUBLIC void addTwoString(char *to_str, char *from_str1, char *from_str2) {
	int i = 0, j = 0;
	while (from_str1[i] != 0)
		to_str[j++] = from_str1[i++];
	i = 0;
	while (from_str2[i] != 0)
		to_str[j++] = from_str2[i++];
	to_str[j] = 0;
}

char users[2][128] = { "root","shen"};
char passwords[2][128] = { "root","yujiao"};

//包含进程的操作 文件 启动游戏
void shell(char *tty_name) {

	int fd;
	//int isLogin = 0;
	char rdbuf[512];
	char cmd[512];
	char arg1[512];
	char arg2[512];
	char buf[1024];
	char temp[512];

	char running_string[] = "running ";
	char dead_string[] = "  dead  ";
	char paused_string[] = " paused ";

	int fd_stdin = open(tty_name, O_RDWR);
	assert(fd_stdin == 0);
	int fd_stdout = open(tty_name, O_RDWR);
	assert(fd_stdout == 1);
	animation();  // the start animation
	char current_dirr[512] = "/";
	char current_login[512];
	//login
	while (1) {
		printf("please login first)\n");
		clearArr(rdbuf, 512);
		clearArr(cmd, 512);
		clearArr(arg1, 512);
		clearArr(arg2, 512);
		clearArr(buf, 1024);
		clearArr(temp, 512);

		printf("username: ");
		int r = read(fd_stdin, rdbuf, 512);
		if (strcmp(rdbuf, "") == 0)
			continue;
		strcpy(current_login, rdbuf);
		int i = 0;
		int j = 0;
		while (rdbuf[i] != ' ' && rdbuf[i] != 0) {
			cmd[i] = rdbuf[i];
			i++;
		}
		i++;
		while (rdbuf[i] != ' ' && rdbuf[i] != 0) {
			arg1[j] = rdbuf[i];
			i++;
			j++;
		}
		i++;
		j = 0;
		while (rdbuf[i] != ' ' && rdbuf[i] != 0) {
			arg2[j] = rdbuf[i];
			i++;
			j++;
		}
		// clear
		rdbuf[r] = 0;
		char old_cmd[512];
		strcpy(old_cmd, cmd);
		int cnt = 0, flag = 0;
		for (cnt = 0; cnt < 2; cnt++) {
			if (strcmp(old_cmd, users[cnt]) == 0) {
				printf("password: ");
				clearArr(rdbuf, 512);
				clearArr(cmd, 512);
				clearArr(arg1, 512);
				clearArr(arg2, 512);
				clearArr(buf, 1024);
				clearArr(temp, 512);
				int r = read(fd_stdin, rdbuf, 512);
				if (strcmp(rdbuf, "") == 0)
					continue;
				int i = 0;
				int j = 0;
				while (rdbuf[i] != ' ' && rdbuf[i] != 0) {
					cmd[i] = rdbuf[i];
					i++;
				}
				i++;
				while (rdbuf[i] != ' ' && rdbuf[i] != 0) {
					arg1[j] = rdbuf[i];
					i++;
					j++;
				}
				i++;
				j = 0;
				while (rdbuf[i] != ' ' && rdbuf[i] != 0) {
					arg2[j] = rdbuf[i];
					i++;
					j++;
				}
				// clear
				rdbuf[r] = 0;
				if (strcmp(cmd, passwords[cnt]) == 0) {
					flag = 1;
				}
				else {
					printf("invalid password!\n");
				}
			}
		}
		if (flag == 0) {
			printf("no valid username!\n");
		}
		else {
			printf("login sucessfully~\n");
			break;
		}
	}

	menu();
	while (1) {
		// clear the array ！
		clearArr(rdbuf, 512);
		clearArr(cmd, 512);
		clearArr(arg1, 512);
		clearArr(arg2, 512);
		clearArr(buf, 1024);
		clearArr(temp, 512);

		printf("%s@os:~%s$ ", current_login, current_dirr);
		int r = read(fd_stdin, rdbuf, 512);
		if (strcmp(rdbuf, "") == 0)//no input and enter
			continue;
		int i = 0;
		int j = 0;
		while (rdbuf[i] != ' ' && rdbuf[i] != 0) {//strip command
			cmd[i] = rdbuf[i];
			i++;
		}
		i++;
		while (rdbuf[i] != ' ' && rdbuf[i] != 0) {//strip argument 1
			arg1[j] = rdbuf[i];
			i++;
			j++;
		}
		i++;
		j = 0;
		while (rdbuf[i] != ' ' && rdbuf[i] != 0) {//strip argument 2
			arg2[j] = rdbuf[i];
			i++;
			j++;
		}
		// clear
		rdbuf[r] = 0;

		if (strcmp(cmd, "process") == 0) {
			ProcessManage();
		}
		else if (strcmp(cmd, "menu") == 0) {
			menu();
		}
		else if (strcmp(cmd, "clear") == 0) {
			clear();
		}
		else if (strcmp(cmd, "ls") == 0) {
			ls(current_dirr);
		}
		//文件系统操作 tian 307-325
		else if (strcmp(cmd, "nf") == 0) {
			CreateFile(current_dirr, arg1);
		}
		else if (strcmp(cmd, "rm") == 0) {
			DeleteFile(current_dirr,arg1);
		}
		else if (strcmp(cmd, "pfc") == 0) {
			ReadFile(current_dirr, arg1);
		}
		else if (strcmp(cmd, "wf") == 0) {
			WriteFile(current_dirr, arg1);
		}
		else if (strcmp(cmd, "mkdir")==0){
			CreateDir(current_dirr,arg1);
		}
		else if (strcmp(cmd, "cd") == 0) {
			GoDir(current_dirr, arg1);
		}
		else if (strcmp(rdbuf, "pause 4") == 0) {
			memcpy(proc_table[4].run_state, paused_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "pause 5") == 0) {
			memcpy(proc_table[5].run_state, paused_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "pause 6") == 0) {
			memcpy(proc_table[6].run_state, paused_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "kill 4") == 0) {
			proc_table[4].p_flags = 1;
			memcpy(proc_table[4].run_state, dead_string, 8);
			ProcessManage();
			printf("cant kill this process!");
		}
		else if (strcmp(rdbuf, "kill 5") == 0) {
			proc_table[5].p_flags = 1;
			memcpy(proc_table[5].run_state, dead_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "kill 6") == 0) {
			proc_table[6].p_flags = 1;
			memcpy(proc_table[6].run_state, dead_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "resume 4") == 0) {
			memcpy(proc_table[4].run_state, running_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "resume 5") == 0) {
			memcpy(proc_table[5].run_state, running_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "resume 6") == 0) {
			memcpy(proc_table[6].run_state, running_string, 8);
			ProcessManage();
		}
		else if (strcmp(rdbuf, "up 4") == 0) {
			proc_table[4].priority = proc_table[4].priority * 2;
			ProcessManage();
		}
		else if (strcmp(rdbuf, "up 5") == 0) {
			proc_table[5].priority = proc_table[5].priority * 2;
			ProcessManage();
		}
		else if (strcmp(rdbuf, "up 6") == 0) {
			proc_table[6].priority = proc_table[6].priority * 2;
			ProcessManage();
		}

	
		else if (strcmp(cmd, "minesweeper") == 0) {
			game(fd_stdin);
		}
		else if (strcmp(cmd, "snake") == 0) {
			snakeGame();
		}
	
		
		else
			printf("Invalid Input!\n");
	}
}

/*======================================================================*
							   TestA
 *======================================================================*/

 //A process
void TestA() {
	shell("/dev_tty0");
	assert(0);
}

/*======================================================================*
							   TestB
 *======================================================================
*/
//B process
void TestB() {
	shell("/dev_tty1");
	assert(0); /* never arrive here */
}
/*======================================================================*
							   TestC
 *======================================================================*/
 //C process
void TestC() {
	//shell("/dev_tty2");
	assert(0);
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...) {
	int i;
	char buf[256];
	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);
	i = vsprintf(buf, fmt, arg);
	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);
	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

/*****************************************************************************
 *                                Custom Command
 *****************************************************************************/
char* findpass(char *src) {
	char pass[128];
	int flag = 0;
	char *p1, *p2;

	p1 = src;
	p2 = pass;

	while (p1 && *p1 != ' ')
	{
		if (*p1 == ':')
			flag = 1;

		if (flag && *p1 != ':')
		{
			*p2 = *p1;
			p2++;
		}
		p1++;
	}
	*p2 = '\0';

	return pass;
}

void clearArr(char *arr, int length) {
	int i;
	for (i = 0; i < length; i++)
		arr[i] = 0;
}

void clear() {
	clear_screen(0, console_table[current_console].cursor);
	console_table[current_console].crtc_start = console_table[current_console].orig;
	console_table[current_console].cursor = console_table[current_console].orig;
}




/*======================================================================*
						menu 478-483（新增）
 *======================================================================*/
void menu() {
	printf("=============================================================================\n");
	printf("                             Select Your Operation                           \n");
	printf("=============================================================================\n");
	printf("    clear                         : clear the screen                         \n");
	printf("    ls                            : list files in current directory          \n");
	printf("    minesweeper                   : start the minesweeper game               \n");
	printf("    snake                         : start the snake game                     \n");
	printf("    process                       : display all process-info and manage      \n");
	printf("    nf                            : create a new file                        \n");
	printf("    rm                            : delete a file                            \n");
	printf("    pfc                           : print file content                       \n");
	printf("    wf                            : write a file                             \n");
	printf("    mkdir                         : create a new directory                   \n");
	printf("    cd                            : change current directory                 \n");
	printf("=============================================================================\n");
}

void ProcessManage()
{
	int i;
	printf("=============================================================================\n");
	printf("            PID      |    name       |      priority    |     running        \n");
	printf("-----------------------------------------------------------------------------\n");
	for (i = 0; i < NR_TASKS + NR_PROCS; ++i) {
		printf("%13d%17s%19d%19s\n",
			proc_table[i].pid, proc_table[i].name, proc_table[i].priority, proc_table[i].run_state);
	}
	printf("=============================================================================\n");
	printf("=          Usage: pause  [pid]  you can pause one process                   =\n");
	printf("=          	      resume [pid]  you can resume one process                  =\n");
	printf("=                 kill   [pid]  kill the process                            =\n");
	printf("=                 up     [pid]  improve the process priority                =\n");
	printf("=============================================================================\n");
}
/*======================================================================*
					文件系统操作   516-675
 *======================================================================*/
void CreateFile(char* path, char* file)//新建文件
{
	char absoPath[512];
	addTwoString(absoPath, path, file);

	int fd = open(absoPath, O_CREAT | O_RDWR);

	if (fd == -1)
	{
		printf("Failed to create a new file with name %s\n", file);
		return;
	}

	char buf[1] = { 0 };
	write(fd, buf, 1);
	printf("File created: %s (fd %d)\n", file, fd);
	close(fd);
}

void DeleteFile(char* path, char* file)//删除文件
{
	char absoPath[512];
	addTwoString(absoPath, path, file);
	int m = unlink(absoPath);
	if (m == 0)
		printf("%s deleted!\n", file);
	else
		printf("Failed to delete %s!\n", file);
}

void ReadFile(char* path, char* file)//读文件
{
	char absoPath[512];
	addTwoString(absoPath, path, file);
	int fd = open(absoPath, O_RDWR);
	if (fd == -1)
	{
		printf("Failed to open %s!\n", file);
		return;
	}

	char buf[4096];
	int n = read(fd, buf, 4096);
	if (n == -1)  // 读取文件内容失败
	{
		printf("An error has occured in reading the file!\n");
		close(fd);
		return;
	}

	printf("%s\n", buf);
	close(fd);
}

void WriteFile(char* path, char* file)//写文件
{
	char absoPath[512];
	addTwoString(absoPath, path, file);
	int fd = open(absoPath, O_RDWR);
	if (fd == -1)
	{
		printf("Failed to open %s!\n", file);
		return;
	}

	char tty_name[] = "/dev_tty0";
	int fd_stdin = open(tty_name, O_RDWR);
	if (fd_stdin == -1)
	{
		printf("An error has occured in writing the file!\n");
		return;
	}
	char writeBuf[4096];  // 写缓冲区
	int endPos = read(fd_stdin, writeBuf, 4096);
	writeBuf[endPos] = 0;
	write(fd, writeBuf, endPos + 1);  // 结束符也应写入
	close(fd);
}

void CreateDir(char* path, char* file)//新建目录
{
	char absoPath[512];
	addTwoString(absoPath, path, file);
	int fd = open(absoPath, O_RDWR);

	if (fd != -1)
	{
		printf("Failed to create a new directory with name %s\n", file);
		return;
	}
	mkdir(absoPath);
}

void GoDir(char* path, char* file)//切换当前目录
{
	int flag = 0;  // 判断是进入下一级目录还是返回上一级目录
	char newPath[512] = { 0 };
	if (file[0] == '.' && file[1] == '.')  // cd ..返回上一级目录
	{
		flag = 1;
		int pos_path = 0;
		int pos_new = 0;
		int i = 0;
		char temp[128] = { 0 };  // 用于存放某一级目录的名称
		while (path[pos_path] != 0)
		{
			if (path[pos_path] == '/')
			{
				pos_path++;
				if (path[pos_path] == 0)  // 已到达结尾
					break;
				else
				{
					temp[i] = '/';
					temp[i + 1] = 0;
					i = 0;
					while (temp[i] != 0)
					{
						newPath[pos_new] = temp[i];
						temp[i] = 0;  
						pos_new++;
						i++;
					}
					i = 0;
				}
			}
			else
			{
				temp[i] = path[pos_path];
				i++;
				pos_path++;
			}
		}
	}
	char absoPath[512];
	char temp[512];
	int pos = 0;
	while (file[pos] != 0)
	{
		temp[pos] = file[pos];
		pos++;
	}
	temp[pos] = '/';
	temp[pos + 1] = 0;
	if (flag == 1)  // 返回上一级目录
	{
		temp[0] = 0;
		addTwoString(absoPath, newPath, temp);
	}
	else  // 进入下一级目录
		addTwoString(absoPath, path, temp);
	int fd = open(absoPath, O_RDWR);
	if (fd == -1)
		printf("%s is not a directory!\n", absoPath);
	else
		memcpy(path, absoPath, 512);
}


/*======================================================================*
							welcome
 *======================================================================*/
void animation() {
	clear();
	printf("_____________________________________________________\n");
	printf("...................................................\n");
	printf(".................oooooo.......sssssss..............\n");
	printf("...............oooooooooo....sssssssss.............\n");
	printf("..............ooo......ooo..sss....................\n");
	printf("..............ooo......ooo..sss....................\n");
	printf("..............ooo......ooo...ssssssss..............\n");
	printf("..............ooo......ooo.........ssss............\n");
	printf("...............oooooooooo....ssssssssss............\n");
	printf(".................oooooo.......sssssss..............\n");
	printf("...................................................\n");
	printf("_____________________________________________________\n");
	milli_delay(20000);
	clear();
}
