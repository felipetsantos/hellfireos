void polling_server(void){
	int32_t rc;
	volatile int32_t status;

	status = _di();
#if KERNEL_LOG >= 1
		dprintf("polling_server() %d ", (uint32_t)_read_us());
#endif	
	krnl_task = &krnl_tcb[krnl_current_task];
	rc = setjmp(krnl_task->task_context);
	if (rc){
		_ei(status);
		return;
	}
	if (krnl_task->state == TASK_RUNNING)
		krnl_task->state = TASK_READY;
	if (krnl_task->pstack[0] != STACK_MAGIC)
		panic(PANIC_STACK_OVERFLOW);
	if (krnl_tasks > 0){
		krnl_current_task = krnl_pcb.sched_ap();
		krnl_task->state = TASK_RUNNING;
		krnl_pcb.coop_cswitch++;
#if KERNEL_LOG >= 1
		dprintf("\n%d %d %d %d %d ", krnl_current_task, krnl_task->period, krnl_task->capacity, krnl_task->deadline, (uint32_t)_read_us());
#endif
		_restoreexec(krnl_task->task_context, status, krnl_current_task);
		panic(PANIC_UNKNOWN);
	}else{
		panic(PANIC_NO_TASKS_LEFT);
	}
}
