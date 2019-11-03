#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);
//// user define start
struct thread *get_child_process(int pid);
void remove_child_process(struct thread *cp);
//// user define end

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{
  char *fn_copy;
  tid_t tid;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;
  strlcpy (fn_copy, file_name, PGSIZE);

  /* Create a new thread to execute FILE_NAME. */
  //tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
  //// user define start
	char *token, *save_ptr;
	token = strtok_r((char *)file_name, " ", &save_ptr);
	tid = thread_create (file_name, PRI_DEFAULT, start_process, fn_copy);
	//// user define end
	if (tid == TID_ERROR)
    palloc_free_page (fn_copy); 
  return tid;
}

/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *file_name_)
{
  char *file_name = file_name_;
  struct intr_frame if_;
  bool success;

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);

	//// user define start
	// 메모리 적재 성공 시 유저 프로그램 실행, 실패 시 스레드 종료

	/* 메모리 적재 완료 시 부모 프로세스 다시 진행 (세마포어) */
	struct thread *t = thread_current();
	sema_up(&(t->load_semaphore));

	/* If load failed, quit. */
	palloc_free_page (file_name);
	if (!success) {
		/* 메모리 적재 실패 시 프로세스 디스크립터에 메모리 적재 실패 */
		t->is_load = -1;
		thread_exit();
	}
	else {
		/* 메모리 적재 성공 시 프로세스 디스크립터에 메모리 적재 성공 */
		t->is_load = 1;
	}
	//// user define end

  /* If load failed, quit. */
//  palloc_free_page (file_name);
//  if (!success) 
//    thread_exit ();

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid UNUSED) 
{
	//// user define start
	//while(1);
	// 자식 프로세스가 수행 되고 종료될 때 까지 부모 프로세스 대기
	// 프로세스의 정보를 알기 위해 프로세스 디스크립터 검색
	// 자식 프로세스가 종료되지 않았으면 부모 프로세스 대기
	// 유저 프로세스가 정상적으로 종료 시 exit status 반환, 아닐 시 -1 반환
	
	/* 자식 프로세스의 프로세스 디스크립터 검색 */
	struct thread *child = get_child_process(child_tid);
	int exit_status = child->exit_status;

	/* 예외 처리 발생시 -1 리턴 */
	if (child == NULL)
		return -1;
	
	/* 자식 프로세스가 종료될 때까지 부모 프로세스 대기 (세마포어) */
	sema_down(&(child->exit_semaphore));

	/* 자식 프로세스 디스크립터 삭제 */
	remove_child_process(child);
	
	/* 자식 프로세스의 exit status 리턴 */
	return exit_status;

	//// user define end
  return -1;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
//// user define start
#define	MAX_ARG	30
#define	MAX_LEN	30
//// user define end
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofset;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

	//// user define start
	/* parse file name */
	char argv[MAX_ARG][MAX_LEN];
	char *token, *save_ptr;
	int arg_idx, argc = 0;

	for (token = strtok_r((char *)file_name, "' '\n", &save_ptr);
		token != NULL; token = strtok_r(NULL, "' '\n", &save_ptr)) {
		arg_idx = argc;
		strlcpy(argv[arg_idx], token, MAX_LEN);
		argc++;
	}
	//printf("%s\n", file_name);
	//// user define end

  /* Open executable file. */
  file = filesys_open (file_name);
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }

  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofset = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofset < 0 || file_ofset > file_length (file))
        goto done;
      file_seek (file, file_ofset);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofset += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp))
    goto done;
	//// user define start
	/* construct stack */
	void *argv_addr[MAX_ARG], *tmp_esp = *esp;
	int j, len, total_len = 0;

	// push argv string (argv[i][j]), char
	for (i = argc -1; i > -1; i--) {
		len = strlen(argv[i]);
		total_len += (len + 1);
		for (j = len; j > -1; j--) {
			*esp = *esp - 1;
			**(char **)esp = argv[i][j];
		}
		argv_addr[i] = (uint32_t *)*esp;
	}

	// word align, uint8_t
	if (total_len % 4 != 0)
		*esp = *esp - (4 - (total_len % 4));
	
	// push NULL (argv[argc]), char *
	*esp = *esp - 4;
	**(uint32_t **)esp = 0;
	
	// push argv address (argv[i]), char *
	for (i = argc - 1; i > -1; i--) {
		*esp = *esp - 4;
		**(uint32_t **)esp = argv_addr[i];
	}
	//

	// push argv, char **
	*esp = *esp - 4;
	**(uint32_t **)esp = *esp + 4;

	// push argc, int
	*esp = *esp - 4;
	**(uint32_t**)esp = argc;

	// push return address, void *
	*esp = *esp - 4;
	**(uint32_t**)esp = 0;

	//hex_dump(*esp, *esp, tmp_esp - *esp, true);
	//// user define end

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
  file_close (file);
  return success;
}

/* load() helpers. */

static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *knpage = palloc_get_page (PAL_USER);
      if (knpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, knpage, page_read_bytes) != (int) page_read_bytes)
        {
          palloc_free_page (knpage);
          return false; 
        }
      memset (knpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, knpage, writable)) 
        {
          palloc_free_page (knpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp) 
{
  uint8_t *kpage;
  bool success = false;

  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
  if (kpage != NULL) 
    {
      success = install_page (((uint8_t *) PHYS_BASE) - PGSIZE, kpage, true);
      if (success)
        *esp = PHYS_BASE;
      else
        palloc_free_page (kpage);
    }
  return success;
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
static bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *th = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (th->pagedir, upage) == NULL
          && pagedir_set_page (th->pagedir, upage, kpage, writable));
}

//// user define start
struct thread *get_child_process(int pid) {
	// 자식 리스트를 pid로 검색하여 해당 프로세스 디스크립터를 반환
	// pid가 없을 경우 NULL 반환

	struct list_elem *e;
	struct thread *tmp, *p = thread_current();

	if (p == NULL)
		return NULL;

	/* 자식 리스트에 접근하여 프로세스 디스크립터 검색 */
	for (e = list_begin(&(p->child_list)); e != list_end(&(p->child_list));
		e = list_next(e)) {
			tmp = list_entry(e, struct thread, child_elem);

			/* 해당 pid가 존재하면 프로세스 디스크립터 반환 */
			if (tmp->tid == pid) {
				return tmp;
			}
	}

	/* 리스트에 존재하지 않으면 NULL 리턴 */
	return NULL;
}

void remove_child_process(struct thread *cp) {
	// 부모 프로세스의 자식 리스트에서 프로세스 디스크립터 제거
	// 프로세스 디스크립터 메모리 해제

	if (cp != NULL) {
		/* 자식 리스트에서 제거 */
		// 다시 보기
		struct list_elem *e;
		struct thread *tmp, *p = thread_current();

		for (e = list_begin(&(p->child_list)); e != list_end(&(p->child_list));
			e = list_next(e)) {
				tmp = list_entry(e, struct thread, child_elem);
				if (tmp->tid == cp->tid) {
					list_remove(&(tmp->child_elem));
				}
		}


		/* 프로세스 디스크립터 메모리 해제 */
		palloc_free_page(cp);
	}
}
//// user define end
