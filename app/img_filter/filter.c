#include <hellfire.h>
#include <noc.h>
#include "image.h"
#include <math.h>

#define BW 2 // largura de uma parte da imagem
#define BH 3 // altura de uma parte da imagem
#define N_CPU 25 // número de cpus


uint8_t gausian(uint8_t buffer[5][5]){
	int32_t sum = 0, mpixel;
	uint8_t i, j;

	int16_t kernel[5][5] =	{	{2, 4, 5, 4, 2},
					{4, 9, 12, 9, 4},
					{5, 12, 15, 12, 5},
					{4, 9, 12, 9, 4},
					{2, 4, 5, 4, 2}
				};
	for (i = 0; i < 5; i++)
		for (j = 0; j < 5; j++)
			sum += ((int32_t)buffer[i][j] * (int32_t)kernel[i][j]);
	mpixel = (int32_t)(sum / 159);

	return (uint8_t)mpixel;
}

uint32_t isqrt(uint32_t a){ 
	uint32_t i, rem = 0, root = 0, divisor = 0;

	for (i = 0; i < 16; i++){
		root <<= 1;
		rem = ((rem << 2) + (a >> 30));
		a <<= 2;
		divisor = (root << 1) + 1;
		if (divisor <= rem){
			rem -= divisor;
			root++;
		}
	}
	return root;
}

uint8_t sobel(uint8_t buffer[3][3]){
	int32_t sum = 0, gx = 0, gy = 0;
	uint8_t i, j;

	int16_t kernelx[3][3] =	{	{-1, 0, 1},
					{-2, 0, 2},
					{-1, 0, 1},
				};
	int16_t kernely[3][3] =	{	{-1, -2, -1},
					{0, 0, 0},
					{1, 2, 1},
				};
	for (i = 0; i < 3; i++){
		for (j = 0; j < 3; j++){
			gx += ((int32_t)buffer[i][j] * (int32_t)kernelx[i][j]);
			gy += ((int32_t)buffer[i][j] * (int32_t)kernely[i][j]);
		}
	}
	
	sum = isqrt(gy * gy + gx * gx);

	if (sum > 255) sum = 255;
	if (sum < 0) sum = 0;

	return (uint8_t)sum;
}

int get_free_cpu(int8_t *work_map){
	uint8_t i=0;
	for(i=0; i<N_CPU; i++){
		if(work_map[i] == -1){
		 return i;
		}
	}
	return -1;
}

void init_work_map(int8_t *work_map){
	uint8_t i=0;
	for(i=0; i < N_CPU ; i++){
		work_map[i] = -1;
	}
}

int getIndex(int row, int col, int w){
	return row*w+col;
}

int shouldMirrorFirstLine(uint8_t p, int32_t wp){
	if(p <= wp){
		return 1;
	}else{
		return 0;
	}
}

int shouldMirrorLastLine(uint8_t p, int32_t wp, int32_t hp){
	if ( p > ((wp * hp) - wp) ) {
		return 1;
	}else{
		return 0;
	}
}

int shouldMirrorFirstCol(uint8_t p, int32_t wp){
	if ( p%wp == 1 || p == 1 ) {
		return 1;
	}else{
		return 0;
	}
}

int shouldMirrorLastCol(uint8_t p, int32_t wp){
	if ( p%wp == 0 ) {
		return 1;
	}else{
		return 0;
	}
}

void treatBoxFirstLine(uint8_t p, uint8_t *buf, int32_t wp, int32_t bl, int32_t bc, int32_t bw, int32_t i, int32_t l, int32_t c){
	int32_t boxPrevLineI = 0, prevLineI = 0;
	if(bl-1 == 0 && shouldMirrorFirstLine(p, wp)){
		if ((bc-1) == 0) {
			buf[getIndex(bl-1, bc-1, bw+2)] = image[i];
		}else if (bc == bw && l == 0) {
			buf[getIndex(bl-1, bc+1, bw+2)] = image[i];
		}
		boxPrevLineI = getIndex(bl-1, bc, bw+2);
		buf[boxPrevLineI] = image[i];
	}else if(bl-1 == 0){
		boxPrevLineI = getIndex(bl-1, bc, bw+2);
		prevLineI = getIndex(l-1, c, width);
		buf[boxPrevLineI] = image[prevLineI];
		if ((bc-1) == 0 && c != 0) {
			buf[getIndex(bl-1, bc-1, bw+2)] = image[getIndex(l-1, c-1, width)];
		}else if ((bc-1) == 0 && c == 0) {
			buf[getIndex(bl-1, bc-1, bw+2)] = image[i];
		}else if (bc == bw && l != 0 && p%wp != 0) {
			buf[getIndex(bl-1, bc+1, bw+2)] = image[getIndex(l-1, c+1, width)];
		}else if (bc == bw && l != 0 && p%wp == 0){
			buf[getIndex(bl-1, bc+1, bw+2)] = image[i];
		}

	}
}

