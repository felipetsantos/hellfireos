#include <hellfire.h>
	
void polling_server(void){
	printf("Executou polling server");
}


void app_main(void){
	hf_spawn(polling_server,3,1,3,"polling_server", 2048);
}

