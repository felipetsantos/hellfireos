#include <hellfire.h>
#include <noc.h>
#include "image.h"
#include <math.h>

#define BW 24 // largura de uma parte da imagem
#define BH 18 // altura de uma parte da imagem
#define BORDER 2
#define N_CPU 6 // número de cpus

//convert car.png -alpha set -define bmp:format=bmp3 -grayscale Rec709Luma  car.bmp

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

void do_gausian(uint8_t *img, int32_t width, int32_t height){
	int32_t i, j, k, l;
	uint8_t image_buf[5][5];
	
	for(i = 0; i < height; i++){
		if (i > 1 || i < height-1){
			for(j = 0; j < width; j++){
				if (j > 1 || j < width-1){
					for (k = 0; k < 5;k++)
						for(l = 0; l < 5; l++)
							image_buf[k][l] = img[(((i + l-2) * width) + (j + k-2))];

					img[((i * width) + j)] = gausian(image_buf);
				}else{
					img[((i * width) + j)] = img[((i * width) + j)];
				}
			}
		}
	}
}

void do_sobel(uint8_t *img, int32_t width, int32_t height){
	int32_t i, j, k, l;
	uint8_t image_buf[3][3];
	
	for(i = 0; i < height; i++){
		if (i > 0 || i < height-1){
			for(j = 0; j < width; j++){
				if (j > 0 || j < width){
					for (k = 0; k < 3;k++)
						for(l = 0; l < 3; l++)
							image_buf[k][l] = img[(((i + l-1) * width) + (j + k-1))];

					img[((i * width) + j)] = sobel(image_buf);
				}else{
					img[((i * width) + j)] = img[((i * width) + j)];
				}
			}
		}
	}
}


int get_free_cpu(int8_t *work_map){
	uint8_t i=0;
	for(i=1; i<N_CPU; i++){
		if(work_map[i] == -1){
		 return i;
		}
	}
	return -1;
}

void init_work_map(int8_t *work_map){
	uint8_t i=0;
	work_map[0] = 0;
	for(i=1; i < N_CPU ; i++){
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
			buf[getIndex(bl-1, bc-1, bw+BORDER)] = image[i];
		}else if (bc == bw && l == 0) {
			buf[getIndex(bl-1, bc+1, bw+BORDER)] = image[i];
		}
		boxPrevLineI = getIndex(bl-1, bc, bw+BORDER);
		buf[boxPrevLineI] = image[i];
	}else if(bl-1 == 0){
		boxPrevLineI = getIndex(bl-1, bc, bw+BORDER);
		prevLineI = getIndex(l-1, c, width);
		buf[boxPrevLineI] = image[prevLineI];
		if ((bc-1) == 0 && c != 0) {
			buf[getIndex(bl-1, bc-1, bw+BORDER)] = image[getIndex(l-1, c-1, width)];
		}else if ((bc-1) == 0 && c == 0) {
			buf[getIndex(bl-1, bc-1, bw+BORDER)] = image[i];
		}else if (bc == bw && l != 0 && p%wp != 0) {
			buf[getIndex(bl-1, bc+1, bw+BORDER)] = image[getIndex(l-1, c+1, width)];
		}else if (bc == bw && l != 0 && p%wp == 0){
			buf[getIndex(bl-1, bc+1, bw+BORDER)] = image[i];
		}

	}
}

void treatBoxFirstCol(uint8_t p, uint8_t *buf, int32_t wp, int32_t bl,int32_t bc, int32_t bw, int32_t i, int32_t l, int32_t c){
	int32_t boxPrevColI = 0, prevColI =0;
	if(bc-1 == 0 && shouldMirrorFirstCol(p, wp)) {
    	boxPrevColI = getIndex(bl, bc-1, bw+BORDER);
    	buf[boxPrevColI] = image[i];
	}else if(bc-1 == 0){
		prevColI = getIndex(l, c-1, width);
		boxPrevColI = getIndex(bl, bc-1, bw+BORDER);
		buf[boxPrevColI] = image[prevColI];
	}
}

