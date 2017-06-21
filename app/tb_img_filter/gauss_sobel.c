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

int32_t convolution(uint8_t buffer[3][3],int16_t kernel[3][3]){
	int32_t g;
	g = ((int32_t)buffer[2][2] * (int32_t)kernel[0][0])
		   +
		   ((int32_t)buffer[2][1] * (int32_t)kernel[0][1])
		   +
		   ((int32_t)buffer[2][0] * (int32_t)kernel[0][2]);
	
	g += ((int32_t)buffer[1][2] * (int32_t)kernel[1][0])
		   +
		   ((int32_t)buffer[1][1] * (int32_t)kernel[1][1])
		   +
		   ((int32_t)buffer[1][0] * (int32_t)kernel[1][2]);

	g += ((int32_t)buffer[0][2] * (int32_t)kernel[2][0])
		   +
		   ((int32_t)buffer[0][1] * (int32_t)kernel[2][1])
		   +
		   ((int32_t)buffer[0][0] * (int32_t)kernel[2][2]);
	return g;
}

uint8_t sobel(uint8_t buffer[3][3]){
	int32_t sum = 0, gx = 0, gy = 0;
	uint8_t i, j;

	int16_t kernelx[3][3] =	{	{1, 0, -1},
					{2, 0, -2},
					{1, 0, -1},
				};
	int16_t kernely[3][3] =	{	{1, 2, 1},
					{0, 0, 0},
					{-1, -2, -1},
				};

	gx = convolution(buffer, kernelx);
	gy = convolution(buffer, kernely);

	//gy += ((int32_t)buffer[i][j] * (int32_t)kernely[i][j]);
	
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
	uint8_t *cp_img;
	cp_img = (uint8_t *) malloc(height * width);
	memcpy(cp_img,img,height * width);
	for(i = 1; i < height-1; i++){
		for(j = 1; j < width-1; j++){
				for (k = 0; k < 3;k++){
					for(l = 0; l < 3; l++){
						image_buf[k][l] = cp_img[getIndex((i-1)+k,(j-1)+l,width)];
					}
					
				}
				img[((i * width) + j)] = sobel(image_buf);
		}
	}
}