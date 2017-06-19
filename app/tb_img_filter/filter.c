#include <hellfire.h>
#include <noc.h>
#include "image.h"
#include <math.h>

#define BW 24 // largura de uma parte da imagem
#define BH 18 // altura de uma parte da imagem
//#define BW 2 // largura de uma parte da imagem
//#define BH 3 // altura de uma parte da imagem
#define BORDER 4
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
		if (i > 1 && i < height-2){
			for(j = 0; j < width; j++){
				if (j > 1 && j < width-2){
					for (k = 0; k < 5;k++)
						for(l = 0; l < 5; l++)
							image_buf[k][l] = img[getIndex((i-2)+k,(j-2)+l,width)];

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
		if (i > 1 && i < height-2){
			for(j = 0; j < width; j++){
				if (j > 1 && j < width-2){
					for (k = 0; k < 3;k++)
						for(l = 0; l < 3; l++)
							image_buf[k][l] = img[getIndex((i-1)+k,(j-1)+l,width)];

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


void get_image_part(uint8_t p, uint8_t *buf, int32_t wp, int32_t hp, int32_t bw, int32_t bh){
	int32_t sc =  ((p - ( ((p - 1) / wp) * wp)) -1)   * bw; 
	int32_t sl = ((p - 1) / wp) * bh;
	int32_t ec = sc + bw;
	int32_t el = sl + bh;
	int32_t l = sl, bl = 2;
	int32_t c = sc, bc = 2;

	//printf("Part:%d, sc:%d, sl:%d, ec:%d, el:%d \n", p, sc, sl, ec, el);
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
			printf("%d\t", matrix[i]);
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
	int32_t dest_port = 5000;
	if (hf_comm_create(hf_selfid(), 1111, 0))
		panic(0xff);

	// init in how many parts the image will be split
	wp = width / BW;
	hp = height / BH;
	n_parts = wp * hp;

	printf("Partes:%d\n", n_parts);

	current_part = 1;
	ready_parts = 0;
	init_work_map(work_map);
	//print_matrix(image, width, height );
	img_result = (uint8_t *) malloc(height * width);

    while(loop)
    {
    	int next = get_free_cpu(work_map);
    	
    	if(next != -1 && current_part <= n_parts){
    		
    		// SEND WORK
    		memset(buf, 0, (BW+BORDER)*(BH+BORDER) * sizeof(uint8_t));
    		get_image_part(current_part, buf, wp, hp, BW, BH);
    		dest_port = 5000 + next;
    		//printf("Next:%d, dest_port:%d \n", next, dest_port);
    		val = hf_sendack(next, dest_port, buf, sizeof(buf), next+300, 600);  
			if (val)
				printf("hf_sendack(): error %d\n", val);    		
    	//	printf("Enviou parte %d para o escravo %d, porta:%d, canal:%d\n", current_part, next, dest_port,next+300);
    		//print_matrix(buf, BW+BORDER, BH+BORDER);
    		//printf("######\n\n\n"); 
    		work_map[next] = current_part;
    		current_part++;

    	}else{
    		
    		int32_t channel = -1;
    		int8_t received_p = 0;
    		if(ready_parts < n_parts){
    			while(received_p < N_CPU-1 && ready_parts < n_parts){
	    			channel = hf_recvprobe();
					if (channel >= 0) {
		    			//printf("Esperando matrix do escravo.\n");
		    			val = hf_recvack(&cpu, &task, buf, &size, channel);
		    			if (val)
							printf("hf_recvack(): error %d\n", val);
		    			//printf("Recebeu matrix do escravo:%d canal:%d\n",cpu, channel);
		    			//print_matrix(buf, BW+BORDER, BH+BORDER);
		    			part_received = work_map[cpu];
		    			realoc_part(img_result, part_received, buf, wp, hp, BW, BH);
		    			work_map[cpu] = -1;
		    			//printf("Partes:%d\n", ready_parts);
		    			ready_parts++;
		 				received_p++;		
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
	uint16_t src_cpu, src_port, size;
	int16_t val;
	int8_t recv_buf[(BW+BORDER)*(BH+BORDER)];

	if (hf_comm_create(hf_selfid(), port, 0)) {
			panic(0xff);
	}
	printf("Inicializou escravo: %d na porta%d\n", hf_cpuid(), port);
	
	
	//uint32_t time;

	while (1){
		channel = hf_recvprobe();
		if(channel >= 0){
			//printf("Escravo %d esperando trabalho. Porta: %d\n", hf_cpuid(), port);

			//memset(recv_buf, 0, (BW+BORDER)*(BH+BORDER)*sizeof(uint8_t));
			val = hf_recvack(&src_cpu, &src_port, recv_buf, &size, channel);
			if (val)
				printf("hf_recvack(): error %d\n", val);
			//printf("Escravo %d recebeu trabalho no canal %d.\n", hf_cpuid(), channel);
			//time = _readcounter();

			do_gausian(recv_buf, BW + BORDER, BH + BORDER);
			do_sobel(recv_buf, BW+BORDER, BH+BORDER);
			delay_ms(50);
			
			//print_matrix(recv_buf, BW+BORDER, BH+BORDER);
			//printf("Escravo %d  vai enviar trabalho feito, para porta %d no canal %d.\n", hf_cpuid(), 1111, hf_cpuid()+100);
			val = hf_sendack(0, 1111, recv_buf, sizeof(recv_buf), hf_cpuid()+100, 100);
			if (val)
				printf("hf_sendack(): error %d\n", val);
			//printf("Escravo %d  enviou trabalho feito.\n", hf_cpuid());
			//time = _readcounter() - time;

			//printf("done in %d clock cycles.\n\n", time);
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
