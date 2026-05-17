#ifndef _DMESG_H_
#define _DMESG_H_

#define LOG_SYSCALL  (1<<0)
#define LOG_INTR     (1<<1)
#define LOG_PROC     (1<<2)
#define LOG_EXEC     (1<<3)

#define LOG_ALL  (LOG_SYSCALL | LOG_INTR | LOG_PROC | LOG_EXEC)
#define LOG_NONE 0

#endif
