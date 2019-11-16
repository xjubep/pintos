#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
//// user define start
#include <devices/shutdown.h>
#include <devices/input.h>
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include <string.h>
//// user define end

static void syscall_handler (struct intr_frame *);
//// user define start
typedef int pid_t;
void check_address(void *addr); 
void get_argument(void *sp, int *arg, int count);
void halt(void);
void exit(int status);
int wait(tid_t tid);
pid_t exec(const char *cmd_line);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
int fibonacci(int n);
int sum_of_four_int(int a, int b, int c, int d);
struct thread *get_child_process(int pid);
tid_t process_execute(const char *file_name);
int process_wait(tid_t child_tid);
//// user define end
//// user define start - prj2
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize(int fd);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close(int fd);

struct file {
	struct inode *inode;
	off_t pos;
	bool deny_write;
};
//// user define end - prj2
//
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	//// user define start
	void *sp = f->esp;
	int syscall_num = *(int *)sp;
	int arg[4];
	int count[22] = {
		0, 1, 1, 1, 2, 			// HALT, EXIT, EXEC, WAIT, CREATE
		1, 1, 1, 3, 3, 			// REMOVE, OPEN, FILESIZE, READ, WRITE
		2, 1, 1,						// SEEK, TELL, CLOSE
		-1, -1,							// MMAP, MUNMAP
		-1, -1, -1, -1, -1, // CHDIR, MKDIR, READDIR, ISDIR, INUMBER
		1, 4 };							// FIBO, SUM
		
	//printf("system_call: %d\n", syscall_num);

	check_address(sp);

	if (syscall_num != -1)
		get_argument(sp, arg, count[syscall_num]);
	switch (syscall_num) {
		case SYS_HALT: // 0
			halt();
			break;
		case SYS_EXIT: // 1
			exit(arg[0]);
			break;
		case SYS_EXEC: // 2
			f->eax = exec((const char *)arg[0]);
			break;
		case SYS_WAIT: // 3
			f->eax = wait(arg[0]);
			break;
		case SYS_READ: // 8
			f->eax = read(arg[0], (void *)arg[1], (unsigned)arg[2]);
			break;
		case SYS_WRITE: // 9
			f->eax = write(arg[0], (const void *)arg[1], (unsigned)arg[2]);
			break;
		case SYS_FIBONACCI: // 21
			fibonacci(arg[0]);
			break;
		case SYS_SUM_OF_FOUR_INT: // 22
			sum_of_four_int(arg[0], arg[1], arg[2], arg[3]);
			break;
		case SYS_CREATE: // 4
			f->eax = create((const char *)arg[0], (unsigned)arg[1]);
			break;
		case SYS_REMOVE: // 5
			f->eax = remove((const char *)arg[0]);
			break;
		case SYS_OPEN: // 6
			f->eax = open((const char *)arg[0]);
			break;
		case SYS_FILESIZE: // 7
			f->eax = filesize(arg[0]);
			break;
		case SYS_SEEK: // 10
			seek(arg[0], (unsigned)arg[1]);
			break;
		case SYS_TELL: // 11
			f->eax = (unsigned)tell(arg[0]);
			break;
		case SYS_CLOSE: // 12
			close(arg[0]);
			break;
		default:
			thread_exit ();
	}
	//// user define end
}

//// user define start
void check_address(void *addr) {
	int check_user_addr = 1; // 

	if (addr < (void *)0x08048000 || addr >= (void *)PHYS_BASE)
		check_user_addr = 0;
	if (check_user_addr == 0)
		exit(-1);
}

void get_argument(void *sp, int *arg, int count) {
	void *stack_ptr;
	int i;

	stack_ptr = sp + 4;
	for (i = 0; i < count; i++) {		
		check_address(stack_ptr);
		arg[i] = *(int *)stack_ptr;
		stack_ptr = stack_ptr + 4;
	}
	
}

void halt(void) {
	shutdown_power_off();
}

void exit(int status) {
	struct thread *t = thread_current();

	t->exit_status = status;
	printf("%s: exit(%d)\n", t->name, status);
	thread_exit();
}

pid_t exec(const char *cmd_line) {
	pid_t pid = process_execute(cmd_line);
	struct thread *child = get_child_process(pid);
	
	if (child == NULL)
		return -1;

	sema_down(&(child->load_semaphore));
	
	if (child->is_load == -1)
		return -1;
	else
		return pid;	
}

