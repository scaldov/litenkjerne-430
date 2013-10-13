name liten_kjerne

public krn_context_switch
public krn_context_load
public krn_uthread_idle
public krn_enter_thread

rseg CODE

krn_uthread_idle:
jmp krn_uthread_idle

krn_enter_thread:
pop r13
push r12
ret

krn_context_switch:
;r12 - current thread context, r13 - new context
push r4
push r5
push r6
push r7
push r8
push r9
push r10
push r11
mov sp, r6
mov r6, 0(r12)
mov r13, r12
krn_context_load:
;r12 - thread context
mov 0(r12), r6
mov r6, sp
pop r11
pop r10
pop r9
pop r8
pop r7
pop r6
pop r5
pop r4
ret

end
