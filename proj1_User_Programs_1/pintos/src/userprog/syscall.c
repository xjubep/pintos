#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
//// user define start
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
//// user define end

static void syscall_handler (struct intr_frame *);

//// user define start
typedef int pid_t;
void check_address(void *addr);
void get_argument(void *esp, int *arg, int count);
void halt(void);
void exit(int status);
pid_t exec(const char *cmd_line);
int wait(pid_t pid);
int read(int fd, void *buffer, unsigned size);
int write(int fd, const void *buffer, unsigned size);
int fibonacci(int num);
int sum_of_four_int(int a, int b, int c, int d);
//// user define end

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

//// user define start
void check_address(void *addr) {
	// 포인터가 가리키는 주소가 유저 영역의 주소인지 확인
	// 잘못된 접근일 경우 프로세스 종료
	// pagedir_get_page()는 주어진 user virtual address에 매핑 되는
	// kernel virtual address를 리턴, 매핑 x면 NULL 리턴
	// is_user_vaddr()와 is_kernel_vaddr()는 각각 유저/커널 가상주소 확인
	void *page = pagedir_get_page(thread_current()->pagedir, addr);

	if (!(page != NULL && is_user_vaddr(addr)))
		exit(-1);
}

void get_argument(void *esp, int *arg, int count) {
	// 유저 스택에 있는 인자들을 커널에 저장하는 함수
	// 스택 포인터(esp)에 count(인자의 개수) 만큼의 데이터를 arg에 저장
	void *stack_ptr;
	int i;

	stack_ptr = esp + 4;
	for (i = 0; i < count; i++) {
		check_address(stack_ptr);
		arg[i] = *(int *)stack_ptr;
		stack_ptr += 4;
	}
}

void halt(void) {
	// therminates pintos by calling shutdown_power_off()
	shutdown_power_off();
}

void exit(int status) {
	// 실행중인  스레드 구조체를 가져옴
	// 프로세스 종료 메시지 출력, "프로세스 이름: exit(종료상태)"
	// 스레드 종료
	// terminates the current user program, returning status to the kernel
	struct thread *cur = thread_current();
	//status = cur->status;
	printf("%s: exit(%d)\n", cur->name, status);
	//printf("%s: exit(%d)\n", thread_name(), status);
	thread_exit();
}

pid_t exec(const char *cmd_line) {
	// create child process
	// refer to process_execute() in userprog/process.c
	// success: return pid // fail: return -1
/*
	if ( ) {
		return 
	}
*/	
	// 자식 프로세스를 생성하고 프로그램을 실행시키는 시스템 콜
	// 프로세스 생성에 성공 시 생성된 프로세스에 pid 값을 반환, 실패 시 -1 반환
	// 부모 프로세스는 생성된 자식 프로세스의 프로그램이 메모리에에 적재될 때까지 대기
	pid_t pid = process_execute(cmd_line);
	struct thread *child = get_child_process(pid);

	sema_down(&(child->load_semaphore));

	return (child->is_load_success == 1)? pid: -1;
	//return process_execute(cmd_line);
	//return -1;
}

int wait(pid_t pid) {
	// what wait() system call should do is wait child process
	// until it finishes its work
	// check child thread ID is valid
	// get the exit status from child thread when the child thread is dead
	// to prevent termination of process bofre return from wait(),
	// you can use busy waiting technique or thread_yieled()
	// 자식 프로세스가 종료 될 때까지 대기
	// process_wait 사용

	return process_wait(pid);
}	

int read(int fd, void *buffer, unsigned size) {
	// fd: file descriptor (stdin = 0, stdou = 1)
	int i, len = 0;

	if (fd == 0) {
		for (i = 0; i < (int)size; i++) {
			//printf("%c\n", ((char *)buffer)[i]);
			((char *)buffer)[i] = input_getc();
			if (((char *)buffer)[i] == '\n' || ((char *)buffer)[i] == '\0') {
				((char *)buffer)[i] = '\0';
				break;
			}
			else
				len++;
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

int fibonacci(int num) {
	int i, f = 0, g, h;

	if (num == 0 || num == 1) {
		f = num;
	}
	else {
		g = 0;
		h = 1;
		for (i = 2; i <= num; i++) {
			f = g + h;
			g = h;
			h = f;
		}
	}	

	return f;
}

int sum_of_four_int (int a, int b, int c, int d) {
	int sum = 0;
	sum = a + b + c + d;
	return sum;
}
/// user define end


static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  //printf ("system call!\n");

	//// user define start
	int arg[4];
	void *esp = f->esp;
	int syscall_num = *(int*)f->esp;
	struct thread *cur = thread_current();
	
	//printf("system call number: %d\n", syscall_num);
	check_address(esp);
	//hex_dump(esp, esp, 100, true);
	switch (syscall_num) {
		case SYS_HALT://
			halt();
			break;
		case SYS_EXIT://
			get_argument(esp, arg, 1);
			exit(arg[0]);
			break;
		case SYS_EXEC://
			get_argument(esp, arg, 1);
			f->eax = exec((const char *)arg[0]);
			break;
		case SYS_WAIT://
			get_argument(esp, arg, 1);
			f->eax = wait(arg[0]);
			break;
		case SYS_CREATE:
			break;
		case SYS_REMOVE:
			break;
		case SYS_OPEN:
			break;
		case SYS_FILESIZE:
			break;
		case SYS_READ://
			get_argument(esp, arg, 3);
			f->eax= read(arg[0], (void *)arg[1], (unsigned)arg[2]);
			break;
		case SYS_WRITE://
			get_argument(esp, arg, 3);
			f->eax = write(arg[0], (const void *)arg[1], (unsigned)arg[2]);
			break;
		case SYS_SEEK:
			break;
		case SYS_TELL:
			break;
		case SYS_CLOSE:
			break;
		case SYS_FIBONACCI:
			get_argument(esp, arg, 1);
			f->eax = fibonacci(arg[0]);
			break;
		case SYS_SUM_OF_FOUR_INT:
			get_argument(esp, arg, 4);
			f->eax = sum_of_four_int(arg[0], arg[1], arg[2], arg[3]);
			break;
		default:
			break;
	}
	//// user define end
  //thread_exit ();
}
