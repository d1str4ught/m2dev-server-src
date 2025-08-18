#ifndef __INC_METIN_II_TELLWAIT_H__
#define __INC_METIN_II_TELLWAIT_H__

#ifndef OS_WINDOWS

extern void TELL_WAIT();

extern void WAIT_CHILD();
extern void TELL_CHILD(pid_t pid);

extern void WAIT_PARENT();
extern void TELL_PARENT(pid_t pid);

#endif

#endif
