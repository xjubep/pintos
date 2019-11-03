#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
//// user define start
#include <devices/shutdown.h>
#include <devices/input.h>
#include "threads/vaddr.h"

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

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf ("system call!\n");
	//// user define start
	/* user stack에 저장되어 있는 syscall_num을 이용해 handler 구현 */
	void *sp = f->esp;
	int syscall_num = *(int *)sp;
	int arg[4], count = 0;

	/* stack ptr이 user 영역인지 확인 */
	check_address(sp);

	/* 저장된 인자 값이 포인터일 경우 유저 영역의 주소인지 확인 */
	//printf("%d\n", syscall_num);
	switch (syscall_num) {
		case SYS_HALT: // 0
			halt();
			break;
		case SYS_EXIT: // 1
			count = 1;
			get_argument(sp, arg, count);
			exit(arg[0]);
			break;
		case SYS_EXEC: // 2
			count = 1;
			get_argument(sp, arg, count);
			//check_address((void *)arg[0]);
			f->eax = exec((const char *)arg[0]);
			break;
		case SYS_WAIT: // 3
			count = 1;
			get_argument(sp, arg, count);

			f->eax = wait(arg[0]);
			break;
		case SYS_READ: // 8
			count = 3;
			get_argument(sp, arg, count);
			f->eax = read(arg[0], (void *)arg[1], (unsigned)arg[2]);
			break;
		case SYS_WRITE: // 9
			count = 3;
			get_argument(sp, arg, count);
			f->eax = write(arg[0], (const void *)arg[1], (unsigned)arg[2]);
			break;
		case SYS_FIBONACCI:
			count = 1;
			get_argument(sp, arg, count);
			fibonacci(arg[0]);
			break;
		case SYS_SUM_OF_FOUR_INT:
			count = 4;
			get_argument(sp, arg, count);
			sum_of_four_int(arg[0], arg[1], arg[2], arg[3]);
			break;
		default:
			thread_exit ();
	}
	//// user define end
}

//// user define start
void check_address(void *addr) {
	// 주소 유효성 검사: 포인터가 가리키는 주소가 사용자 영역
	// (0x08048000 ~ 0xc0000000)인지 확인
	// 유저 영역을 벗어난 영역일 경우 프로세스 종료 (exit(-1))

	int check_user_addr = 1; // 

	/* 포인터가 가리키는 주소가 유저영역의 주소인지 확인 */
	//check_user_addr = is_user_vaddr(addr);
	if (addr < 0x08048000 || addr >= PHYS_BASE)
		check_user_addr = 0;

	/* 잘못된 접근일 경우 프로세스 종료 */
	if (check_user_addr == 0)
		exit(-1);
}

void get_argument(void *sp, int *arg, int count) {
	// 유저 스택에 있는 인자들을 커널에 저장
	// 스택에서 인자들을 4byte 크기로 꺼내어 arg배열에 순차적으로 저장
	// count 개수 만큼의 인자를 스택에서 가져옴

	void *stack_ptr;
	int i;

	/* 유저 스택에 저장된 인자값들을 커널로 저장 */
	stack_ptr = sp + 4;
	for (i = 0; i < count; i++) {
		
	/* 인자가 저장된 위치가 유저영역인지 확인 */
		check_address(stack_ptr);
		arg[i] = *(int *)stack_ptr;
		stack_ptr = stack_ptr + 4;
	}
	
}

void halt(void) {
	/* shutdown_power_off()를 사용하여 pintos 종료 */
	shutdown_power_off();
}

void exit(int status) {
	/* 실행중인 스레드 구조체를 가져옴 */
	struct thread *t = thread_current();

	// 프로그램 종료 시 exit() 시스템 콜을 호출
	// 정상적으로 종료가 됐는지 확인하기 위해 exit status 저장
	/* 프로세스 디스크립터에 exit status 저장 */
	t->exit_status = status;

	/* 프로세스 종료 메시지 출력,
		출력 양식: "프로세스 이름: exit(종료상태" */
	printf("%s: exit(%d)\n", t->name, status);

	/* 스레드 종료 */
	thread_exit();
}

pid_t exec(const char *cmd_line) {
	// 자식 프로세스를 생성하고 프로그램을 실행시키는 시스템 콜
	// 프로세스 생성에 성공 시 생성된 프로세스에 pid값을 반환, 실패 시 -1 반환
	// 부모 프로세스는 생성된 자식 프로세스의 프로그램이 메모리에 적재 될 때가지 대기 (세마포어)

	/* process_execute() 함수를 호출하여 자식 프로세스 생성 */
	pid_t pid = process_execute(cmd_line);

	/* 생성된 자식 프로세스의 프로세스 디스크립터를 검색 */
	struct thread *child = get_child_process(pid);
	
	/* 자식 프로세스의 프로그램이 적재될 때까지 대기 */
	sema_down(&(child->load_semaphore));

	/* 프로그램 적재 실패 시 -1 리턴 */
	if (child->is_load == -1)
		return -1;

	/* 프로그램 적재 성공 시 자식 프로세스의 pid 리턴 */
	else
		return pid;	
}

int wait (tid_t tid) {
	/* 자식 프로세스가 종료 될 때까지 대기 */
	/* process_wait() 사용 */
	return process_wait(tid);
}

int read(int fd, void *buffer, unsigned size) {
	// fd: file descriptor (stdin = 0, stdout = 1)
	int i, len = 0;

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
	return -1;
}

int write(int fd, const void *buffer, unsigned size) {
	// fd: file descriptor (stdin = 0, stdout = 1)
	if (fd == 1) {
		putbuf(buffer, size);
		return size;
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
//// userdefine end
