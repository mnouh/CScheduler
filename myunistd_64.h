#ifndef _ASM_X86_UNISTD_64_H
#define _ASM_X86_UNISTD_64_H

#define __NR_start_pid_record			300
__SYSCALL(__NR_start_pid_record, sys_start_pid_record)
#define __NR_stop_pid_record			301
__SYSCALL(__NR_stop_pid_record, sys_stop_pid_record)