void treatBoxFirstCol(uint8_t p, uint8_t *buf, int32_t wp, int32_t bl,int32_t bc, int32_t bw, int32_t i, int32_t l, int32_t c){
	int32_t boxPrevColI = 0, prevColI =0;
	if(bc-1 == 0 && shouldMirrorFirstCol(p, wp)) {
    	boxPrevColI = getIndex(bl, bc-1, bw+2);
    	buf[boxPrevColI] = image[i];
	}else if(bc-1 == 0){
		prevColI = getIndex(l, c-1, width);
		boxPrevColI = getIndex(bl, bc-1, bw+2);
		buf[boxPrevColI] = image[prevColI];
	}
}

void treatBoxLastLine(uint8_t p, uint8_t *buf, int32_t wp, int32_t hp, int32_t bl,int32_t bc, int32_t bw, int32_t i, int32_t l, int32_t c, int32_t el){
	int32_t boxLastLineI = 0, lastLineI = 0;
	if (l == el-1 && shouldMirrorLastLine(p, wp, hp)) {
		boxLastLineI = getIndex(bl+1, bc, bw+2);
		buf[boxLastLineI] = image[i];

		if ((bc-1) == 0) {
			buf[getIndex(bl+1, bc-1, bw+2)] = image[i];
		}else if (bc == bw) {
			buf[getIndex(bl+1, bc+1, bw+2)] = image[i];
		}

	}else if(l == el-1){

		lastLineI = getIndex(l+1, c, width);
		boxLastLineI = getIndex(bl+1, bc, bw+2);
		buf[boxLastLineI] = image[lastLineI];
		if ((bc-1) == 0 && c != 0) {
			buf[getIndex(bl+1, bc-1, bw+2)] = image[getIndex(l+1, c-1, width)];
		}else if ((bc-1) == 0 && c == 0) {
			buf[getIndex(bl+1, bc-1, bw+2)] = image[i];
		}else if (bc == bw && p%wp != 0) {
			buf[getIndex(bl+1, bc+1, bw+2)] = image[getIndex(l+1, c+1, width)];
		}else if (bc == bw && p%wp == 0){
			buf[getIndex(bl+1, bc+1, bw+2)] = image[i];
		}


	}	
}


void treatBoxLastCol(uint8_t p, uint8_t *buf, int32_t wp, int32_t bl,int32_t bc, int32_t bw, int32_t i, int32_t l, int32_t c,int32_t ec) {
	int32_t boxLastColI = 0, lastColI =0;
	if (c == ec-1 && shouldMirrorLastCol(p, wp)) {
		boxLastColI = getIndex(bl, bc+1, bw+2);
		buf[boxLastColI] = image[i];
	}else if(c == ec-1){
		lastColI = getIndex(l, c+1, width);
		boxLastColI = getIndex(bl, bc+1, bw+2);
		buf[boxLastColI] = image[lastColI];
	}
}

void get_image_part(uint8_t p, uint8_t *buf, int32_t wp, int32_t hp, int32_t bw, int32_t bh){
	int32_t sc =  ((p - ( ((p - 1) / wp) * wp)) -1)   * bw; 
	int32_t sl = ((p - 1) / wp) * bh;
	int32_t ec = sc + bw;
	int32_t el = sl + bh;
	int32_t l = sl, bl = 1;
	int32_t c = sc, bc = 1;

	printf("Part:%d, sc:%d, sl:%d, ec:%d, el:%d \n", p, sc, sl, ec, el);
	for(l = sl,bl = 1; l < el; l++, bl++){
		for(c = sc, bc = 1; c < ec; c++, bc++){
			
			int32_t i = getIndex(l, c, width);
			int32_t boxI = getIndex(bl, bc, bw+2); 
			
			treatBoxFirstLine(p, buf, wp, bl, bc, bw, i, l, c);
			
			treatBoxFirstCol(p, buf, wp, bl, bc, bw, i, l, c);
			
			buf[boxI] = image[i];

			treatBoxLastLine(p, buf, wp, hp, bl, bc, bw, i, l, c, el);

			treatBoxLastCol(p, buf, wp, bl, bc, bw, i, l, c, ec);
			
		}		
	}
}


void realoc_part(uint8_t *img_result, uint8_t part_received, uint8_t *buf){

}