void treatBoxLastLine(uint8_t p, uint8_t *buf, int32_t wp, int32_t hp, int32_t bl,int32_t bc, int32_t bw, int32_t i, int32_t l, int32_t c, int32_t el){
	int32_t boxLastLineI = 0, lastLineI = 0;
	if (l == el-1 && shouldMirrorLastLine(p, wp, hp)) {
		boxLastLineI = getIndex(bl+1, bc, bw+BORDER);
		buf[boxLastLineI] = image[i];

		if ((bc-1) == 0) {
			buf[getIndex(bl+1, bc-1, bw+BORDER)] = image[i];
		}else if (bc == bw) {
			buf[getIndex(bl+1, bc+1, bw+BORDER)] = image[i];
		}

	}else if(l == el-1){

		lastLineI = getIndex(l+1, c, width);
		boxLastLineI = getIndex(bl+1, bc, bw+BORDER);
		buf[boxLastLineI] = image[lastLineI];
		if ((bc-1) == 0 && c != 0) {
			buf[getIndex(bl+1, bc-1, bw+BORDER)] = image[getIndex(l+1, c-1, width)];
		}else if ((bc-1) == 0 && c == 0) {
			buf[getIndex(bl+1, bc-1, bw+BORDER)] = image[i];
		}else if (bc == bw && p%wp != 0) {
			buf[getIndex(bl+1, bc+1, bw+BORDER)] = image[getIndex(l+1, c+1, width)];
		}else if (bc == bw && p%wp == 0){
			buf[getIndex(bl+1, bc+1, bw+BORDER)] = image[i];
		}


	}	
}


