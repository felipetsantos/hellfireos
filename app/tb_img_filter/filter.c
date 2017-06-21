#include <hellfire.h>
#include <noc.h>
#include "image.h"
#include <math.h>
#include "gauss_sobel.c"

#define BW 32
#define BH 32
#define BORDER 4
#define N_CPU 6 // número de cpus

//convert car.png -alpha set -define bmp:format=bmp3 -grayscale Rec709Luma  car.bmp

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

int32_t getIndex(int32_t row, int32_t col, int32_t w){
	return row*w+col;
}


void get_image_part(uint8_t p, uint8_t *buf, int32_t wp, int32_t hp, int32_t bw, int32_t bh){
	int32_t sc =  ((p - ( ((p - 1) / wp) * wp)) -1)   * bw; 
	int32_t sl = ((p - 1) / wp) * bh;
	int32_t ec = sc + bw;
	int32_t el = sl + bh;
	int32_t l = sl, bl = 2;
	int32_t c = sc, bc = 2;

	for(l = sl,bl = 2; l < el; l++, bl++){
		for(c = sc, bc = 2; c < ec; c++, bc++){
			
			int32_t i = getIndex(l, c, width);
			int32_t boxI = getIndex(bl, bc, bw+BORDER); 
			
			buf[boxI] = image[i];	
		}		
	}

	fillTopLines(buf, bw, bh, sl, sc, el, ec);
	fillLeftCols(buf, bw, bh, sl, sc, el, ec);
	fillBottomLines(buf, bw, bh, sl, sc, el, ec);
	fillRightCols(buf, bw, bh, sl, sc, el, ec);
}

void fillLeftCols(uint8_t *buf, int32_t bw, int32_t bh, int32_t sl, int32_t sc, int32_t el, int32_t ec){
	int8_t l,c;
	int32_t il;
	uint8_t bwB = bw + BORDER;
	uint8_t bhB = bh + BORDER;
	
	// Last line of matrix
	if(sc == 0){
		for (l = 2,il = sl;l < bhB-2; l++, il++) {
			buf[getIndex(l, 0, bwB)] = buf[getIndex(l, 3, bwB)];
			buf[getIndex(l, 1, bwB)] = buf[getIndex(l, 2, bwB)];
		}
	}else {
			for (l = 2,il = sl;l < bhB-2; l++, il++) {
				buf[getIndex(l, 0, bwB)] = image[getIndex(il, sc - 2, width)];
				buf[getIndex(l, 1, bwB)] = image[getIndex(il, sc - 1, width)];
			}
	}
}

void fillRightCols(uint8_t *buf, int32_t bw, int32_t bh, int32_t sl, int32_t sc, int32_t el, int32_t ec){
	int8_t l,c;
	int32_t il;
	uint8_t bwB = bw + BORDER;
	uint8_t bhB = bh + BORDER;
	
	// Last line of matrix
	if(ec == width){
		for (l = 2,il = sl;l < bhB-2; l++, il++) {
			buf[getIndex(l, bwB - 1, bwB)] = buf[getIndex(l, bwB - 4, bwB)];
			buf[getIndex(l, bwB - 2, bwB)] = buf[getIndex(l, bwB - 3, bwB)];
		}
	}else {
			for (l = 2,il = sl;l < bhB-2; l++, il++) {
				buf[getIndex(l, bwB - 1, bwB)] = image[getIndex(il, ec + 1, width)];
				buf[getIndex(l, bwB - 2, bwB)] = image[getIndex(il, ec, width)];
			}
	}
}

