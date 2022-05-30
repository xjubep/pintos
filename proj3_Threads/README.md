# Project 3 Threads

* Process Scheduling, Threads, Synchronization
  * Pintos uses <b>round robin scheduler</b> as default scheduler.
  * <b>It means Pintos doesn't consider the priority of each process or thread</b>.
  * Implement complex scheduler which considers thread's priority
  * Synchronization such as semaphores or locks should be used in the scheduler to arrange order of thread execution.
* Alarm Clock
* Priority Scheduling
* Advanced Scheduler (BSD Scheduler)
  * Additional
-----
## Evaluation
1. alarm-single `pass`
2. alarm-multiple `pass`
3. alarm-simultaneous `pass`
4. alarm-priority `pass`
5. alarm-zero `pass`
6. alarm-negative `pass`
7. priority-change `pass`
8. priority-change-2 `pass`
9. priority-fifo `pass`
10. priority-lifo `pass` // pintos -v -- -q run priority-lifo (can't be checked by make check)
11. priority-preemp `pass`
12. priority-sema `pass`
13. priority-aging `pass`
## Additional Requirement - BSD Scheduler (5%)
1. mlfqs-block `FAIL`
2. mlfqs-fair-2 `FAIL`
3. mlfqs-fair-20 `FAIL`
4. mlfqs-load-1 `FAIL`
5. mlfqs-load-60 `FAIL`
6. mlfqs-load-avg `FAIL`
7. mlfqs-nice-10 `FAIL`
8. mlfqs-nice-2 `FAIL`
9. mlfqs-recent-1 `FAIL`
