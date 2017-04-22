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
                printf("executando tarefa aperiodica\n");
}

void tasks_periodics(void){
	hf_spawn(task, 4, 1, 4, "task 1", 2048);
	hf_spawn(task, 8, 2, 8, "task 2", 2048);
	hf_spawn(task, 12, 3, 12, "task 3", 2048);
}

void generate_ap_tasks(void){
	for(;;){
		int32_t r = (random()%140)+60;
		delay_ms(r);
		hf_spawn(task2, 0, 1, 0, "aperidic", 2048);
	}	
}
void app_main(void){
	hf_spawn(generate_ap_tasks, 0, 0, 0, "generate", 2048);
	tasks_periodics();
}

