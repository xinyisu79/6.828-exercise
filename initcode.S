# Initial process execs /init.

#include "syscall.h"
#include "traps.h"


# exec(init, argv)

#第一个进程初始化的时候，用initnode.S的代码作为执行代码
#但这个又是/init程序的bootstrap, 下面的代码用exec系统调用
#执行"/init" 程序
.globl start
start:
  pushl $argv
  pushl $init
  pushl $0  // where caller pc would be
  movl $SYS_exec, %eax
  int $T_SYSCALL

# for(;;) exit();
exit:
  movl $SYS_exit, %eax
  int $T_SYSCALL
  jmp exit

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

