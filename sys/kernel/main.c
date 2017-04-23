/**
 * @file main.c
 * @author Sergio Johann Filho
 * @date January 2016
 * 
 * @section LICENSE
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file 'doc/license/gpl-2.0.txt' for more details.
 * 
 * @section DESCRIPTION
 * 
 * The HellfireOS realtime operating system kernel.
 * 
 */
 
#include <hal.h>
#include <libc.h>
#include <kprintf.h>
#include <malloc.h>
#include <queue.h>
#include <kernel.h>
#include <panic.h>
#include <scheduler.h>
#include <task.h>
#include <processor.h>
#include <main.h>
#include <ecodes.h>

void polling_server_sched(void);

static void print_config(void)
{
	kprintf("\n===========================================================");
	kprintf("\nHellfireOS %s [%s, %s]", KERN_VER, __DATE__, __TIME__);
	kprintf("\nEmbedded Systems Group - GSE, PUCRS - [2007 - 2017]");
	kprintf("\n===========================================================\n");
	kprintf("\narch:          %s", CPU_ARCH);
	kprintf("\nsys clk:       %d kHz", CPU_SPEED/1000);
	if (TIME_SLICE != 0)
		kprintf("\ntime slice:    %d us", TIME_SLICE);
	kprintf("\nheap size:     %d bytes", sizeof(krnl_heap));
	kprintf("\nmax tasks:     %d\n", MAX_TASKS);
}

static void clear_tcb(void)
{
	uint16_t i;
	
	for(i = 0; i < MAX_TASKS; i++){
		krnl_task = &krnl_tcb[i];
		krnl_task->id = -1;
		memset(krnl_task->name, 0, sizeof(krnl_task->name));
		krnl_task->state = TASK_IDLE;
		krnl_task->priority = 0;
		krnl_task->priority_rem = 0;
		krnl_task->delay = 0;
		krnl_task->rtjobs = 0;
		krnl_task->bgjobs = 0;
		krnl_task->apjobs = 0;
		krnl_task->deadline_misses = 0;
		krnl_task->period = 0;
		krnl_task->capacity = 0;
		krnl_task->deadline = 0;
		krnl_task->capacity_rem = 0;
		krnl_task->deadline_rem = 0;
		krnl_task->ptask = NULL;
		krnl_task->pstack = NULL;
		krnl_task->stack_size = 0;
		krnl_task->other_data = 0;
	}

	krnl_tasks = 0;
	krnl_current_task = 0;
	krnl_schedule = 0;
}

static void clear_pcb(void)
{
	krnl_pcb.sched_rt = sched_rma;
	krnl_pcb.sched_be = sched_priorityrr;
	krnl_pcb.sched_ap = sched_ap;
	krnl_pcb.coop_cswitch = 0;
	krnl_pcb.preempt_cswitch = 0;
	krnl_pcb.interrupts = 0;
	krnl_pcb.tick_time = 0;
}

static void init_queues(void)
{
	krnl_run_queue = hf_queue_create(MAX_TASKS);
	if (krnl_run_queue == NULL) panic(PANIC_OOM);
	krnl_delay_queue = hf_queue_create(MAX_TASKS);
	if (krnl_delay_queue == NULL) panic(PANIC_OOM);
	krnl_rt_queue = hf_queue_create(MAX_TASKS);
	if (krnl_rt_queue == NULL) panic(PANIC_OOM);
	//if (krnl_ap_queue == NULL) panic(PANIC_OOM);
	krnl_ap_queue = hf_queue_create(MAX_TASKS);
}

static void idletask(void)
{
	kprintf("\nKERNEL: free heap: %d bytes", krnl_free);
	kprintf("\nKERNEL: HellfireOS is running\n");

	hf_schedlock(0);
	
	for (;;){
		_cpu_idle();
	}
}

static void process_ap_queue(void)
{
	int32_t i, k;
	struct tcb_entry *krnl_task2;
	
	k = hf_queue_count(krnl_ap_queue);
	if(k > 0) 
		printf("\n total AP tasks = (%d)", k);
	for (i = 0; i < k; i++){
		krnl_task2 = hf_queue_remhead(krnl_ap_queue);
		//if (!krnl_task2) panic(PANIC_NO_TASKS_DELAY);
		if (krnl_task2->capacity_rem > 0){
			if (hf_queue_addtail(krnl_ap_queue, krnl_task2)) panic(PANIC_CANT_PLACE_AP);						
		}else {
			hf_kill(krnl_task2->id);
		}
	}
}

void polling_server(void){
	for(;;){
		polling_server_sched();
	}
}

void polling_server_sched(void){
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
	if ((krnl_tasks > 0) && (krnl_ap_tasks > 0)){
		process_ap_queue();
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
/**
 * @internal
 * @brief HellfireOS kernel entry point and system initialization.
 * 
 * @return should not return.
 * 
 * We assume that the following machine state has been already set
 * before this routine.
 *	- Kernel BSS section is filled with 0.
 *	- Kernel stack is configured.
 *	- All interrupts are disabled.
 *	- Minimum page table is set. (MMU systems only)
 */
int main(void)
{
	static uint32_t oops=0xbaadd00d;
	
	_hardware_init();
	hf_schedlock(1);
	_di();
	kprintf("\nKERNEL: booting...");
	if (oops == 0xbaadd00d){
		oops = 0;
		print_config();
		_vm_init();
		clear_tcb();
		clear_pcb();
		init_queues();
		_sched_init();
		_irq_init();
		_timer_init();
		_timer_reset();
		hf_spawn(idletask, 0, 0, 0, "idle task", 1024);
		_device_init();
		_task_init();
		hf_spawn(polling_server, 5,2, 5, "polling server", 1024);
		app_main();
		_restoreexec(krnl_task->task_context, 1, krnl_current_task);
		panic(PANIC_ABORTED);
	}else{
		panic(PANIC_GPF);
	}
	
	return 0;
}
