
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

char users[1][128] = { "root"};
char passwords[1][128] = { "root"};

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
		for (cnt = 0; cnt < 1; cnt++) {
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






void menu() {
	printf("=============================================================================\n");
	printf("                             Select Your Operation                           \n");
	printf("=============================================================================\n");
	printf("    clear                         : clear the screen                         \n");
	printf("    ls                            : list files in current directory          \n");
	printf("    minesweeper                   : start the minesweeper game               \n");
	printf("    snake                         : start the snake game                     \n");
	printf("    process                       : display all process-info and manage      \n");
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
