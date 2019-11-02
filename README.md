# pintos

File to be modified
- src/userprog: process.h / <B>process.c</B>,   syscall.h / <B>syscall.c</B>
- src/threads: <B>thread.h</B> / thread.c 
- src/lib: <B>syscall-nr.h,   user/syscall.h,   user/syscall.c</B>
  
Referenced files
- src/userprog: <B>pagedir.h / pagedir.c</B>,   exception.h / exception.c
- src/threads: synch.h / synch.c,   vaddr.h
- src/devices: shutdown.h / shutdown.c,   input.h / input.c

<pre>
[Functionality] (9/13)
  1. args-none: pass
  2. args-single: pass
  3. args-multiple: pass
    4. args-many: fail
  5. args-dbl-space: pass
  6. exec-once: pass
  7. exec-multiple: pass
    8. exec-arg: fail
    9. wait-simple: fail
    10. wait-twice: fail
    11. multi-recurese: fail
  12. exit: pass
  13. halt: pass

[Robustness] (4/8)
    1. exec-bad-ptr: fail
  2. exec-missing: pass	
    3. sc-bad-arg: fail
    4. sc-bad-sp: fail
  5. sc-boundary: pass
  6. sc-boundary-2: pass
  7. wait-bad-pid: pass
    8. wait-killed: fail
</pre>
