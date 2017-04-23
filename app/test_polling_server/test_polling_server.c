#include <hellfire.h>
	
void task(void){
	int32_t jobs, id;	
	id = hf_selfid();
	for(;;){
		jobs = hf_jobs(id);
		printf("\n%s (%d)[%d][%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
		while (jobs == hf_jobs(id));
	}
}

void task2(void){
	int32_t jobs, id;	
	id = hf_selfid();
	for(;;){
		jobs = hf_jobs(id);
		printf("\n%s (%d)[%d][%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
		while (jobs == hf_jobs(id));
	}
}

void task_generator(void){
	int32_t jobs, id;	
	for(;;){
		int32_t r = (random() % 140) + 60;
		printf("\n%s (%d)[%d][%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
		delay_ms(r);
		hf_spawn(task2, 0, 3, 0, "aperiodic", 2048);
	}	
}
void app_main(void){
	hf_spawn(task, 4, 1, 4, "task 1", 2048);
	//hf_spawn(task, 8, 2, 8, "task 2", 2048);
	// hf_spawn(task, 12, 3, 12, "task 3", 2048);
	hf_spawn(task_generator, 0, 0, 0, "task generator", 2048);
}

