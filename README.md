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
[Functionality] (12/13) (branch 1)
  1. args-none: pass
  2. args-single: pass
  3. args-multiple: pass
  4. args-many: pass
  5. args-dbl-space: pass
  6. exec-once: pass
  7. exec-multiple: pass
    8. exec-arg: fail
  9. wait-simple: pass
  10. wait-twice: pass
  11. multi-recurese: pass
  12. exit: pass
  13. halt: pass

[Robustness] (8/8) (brnach 1)
  1. exec-bad-ptr: pass
  2. exec-missing: pass	
  3. sc-bad-arg: pass
  4. sc-bad-sp: pass
  5. sc-boundary: pass
  6. sc-boundary-2: pass
  7. wait-bad-pid: pass
  8. wait-killed: pass
</pre>

/// 하나만 검사하는 명령어 (pintos/src/userprog/build 에서 수행)
pintos -v -k -T 60 --qemu --filesys-size=2 -p tests/userprog/sc-bad-sp -a sc-bad-sp -- -q -f run sc-bad-sp

