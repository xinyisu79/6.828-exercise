# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save current register context in old
# and then load register context from new.

.globl swtch
swtch:

#注意这儿没有push ebp, esp，所以4(esp)是参数old，8(esp)是new
  movl 4(%esp), %eax
  movl 8(%esp), %edx


#然后将当前的x86 convention中被调函数需要保存的寄存器保存起来
#注意到：这四个寄存器，连着栈下面的returning address. (sched() 调用swtch()时候
#, 即call指令，push上去的。这么多一起，构成了当前 - old context，的状态。所以，
#直接把old context指针指向这儿就行了！
  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
#这是把当前的esp指向的位置，赋值给old context.
  movl %esp, (%eax)
#然后esp指向需要转向的new context保存的栈顶
  movl %edx, %esp


#好，现在是esp指向已经被switch 到new process的context了
#由于这儿的栈也是用类似于上述代码构造的，所以一轮pop之后，就恢复了
#new proces的context寄存器，并且，栈地包含的是new process接下来要执行的
#指令的地址。ret之后，eip就会被赋值为这个地址！
# Load new callee-save registers
  # Load new callee-save registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret
