#ifndef SYS_SCHED_H
#define SYS_SCHED_H

struct thread;

void sched_init(void);
void sched_add(struct thread *thread);
void sched_rm(struct thread *thread);
void sched_run(struct thread *thread);
void sched_tick(void);
void sched_switch(struct thread *thread);

#endif