int wait (tid_t tid) {
	return process_wait(tid);
}

int read(int fd, void *buffer, unsigned size) {
	// fd: file descriptor (stdin = 0, stdout = 1)
	// off_t file_read (struct file *, void *, off_t)
	
	int i, len = 0;
	struct file *fp;
	struct thread *t = thread_current();
	
	check_address(buffer); // ??
	if (fd == 0) {
		for (i = 0; i < (int)size; i++) {
			((char *)buffer)[i] = input_getc();
			if (((char *)buffer)[i] == '\n' || ((char *)buffer)[i] == '\0') {
				((char *)buffer)[i] = '\0';
				break;
			}
			else {
				len++;
			}
		}
		return len;
	}
	else if (fd >= 3) {
		fp = t->fd[fd];
		return file_read(fp, buffer, size);
	}

	return -1;
}

int write(int fd, const void *buffer, unsigned size) {
	// fd: file descriptor (stdin = 0, stdout = 1)
	// off_t file_write (struct file *, const void *, off_t)
	
	struct file *fp;
	struct thread *t = thread_current();

	if (fd == 1) {
		putbuf(buffer, size);
		return size;
	}
	else if (fd >= 3) {
		fp = t->fd[fd];
		if (fp->deny_write) {
			file_deny_write(fp);
		}
		return file_write(fp, buffer, size);
	}

	return -1;
}

int fibonacci(int n) {
	int i, f = 0, g, h;

	if (n == 0 || n == 1)
		f = n;
	else {
		g = 0;
		h = 1;
		for (i = 2; i <= n; i++) {
			f = g + h;
			g = h;
			h = f;
		}
	}
	return f;
}

int sum_of_four_int(int a, int b, int c, int d) {
	int sum = 0;
	sum = a + b + c + d;
	return sum;
}

bool create (const char *file, unsigned initial_size) {
	// file name: file, size: initial_size (byte)인 파일 생성
	// 성공하면 true, 실패하면 false 리턴
	
	if (file == NULL)
		exit(-1);
	else
		return filesys_create(file, initial_size);
}

bool remove (const char *file) {
	// 파일 이름이 file인 파일 제거
	// 성공하면 true, 실패하면 false 리턴
	
	if (file == NULL)
		exit(-1);	
	else	
		return filesys_remove(file);
}

int open (const char *file) {
	// 파일 이름이 file인 파일을 열음
	// 성공하면 file descriptor (fd), 실패하면 -1 리턴
	
	struct file *fp;
	struct thread *t = thread_current();
	int i, fd = -1;

	if (file == NULL)
		exit(-1);

	fp = filesys_open(file);

	if (fp == NULL)
		return -1;
		
	for (i = 3; i < 128; i++) {
		if (t->fd[i] == NULL) {
			if (strcmp(t->name, file) == 0) {
				file_deny_write(fp);
			}
			t->fd[i] = fp;
			//file_deny_write(fp);
			fd = i;
			break;
		}
	}
		
	return fd;
}

int filesize (int fd) {
	// 파일 디스크립터가 fd인 파일의 사이즈 반환
	// 성공하면 파일 크기 (byte), 실패하면 -1 리턴
	
	int file_size = -1;
	struct file *fp;
	struct thread *t = thread_current();

	fp = t->fd[fd];
	file_size = file_length(fp);

	return file_size;
}

void seek (int fd, unsigned position) {
	// 열린 파일(fd)의 위치를 position으로 이동함
	// 원래는 EOF 지나서 write하는 거 에러 아니지만
	// 핀토스 플젝 4 전까지는 에러 처리
	
	struct file *fp;
	struct thread *t = thread_current();

	fp = t->fd[fd];
	file_seek(fp, position);
}

unsigned tell (int fd) {
	// 열린 파일(fd)의 위치를 알려줌
	// 성공하면 파일 위치, 실패하면 -1 리턴
	
	struct file *fp;
	struct thread *t = thread_current();

	fp = t->fd[fd];

	return file_tell(fp);
}

void close (int fd) {
	// 열린 파일 닫음
	// 파일 닫고 file descriptor 제거
	
	struct file *fp;
	struct thread *t = thread_current();

	fp = t->fd[fd];
	file_close(fp);
	t->fd[fd] = NULL;
}
//// user define end
