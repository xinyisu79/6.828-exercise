# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save current register context in old
# and then load register context from new.

.globl swtch
swtch:
  movl 4(%esp), %eax
  movl 8(%esp), %edx

  # Save old callee-save registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

  # Switch stacks
  movl %esp, (%eax)
  movl %edx, %esp

#注意，上面pushl的是cpu->scheduler的context
#这儿esp已经变化了(movl %esp, (%eax)),
#所以加载的是proc自己的context

  # Load new callee-save registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
#跳转到放在栈上的return addr处执行
#我猜：进程切换，当前进程的执行的epi应该被scheduler记住，
#以便下次该进程被schedule的时候，此处的ret还能奏效
  ret
