#include <hellfire.h>


static void ap_queue_next(){
	krnl_task = hf_queue_remhead(krnl_ap_queue);
	if (!krnl_task)
		panic(PANIC_NO_TASKS_AP);
	if (hf_queue_addtail(krnl_ap_queue, krnl_task))
		panic(PANIC_CANT_PLACE_AP);
	
}

int32_t sched_ap(void){
	if (hf_queue_count(krnl_ap_queue) == 0)
		panic(PANIC_NO_TASKS_AP);
	do {
		ap_queue_next();
	} while (krnl_task->state == TASK_BLOCKED);
	krnl_task->bgjobs++;

	return krnl_task->id;
}

void polling_server(void){
	printf("Executou polling server");
}


void app_main(void){
	hf_spawn(polling_server,3,1,3,"polling_server", 2048);
}

