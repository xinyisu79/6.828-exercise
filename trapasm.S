#include "mmu.h"

  # vectors.S sends all traps here.
.globl alltraps
#顾名思义，所有的trap(包括中断，系统调用，exception）都会进入这部分的代码。
#然后调用trap(), trap中根据不同的类型，SYS_CALL, 还是EXCEPTION等，进行不同的处理：
#SYS_CALL的话，就调用sys_call. 异常就调用相应的exception handler.

alltraps:
  # Build trap frame.
  pushl %ds
  pushl %es
  pushl %fs
  pushl %gs
  pushal
  
  # Set up data and per-cpu segments.
  # 将es, fs, gs设置为kernel mode下的segment selector
  movw $(SEG_KDATA<<3), %ax
  movw %ax, %ds
  movw %ax, %es
  movw $(SEG_KCPU<<3), %ax
  movw %ax, %fs
  movw %ax, %gs

  # Call trap(tf), where tf=%esp
  pushl %esp
  call trap
  addl $4, %esp

  # Return falls through to trapret...
.globl trapret
trapret:
  popal
  popl %gs
  popl %fs
  popl %es
  popl %ds
  addl $0x8, %esp  # trapno and errcode
  iret