void print_matrix(uint8_t *matrix, int32_t w, int32_t h){
	int32_t i =0;
	int32_t l = 0;
	int32_t c = 0;
	for(l = 0; l < h; l++){
		for(c = 0; c < w; c++){
			i = getIndex(l, c , w);
			printf("%d\t", matrix[i]);
		}
		printf("\n");
	}
}
void master(){

	uint8_t *img_result;
	uint16_t cpu, task, size, val, n_parts, current_part, part_received, ready_parts, wp, hp;
	uint8_t loop = 1;
	int8_t buf[(BW+2)*(BH+2)], work_map[N_CPU];

	if (hf_comm_create(hf_selfid(), 1111, 0))
		panic(0xff);
	
	// init in how many parts the image will be split
	wp = width/BW;
	hp = height/BH;
	n_parts = wp * hp;
	current_part = 1;
	ready_parts = 0;
	init_work_map(work_map);

	//img_result = (uint8_t *) malloc(height * width);
	
    while(loop)
    {
    	int next = get_free_cpu(work_map);
    	if(next != -1 && current_part <= n_parts){
    		// SEND WORK
    		get_image_part(current_part, buf, wp, hp, BW, BH);
    		print_matrix(buf, BW+2, BH+2);
    		printf("###########\n");
    		//val = hf_sendack(next, 2222, buf, sizeof(buf), next, 500);   
    		current_part++;

    	}else{
    		loop = 0;
    		/*
    		if(ready_parts < n_parts){
    			val = hf_recvack(&cpu, &task, buf, &size, 0);
    			part_received = work_map[cpu];
    			realoc_part(img_result, part_received, buf);
    			work_map[cpu] = -1;
    			ready_parts++;
    		}else{
    			loop = 0;
    		}*/
    	}
    }

}

void slave(){
	
}

void do_gausian(uint8_t *img, int32_t width, int32_t height){
	int32_t i, j, k, l;
	uint8_t image_buf[5][5];
	
	for(i = 0; i < height; i++){
		if (i > 1 || i < height-1){
			for(j = 0; j < width; j++){
				if (j > 1 || j < width-1){
					for (k = 0; k < 5;k++)
						for(l = 0; l < 5; l++)
							image_buf[k][l] = image[(((i + l-2) * width) + (j + k-2))];

					img[((i * width) + j)] = gausian(image_buf);
				}else{
					img[((i * width) + j)] = image[((i * width) + j)];
				}
			}
		}
	}
}

void do_sobel(uint8_t *img, int32_t width, int32_t height){
	int32_t i, j, k, l;
	uint8_t image_buf[3][3];
	
	for(i = 0; i < height; i++){
		if (i > 0 || i < height){
			for(j = 0; j < width; j++){
				if (j > 0 || j < width){
					for (k = 0; k < 3;k++)
						for(l = 0; l < 3; l++)
							image_buf[k][l] = image[(((i + l-1) * width) + (j + k-1))];

					img[((i * width) + j)] = sobel(image_buf);
				}else{
					img[((i * width) + j)] = image[((i * width) + j)];
				}
			}
		}
	}
}

// matrix, sub_matrix, height, width, block line, block column, block height, block width
void create_sub_matrix(uint8_t *a, uint8_t *sub_matrix, uint32_t h, uint32_t w, uint32_t k, uint32_t l, uint32_t bh, uint32_t bw) {
	int i = 0;
	int j = 0;
	
	while(i < bh) {
		j = 0;
		while(j < bw) {
			sub_matrix[i * w + j] = a[(i + k *bh) * w + (j + 1 * bw)];
			j = j + 1;
		}
		i = i + 1;
	}
}

void task(void){
	uint32_t i, j, k = 0;
	uint8_t *img;
	uint32_t time;

	// matrix_print_sub(m, 9, 8, 1, 2, 3, 2)
	
	while(1) {
		img = (uint8_t *) malloc(height * width);
		if (img == NULL){
			printf("\nmalloc() failed!\n");
			for(;;);
		}
		uint32_t sm_h = 32;
		uint32_t sm_w = 32;

		uint8_t sub_matrix[sm_h * sm_w];
		memset(sub_matrix, 0, sm_h * sm_w * sizeof(uint8_t));

	  create_sub_matrix(img, *sub_matrix, sm_h, sm_w, 1, 20, 20, 20);

		printf("\n\nstart of processing!\n\n");

		time = _readcounter();

		do_gausian(sub_matrix, sm_w, sm_h);
		do_sobel(sub_matrix, sm_w, sm_h);

		time = _readcounter() - time;

		printf("done in %d clock cycles.\n\n", time);

		printf("\n\nint32_t width = %d, height = %d;\n", width, sm_h);
		printf("uint8_t image[] = {\n");
		for (i = 0; i < sm_h; i++){
			for (j = 0; j < sm_w; j++){
				printf("0x%x", sub_matrix[i * sm_w + j]);
				if ((i < sm_h-1) || (j < sm_w-1)) printf(", ");
				if ((++k % 16) == 0) printf("\n");
			}
		}
		printf("};\n");

		free(img);

		printf("\n\nend of processing!\n");
		panic(0);
	}
		
}

void app_main(void) {
	if (hf_cpuid() == 0){
		hf_spawn(master, 0, 0, 0, "master", 2048);
	}
}
