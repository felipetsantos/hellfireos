#include <hellfire.h>
#include <noc.h>
#include "image.h"

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
		hf_spawn(task, 0, 0, 0, "filter", 2048);
	}
}