void fillBottomLines(uint8_t *buf, int32_t bw, int32_t bh, int32_t sl, int32_t sc, int32_t el, int32_t ec){
	int8_t l,c;
	int32_t ic;
	uint8_t bwB = bw + BORDER;
	uint8_t bhB = bh + BORDER;
	
	// Last line of matrix
	if(el == height){
		if(sc == 0){
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex((bhB-2)+l, c, bwB)] = image[getIndex(el-(1+l), 1-c, width)];
					buf[getIndex((bhB-2)+l, (bwB-2)+c, bwB)] = image[getIndex(el-(1+l), c+bw, width)];
				}
			}
		}else if(sc + bw == width){
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex((bhB-2)+l, c, bwB)] = image[getIndex(el-(1+l), sc-(2-c), width)];
					buf[getIndex((bhB-2)+l, (bwB-2)+c, bwB)] = image[getIndex(el-(1+l), ec-(c+1), width)];
				}
			}
		}else{

			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex((bhB-2)+l, c, bwB)] = image[getIndex(el-(1+l), sc-(2-c), width)];
					buf[getIndex((bhB-2)+l, (bwB-2)+c, bwB)] = image[getIndex(el-(1+l), sc+c+bw, width)];
				}
			}
		}
		for (c = 2;c < bwB-2; c++) {
			buf[getIndex(bhB - 1,c, bwB)] = buf[getIndex(bhB - 4,c, bwB)];
			buf[getIndex(bhB - 2,c, bwB)] = buf[getIndex(bhB - 3,c, bwB)];
		}
		
	}else{
		 
		if(sc == 0){
			buf[getIndex(bhB - 1, 0, bwB)] = image[getIndex(el+1, sc+1, width)];
			buf[getIndex(bhB - 1, 1, bwB)] = image[getIndex(el+1, sc, width)];
			buf[getIndex(bhB - 2, 0, bwB)] = image[getIndex(el, sc+1, width)];
			buf[getIndex(bhB - 2, 1, bwB)] = image[getIndex(el, sc, width)];
		}else{
			buf[getIndex(bhB - 1, 0, bwB)] = image[getIndex(el + 1, sc-2, width)];
			buf[getIndex(bhB - 1, 1, bwB)] = image[getIndex(el + 1, sc-1, width)];
			buf[getIndex(bhB - 2, 0, bwB)] = image[getIndex(el, sc-2, width)];
			buf[getIndex(bhB - 2, 1, bwB)] = image[getIndex(el, sc-1, width)];
		}

		if(sc + bw == width){
			buf[getIndex(bhB - 1, bwB - 1, bwB)] = image[getIndex(el + 1, ec - 2, width)];
			buf[getIndex(bhB - 1, bwB - 2, bwB)] = image[getIndex(el + 1, ec - 1, width)];
			buf[getIndex(bhB - 2, bwB - 1, bwB)] = image[getIndex(el, ec - 2, width)];
			buf[getIndex(bhB - 2, bwB - 2, bwB)] = image[getIndex(el, ec - 1, width)];
		}else{
			buf[getIndex(bhB - 1, bwB - 1, bwB)] = image[getIndex(el + 1, sc + bw + 1, width)];
			buf[getIndex(bhB - 1, bwB - 2, bwB)] = image[getIndex(el + 1, sc + bw, width)];
			buf[getIndex(bhB - 2, bwB - 1, bwB)] = image[getIndex(el + 0, sc + bw + 1, width)];
			buf[getIndex(bhB - 2, bwB - 2, bwB)] = image[getIndex(el + 0, sc + bw, width)];
		}

		for (c = 2,ic = sc;c < bwB-2; c++, ic++) {
			buf[getIndex(bhB - 1,c, bwB)] = image[getIndex(el + 1, ic, width)];
			buf[getIndex(bhB - 2,c, bwB)] = image[getIndex(el, ic, width)];
		}
	}
}

void fillTopLines(uint8_t *buf, int32_t bw, int32_t bh, int32_t sl, int32_t sc, int32_t el, int32_t ec){
	int8_t l,c;
	int32_t  ic;
	uint8_t bwB = bw + BORDER;
	uint8_t bhB = bh + BORDER;
	
	if(sl == 0){
		int8_t boxI = getIndex(2, 2, bwB);
		if(sc == 0){
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, c, bwB)] = buf[getIndex(3-l, 3-c, bwB)];
					buf[getIndex(l, bwB-(c+1), bwB)] = buf[getIndex(3-l, bwB-(4-c), bwB)];
				}
			}
		}else{
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, c, bwB)] = image[getIndex(1-l, sc-(2-c), width)];
				}
			}
		}

		if(sc+bw == width){
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, bwB-(c+1), bwB)] = image[getIndex(1-l,  (sc+bw)-(2-c), width)];
				}
			}
		}else if(sc != 0){
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, bwB-(c+1), bwB)] = image[getIndex(1-l,  (sc+bw)+(1-c), width)];
				}
			}
		}
		for (l = 0;l < 2; l++) {
			for (c = 2;c < bwB-2; c++) {
				buf[getIndex(l,c, bwB)] = buf[getIndex(3-l,c, bwB)];
			}
		}
		
	}else{
		 
		if(sc == 0){
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, c, bwB)] = image[getIndex(sl-(2-l), sc+(1-c), width)];
				}
			}
		}else {
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, c, bwB)] = image[getIndex(sl-(2-l), sc-(2-c), width)];
				}
			}
		}

		if(sc+bw == width){
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, bwB-(c+1), bwB)] = image[getIndex(sl-(2-l), ec-(2-c), width)];

				}
			}
		}else {
			for (l = 0;l < 2; l++) {
				for (c = 0;c < 2; c++) {
					buf[getIndex(l, bwB-(1+c), bwB)] = image[getIndex(sl-(2-l), sc+bw+(1-c), width)];

				}
			}
		}

		for (c = 2,ic = sc;c < bwB-2; c++, ic++) {
			buf[getIndex(0,c, bwB)] = image[getIndex(sl-2, ic, width)];
			buf[getIndex(1,c, bwB)] = image[getIndex(sl-1, ic, width)];
		}
	}
}

