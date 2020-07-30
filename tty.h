
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				tty.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
 
#ifndef _ORANGES_TTY_H_
#define _ORANGES_TTY_H_


#define TTY_IN_BYTES		256	/* tty input queue size */
#define TTY_OUT_BUF_LEN	2	/* tty output buffer size */

struct s_tty;
struct s_console;

/* TTY */
typedef struct s_tty
{
	u32	ibuf[TTY_IN_BYTES];	/* TTY input buffer */
	u32*	ibuf_head;		/* the next free slot */
	u32*	ibuf_tail;		/* the val to be processed by TTY */
	int	ibuf_cnt;		/* how many */
	char	tmpStr[TTY_IN_BYTES];	/*临时字符串*/
	int	tmpLen;
	char	str[TTY_IN_BYTES];	/*字符串*/
	int	len;
	int	tty_caller;
	int	tty_procnr;
	void*	tty_req_buf;
	int	tty_left_cnt;
	int	tty_trans_cnt;
	int	startScanf;
	struct s_console *	console;
}TTY;

#endif /* _ORANGES_TTY_H_ */
