#include <hellfire.h>
#include <noc.h>

// t1, t2, t4, t6, t8, t9

void gen_rdm_bytestream(size_t size, int8_t *buf)
{
  buf = malloc (size);
  size_t i;

  for(i = 0; i < size; i++)
  {
		buf[i] = random();
  }
}

void t1(void) {
	int8_t *buf2, *buf3, *buf4, *buf5, *buf7;
	uint16_t val2, val3, val4, val5, val7;

	printf("\nCPU %d, executando tarefa 1.", hf_cpuid());

	if (hf_comm_create(hf_selfid(), 5001, 0))
		panic(0xff);

	while(1) {
		gen_rdm_bytestream (256, buf2);
		val2 = hf_sendack(4, 5002, buf2, 256, 0, 500);
		free(buf2);

		gen_rdm_bytestream (64, buf4);
		val4 = hf_sendack(3, 5004, buf4, 64, 0, 500);
		free(buf4);

		gen_rdm_bytestream (64, buf5);
		val5 = hf_sendack(3, 5005, buf5, 64, 0, 500);
		free(buf5);

		gen_rdm_bytestream (64, buf3);
		val3 = hf_sendack(4, 5003, buf3, 64, 0, 500);
		free(buf3);

		gen_rdm_bytestream (1280, buf7);
		val7 = hf_sendack(2, 5007, buf7, 1280, 0, 500);
		free(buf7);

		delay_ms(10);
	}
}

void t2(void){
	int8_t *buf1 = malloc (256);
	int8_t *buf6, *buf7, *buf8;
	uint16_t val, val6, val7, val8;
	uint16_t cpu1, size1, task1;
	
	if (hf_comm_create(hf_selfid(), 5002, 0))
		panic(0xff);

	while(1) {

		val = hf_recvack(&cpu1, &task1, buf1, &size1, 0);
		printf("\nCPU %d, executando tarefa 2.", hf_cpuid());
		printf("\ntarefa 2, CPU %d recebeu buffer da tarefa 1 com aresta %d\n", hf_cpuid(), size1);

		gen_rdm_bytestream (64, buf6);
		val6 = hf_sendack(3, 5006, buf6, 64, 0, 500);
		free(buf6);

		gen_rdm_bytestream (320, buf7);
		val7 = hf_sendack(2, 5007, buf7, 320, 0, 500);
		free(buf7);

		gen_rdm_bytestream (320, buf8);
		val8 = hf_sendack(1, 5008, buf8, 320, 0, 500);
		free(buf8);

		delay_ms(10);
	}

}

void t3(void){
	int8_t *buf1 = malloc (64);
	int8_t *buf7, *buf8;
	uint16_t val1, val7, val8;
	uint16_t cpu1, size1, task1;
	
	if (hf_comm_create(hf_selfid(), 5003, 0))
		panic(0xff);

	while(1) {

		val1 = hf_recvack(&cpu1, &task1, buf1, &size1, 0);
		printf("\nCPU %d, executando tarefa 3.", hf_cpuid());
		printf("\ntarefa 3, CPU %d recebeu buffer da tarefa 1 com aresta %d\n", hf_cpuid(), size1);

		gen_rdm_bytestream (320, buf7);
		val7 = hf_sendack(2, 5007, buf7, 320, 0, 500);
		free(buf7);

		gen_rdm_bytestream (64, buf8);
		val8 = hf_sendack(1, 5008, buf8, 64, 0, 500);
		free(buf8);

		delay_ms(10);
	}

}

void t4(void){
	int8_t *buf1 = malloc (64);
	int8_t *buf8;
	uint16_t val1, val8;
	uint16_t cpu1, size1, task1;
	
	if (hf_comm_create(hf_selfid(), 5004, 0))
		panic(0xff);

	while(1) {

		val1 = hf_recvack(&cpu1, &task1, buf1, &size1, 0);
		printf("\nCPU %d, executando tarefa 4.", hf_cpuid());
		printf("\ntarefa 4, CPU %d recebeu buffer da tarefa 1 com aresta %d\n", hf_cpuid(), size1);

		gen_rdm_bytestream (64, buf8);
		val8 = hf_sendack(1, 5008, buf8, 64, 0, 500);
		free(buf8);

		delay_ms(10);
	}

}

void t5(void){
	int8_t *buf1 = malloc (64);
	int8_t *buf8;
	uint16_t val1, val8;
	uint16_t cpu1, size1, task1;
	
	if (hf_comm_create(hf_selfid(), 5005, 0))
		panic(0xff);

	while(1) {

		val1 = hf_recvack(&cpu1, &task1, buf1, &size1, 0);
		printf("\nCPU %d, executando tarefa 5.", hf_cpuid());
		printf("\ntarefa 5, CPU %d recebeu buffer da tarefa 1 com aresta %d\n", hf_cpuid(), size1);

		gen_rdm_bytestream (640, buf8);
		val8 = hf_sendack(1, 5008, buf8, 640, 0, 500);
		free(buf8);

		delay_ms(10);
	}

}


void t6(void){
	int8_t *buf2 = malloc (64);
	int8_t *buf9;

	uint16_t val2, val9;
	uint16_t cpu2, size2, task2;
	
	if (hf_comm_create(hf_selfid(), 5006, 0))
		panic(0xff);

	while(1) {

		val2 = hf_recvack(&cpu2, &task2, buf2, &size2, 0);
		printf("\nCPU %d, executando tarefa 6.", hf_cpuid());
		printf("\ntarefa 6, CPU %d recebeu buffer da tarefa 2 com aresta %d\n", hf_cpuid(), size2);

		gen_rdm_bytestream (640, buf9);
		val9 = hf_sendack(0, 5009, buf9, 640, 0, 500);
		free(buf9);

		
		delay_ms(10);
	}
}