void realoc_part(uint8_t *img_result, uint8_t p, uint8_t *buf, int32_t wp, int32_t hp, int32_t bw, int32_t bh){
	int32_t sc =  ((p - ( ((p - 1) / wp) * wp)) -1)   * bw; 
	int32_t sl = ((p - 1) / wp) * bh;
	int32_t ec = sc + bw;
	int32_t el = sl + bh;
	int32_t l = sl, bl = 2;
	int32_t c = sc, bc = 2; 
	for(l = sl,bl = 2; l < el; l++, bl++){
		for(c = sc, bc = 2; c < ec; c++, bc++){
			int32_t i = getIndex(l, c, width);
			int32_t boxI = getIndex(bl, bc, bw + BORDER);
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
			printf("0x%x\t", matrix[i]);
		}
		printf("\n");
	}
}



void print_final_result(uint8_t *img_result){
		int32_t i =0, j=0, k = 0;
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
	uint16_t cpu, task, size, n_parts, current_part, part_received, ready_parts, wp, hp;
	int16_t val;
	uint8_t loop = 1;
	int8_t buf[(BW+BORDER)*(BH+BORDER)], work_map[N_CPU];
	int32_t dest_port = 5000, time;

	
	
	if (hf_comm_create(hf_selfid(), 1111, 0))
		panic(0xff);

	time = _readcounter();
	// init in how many parts the image will be split
	wp = width / BW;
	hp = height / BH;
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
    		
    		// SEND WORK
    		get_image_part(current_part, buf, wp, hp, BW, BH);
    		dest_port = 5000 + next;
    		val = hf_sendack(next, dest_port, buf, sizeof(buf), next+300, 600);  
			if (val)
				printf("hf_sendack(): error %d\n", val);    		 
    		work_map[next] = current_part;
    		current_part++;

    	}else{
    		// RECEIVE WORK
    		int32_t channel = -1;
    		int8_t received_p = 0;
    		if(ready_parts < n_parts){
    			while(received_p < N_CPU-1 && ready_parts < n_parts){
	    			channel = hf_recvprobe();
					if (channel >= 0) {
		    			
		    			val = hf_recvack(&cpu, &task, buf, &size, channel);
		    			if (val)
							printf("hf_recvack(): error %d\n", val);
		    			part_received = work_map[cpu];
		    			realoc_part(img_result, part_received, buf, wp, hp, BW, BH);
		    			work_map[cpu] = -1;
		    			ready_parts++;
		 				received_p++;		
	    			}
    			}
    			
    		}else{
    			loop = 0;
    		}
    	}
    }

	time = _readcounter() - time;
	printf("done in %d clock cycles.\n\n", time);
    print_final_result(img_result);
    free(img_result);
}

void slave(){
	uint32_t port = 5000+hf_cpuid();
	int32_t channel;
	uint16_t src_cpu, src_port, size;
	int16_t val;
	int8_t recv_buf[(BW+BORDER)*(BH+BORDER)];

	if (hf_comm_create(hf_selfid(), port, 0)) {
			panic(0xff);
	}
	printf("Inicializou escravo: %d na porta%d\n", hf_cpuid(), port);

	while (1){
		channel = hf_recvprobe();
		if(channel >= 0){
			//memset(recv_buf, 0, (BW+BORDER)*(BH+BORDER)*sizeof(uint8_t));
			val = hf_recvack(&src_cpu, &src_port, recv_buf, &size, channel);
			if (val)
				printf("hf_recvack(): error %d\n", val);

			do_gausian(recv_buf, BW + BORDER, BH + BORDER);
			do_sobel(recv_buf, BW+BORDER, BH+BORDER);
			
			val = hf_sendack(0, 1111, recv_buf, sizeof(recv_buf), hf_cpuid()+100, 100);
			if (val)
				printf("hf_sendack(): error %d\n", val);
		}
	}	
}

void app_main(void) {
	if (hf_cpuid() == 0){
		hf_spawn(master, 0, 0, 0, "master", 4096);
	} else {
		hf_spawn(slave, 0, 0, 0, "slave", 4096);
	}
}