void treatBoxLastCol(uint8_t p, uint8_t *buf, int32_t wp, int32_t bl,int32_t bc, int32_t bw, int32_t i, int32_t l, int32_t c,int32_t ec) {
	int32_t boxLastColI = 0, lastColI =0;
	if (c == ec-1 && shouldMirrorLastCol(p, wp)) {
		boxLastColI = getIndex(bl, bc+1, bw+BORDER);
		buf[boxLastColI] = image[i];
	}else if(c == ec-1){
		lastColI = getIndex(l, c+1, width);
		boxLastColI = getIndex(bl, bc+1, bw+BORDER);
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

	//printf("Part:%d, sc:%d, sl:%d, ec:%d, el:%d \n", p, sc, sl, ec, el);
	for(l = sl,bl = 1; l < el; l++, bl++){
		for(c = sc, bc = 1; c < ec; c++, bc++){
			
			int32_t i = getIndex(l, c, width);
			int32_t boxI = getIndex(bl, bc, bw+BORDER); 
			
			treatBoxFirstLine(p, buf, wp, bl, bc, bw, i, l, c);
			
			treatBoxFirstCol(p, buf, wp, bl, bc, bw, i, l, c);
			
			buf[boxI] = image[i];

			treatBoxLastLine(p, buf, wp, hp, bl, bc, bw, i, l, c, el);

			treatBoxLastCol(p, buf, wp, bl, bc, bw, i, l, c, ec);
			
		}		
	}
}


void realoc_part(uint8_t *img_result, uint8_t p, uint8_t *buf, int32_t wp, int32_t hp, int32_t bw, int32_t bh){
	int32_t sc =  ((p - ( ((p - 1) / wp) * wp)) -1)   * bw; 
	int32_t sl = ((p - 1) / wp) * bh;
	int32_t ec = sc + bw;
	int32_t el = sl + bh;
	int32_t l = sl, bl = 1;
	int32_t c = sc, bc = 1;
	for(l = sl,bl = 1; l < el; l++, bl++){
		for(c = sc, bc = 1; c < ec; c++, bc++){
			int32_t i = getIndex(l, c, width);
			int32_t boxI = getIndex(bl, bc, bw+BORDER);
			img_result[i] = buf[boxI];
		}		
	}
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

void print_final_result(uint8_t *img_result){
		uint8_t i =0, j=0, k = 0;
		printf("\n\nint32_t width = %d, height = %d;\n", width, height);
		printf("uint8_t image[] = {\n");
		for (i = 0; i < height; i++){
			for (j = 0; j < width; j++){
				printf("0x%x", img_result[i * width + j]);
				if ((i < height-1) || (j < width-1)) printf(", ");
				if ((++k % width) == 0) printf("\n");
			}
		}
		printf("};\n");
}

void master(){

	uint8_t *img_result;
	uint16_t cpu, task, size, val, n_parts, current_part, part_received, ready_parts, wp, hp;
	uint8_t loop = 1;
	int8_t buf[(BW+BORDER)*(BH+BORDER)], work_map[N_CPU];
	int32_t dest_port = 5000;
	if (hf_comm_create(hf_selfid(), 1111, 0))
		panic(0xff);
	// init in how many parts the image will be split
	wp = width/BW;
	hp = height/BH;
	n_parts = wp * hp;
	printf("Partes:%d\n", n_parts);
	current_part = 1;
	ready_parts = 0;
	init_work_map(work_map);
	img_result = (uint8_t *) malloc(height * width);
	
    while(loop)
    {
    	int next = get_free_cpu(work_map);
    	
    	if(next != -1 && current_part <= n_parts){
    		//printf("Next:%d, current_part:%d, n_parts:%d\n", next, current_part, n_parts);
    		// SEND WORK
    		//memset(buf, 0, (BW+BORDER)*(BH+BORDER) * sizeof(uint8_t));
    		get_image_part(current_part, buf, wp, hp, BW, BH);
    		dest_port = 5000 + next;
    		val = hf_sendack(next, dest_port, buf, sizeof(buf), next, 100);  
    		printf("Enviou parte %d para o escravo %d, porta:%d\n", current_part, next, dest_port);
    		//print_matrix(buf, BW+BORDER, BH+BORDER);
    		work_map[next] = current_part;
    		current_part++;

    	}else{
    		int32_t channel = -1;
    		if(ready_parts < n_parts){
    			while(1){
	    			channel = hf_recvprobe();
					if (channel >= 0) {
		    			//printf("Esperando matrix do escravo.\n");
		    			val = hf_recvack(&cpu, &task, buf, &size, channel);
		    			printf("Recebeu matrix do escravo:%d\n",cpu);
		    			//print_matrix(buf, BW+BORDER, BH+BORDER);
		    			part_received = work_map[cpu];
		    			realoc_part(img_result, part_received, buf, wp, hp, BW, BH);
		    			work_map[cpu] = -1;
		    			printf("Partes:%d\n", ready_parts);
		    			ready_parts++;
		    			break;
	    			}
    			}
    			
    		}else{
    			loop = 0;
    		}
    	}
    }
    print_final_result(img_result);
    free(img_result);
}

void slave(){
	uint32_t port = 5000+hf_cpuid();
	int32_t channel;
	uint16_t val, src_cpu, src_port, size;
	int8_t recv_buf[(BW+BORDER)*(BH+BORDER)];
	if (hf_comm_create(hf_selfid(), port, 0)) {
			panic(0xff);
	}
	//printf("Inicializou escravo: %d\n", hf_cpuid());
	
	
	//uint32_t time;

	while (1){
		channel = hf_recvprobe();
		if(channel >= 0){
			printf("Escravo %d esperando trabalho. Porta: %d\n", hf_cpuid(), port);

			//memset(recv_buf, 0, (BW+BORDER)*(BH+BORDER)*sizeof(uint8_t));
			val = hf_recvack(&src_cpu, &src_port, recv_buf, &size, channel);
			//printf("Escravo %d recebeu trabalho.\n", hf_cpuid());
			//time = _readcounter();

			//do_gausian(recv_buf, BW+BORDER, BH+BORDER);
			//do_sobel(recv_buf, BW+BORDER, BH+BORDER);

			
			//print_matrix(recv_buf, BW+BORDER, BH+BORDER);

			val = hf_sendack(0, 1111, recv_buf, sizeof(recv_buf), hf_cpuid()+100, 100);
			printf("Escravo %d  enviou trabalho feito.\n", hf_cpuid());
			//time = _readcounter() - time;

			//printf("done in %d clock cycles.\n\n", time);
		}
	}	
}

void app_main(void) {
	if (hf_cpuid() == 0){
		hf_spawn(master, 0, 0, 0, "master", 8096);
	} else {
		hf_spawn(slave, 0, 0, 0, "slave", 8096);
	}
}