void t7(void){
	int8_t *buf1 = malloc (320);//buf[320];
	int8_t *buf2 = malloc (1280);
	int8_t *buf3 = malloc (320);
	int8_t *buf9;

	uint16_t val1, val2, val3, val9;
	uint16_t cpu1, cpu2, cpu3, size1, size2, size3, task1, task2, task3;
	
	if (hf_comm_create(hf_selfid(), 5007, 0))
		panic(0xff);

	while(1) {

		val1 = hf_recvack(&cpu1, &task1, buf1, &size1, 0);
		printf("\nCPU %d, executando tarefa 7.", hf_cpuid());
		printf("\ntarefa 7, CPU %d recebeu buffer da tarefa 1 com aresta %d\n", hf_cpuid(), size1);

		val2 = hf_recvack(&cpu2, &task2, buf2, &size2, 0);
		printf("\ntarefa 7, CPU %d recebeu buffer da tarefa 2 com aresta %d\n", hf_cpuid(), size2);

		val3 = hf_recvack(&cpu3, &task3, buf3, &size3, 0);
		printf("\ntarefa 7, CPU %d recebeu buffer da tarefa 3 com aresta %d\n", hf_cpuid(), size3);

		gen_rdm_bytestream (640, buf9);
		val9 = hf_sendack(0, 5009, buf9, 640, 0, 500);
		free(buf9);
		
		delay_ms(10);
	}
}

void t8(void){
	int8_t *buf2 = malloc (320);//buf[320];
	int8_t *buf3 = malloc (64);
	int8_t *buf4 = malloc (64);
	int8_t *buf5 = malloc (640);
	int8_t *buf9;

	uint16_t val2, val3, val4, val5, val9;
	uint16_t cpu2, cpu3, cpu4, cpu5, size2, size3, size4, size5, task2, task3, task4, task5;
	
	if (hf_comm_create(hf_selfid(), 5008, 0))
		panic(0xff);

	while(1) {

		val2 = hf_recvack(&cpu2, &task2, buf2, &size2, 0);
		printf("\nCPU %d, executando tarefa 8.", hf_cpuid());
		printf("\ntarefa 8, CPU %d recebeu buffer da tarefa 2 com aresta %d\n", hf_cpuid(), size2);

		val3 = hf_recvack(&cpu3, &task3, buf3, &size3, 0);
		printf("\ntarefa 8, CPU %d recebeu buffer da tarefa 3 com aresta %d\n", hf_cpuid(), size3);

		val4 = hf_recvack(&cpu4, &task4, buf4, &size4, 0);
		printf("\ntarefa 8, CPU %d recebeu buffer da tarefa 4 com aresta %d\n", hf_cpuid(), size4);

		val5 = hf_recvack(&cpu5, &task5, buf5, &size5, 0);
		printf("\ntarefa 8, CPU %d recebeu buffer da tarefa 5 com aresta %d\n", hf_cpuid(), size5);

		gen_rdm_bytestream (640, buf9);
		val9 = hf_sendack(0, 5009, buf9, 640, 0, 500);
		free(buf9);
		
		delay_ms(10);
	}
}

void t9(void){

	int8_t *buf6 = malloc (640);//buf[320];
	int8_t *buf7 = malloc (640);
	int8_t *buf8 = malloc (640);

	uint16_t val6, val7, val8;
	uint16_t cpu6, cpu7, cpu8, size6, size7, size8, task6, task7, task8;
	
	if (hf_comm_create(hf_selfid(), 5009, 0))
		panic(0xff);

	while(1) {

		val6 = hf_recvack(&cpu6, &task6, buf6, &size6, 0);
		printf("\nCPU %d, executando tarefa 9.", hf_cpuid());
		printf("\ntarefa 9, CPU %d recebeu buffer da tarefa 6 com aresta %d\n", hf_cpuid(), size6);

		val7 = hf_recvack(&cpu7, &task7, buf7, &size7, 0);
		printf("\ntarefa 9, CPU %d recebeu buffer da tarefa 7 com aresta %d\n", hf_cpuid(), size7);
		
		val8 = hf_recvack(&cpu8, &task8, buf8, &size8, 0);
		printf("\ntarefa 9, CPU %d recebeu buffer da tarefa 8 com aresta %d\n", hf_cpuid(), size8);

		delay_ms(10);
	}
}


void app_main(void)
{
	/* 
	 * Thrads were spawned in the following order
	 * because certain threads need to be created
	 * before others.
	 *
	*/
	if (hf_cpuid() == 0){
		hf_spawn(t9, 0, 0, 0, "t9", 1920);
	}else if (hf_cpuid() == 1){
		hf_spawn(t8, 0, 0, 0, "t8", 2048);
	}else if (hf_cpuid() == 2){
		hf_spawn(t7, 0, 0, 0, "t7", 2560);
	}else if (hf_cpuid() == 3){
		hf_spawn(t4, 0, 0, 0, "t4", 512);
		hf_spawn(t5, 0, 0, 0, "t5", 1024);
		hf_spawn(t6, 0, 0, 0, "t6", 1024);
	}else if (hf_cpuid() == 4){
		hf_spawn(t2, 0, 0, 0, "t2", 1024);
		hf_spawn(t3, 0, 0, 0, "t3", 1024);
	}else if (hf_cpuid() == 5){
		hf_spawn(t1, 0, 0, 0, "t1", 6000);
	}
}
