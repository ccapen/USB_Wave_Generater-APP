#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

static inline int is_little_endian(void) {
    uint16_t x = 0x0001;
    return *(uint8_t*)&x == 0x01;   // 低地址是 0x01 → 小端
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

double sawtooth(double x) {
	double y = fmod(x + 1.0, 2.0);
	y = (y < 0) ? (y + 2.0) : y;
	return (y - 1.0);
}

int16_t* generate_sawtooth(int n, double xmin, double xmax, uint16_t absymax) {
    int16_t* y = (int16_t*)malloc(n * sizeof(int16_t));
    if (y == NULL) return NULL;

    double dx = (xmax - xmin) / n;
    for (int i = 0; i < n; ++i) {
        double x = xmin + i * dx;
        y[i] = (int16_t)(sawtooth(x) * absymax);
    }
    return y;
}

int16_t* generate_sin(int n, double xmin, double xmax, uint16_t absymax) {
    int16_t* y = (int16_t*)malloc(n * sizeof(int16_t));
    if (y == NULL) return NULL;

    double dx = (xmax - xmin) / n;
    for (int i = 0; i < n; ++i) {
        double x = xmin + i * dx;
        y[i] = (int16_t)(sin(x) * absymax);
    }
    return y;
}

/* 归一化 sinc: sin(pi*x)/(pi*x)，x=0 时为 1 */
double sinc(double x) {
    if (fabs(x) < 1e-12) return 1.0;   /* 避免除零 */
    double pix = M_PI * x;
    return sin(pix) / pix;
}

int16_t* generate_sinc(int n, double xmin, double xmax, uint16_t absymax) {
    int16_t* y = (int16_t*)malloc(n * sizeof(int16_t));
    if (y == NULL) return NULL;

    double dx = (xmax - xmin) / (n - 1);
    for (int i = 0; i < n; ++i) {
        double x = xmin + i * dx;
        y[i] = (int16_t)(sinc(x) * absymax);
    }
    return y;
}

int16_t* generate_chirp_accum(double fs, double T, double f0, double f1,
                             uint16_t absymax, uint32_t* out_n) {
    uint32_t n = (uint32_t)(fs * T);
    int16_t* y = (int16_t*)malloc(n * sizeof(int16_t));
    if (y == NULL) return NULL;

    double dt = 1.0 / fs;
    double k  = (f1 - f0) / T;   /* 调频斜率 */
    double phase = 0.0;

    for (int i = 0; i < n; ++i) {
        y[i] = (int16_t)(absymax * cos(phase));
        double f = f0 + k * (i * dt);        /* 当前瞬时频率 */
        phase += 2.0 * M_PI * f * dt;        /* 相位累加 */
        /* 正确归约到 [0, 2π)，对正负都有效 */
        phase -= 2.0 * M_PI * floor(phase / (2.0 * M_PI));
    }

    *out_n = n;
    return y;
}

int16_t* generate_log_chirp_accum(double fs, double T, double f0, double f1,
                                 uint16_t absymax, uint32_t* out_n) {
    uint32_t n = (uint32_t)(fs * T);
    int16_t* y = (int16_t*)malloc(n * sizeof(int16_t));
    if (y == NULL) return NULL;

    double dt = 1.0 / fs;
    double r = f1 / f0;
    double phase = 0.0;

    for (int i = 0; i < n; ++i) {
        y[i] = absymax * cos(phase);

        double t = i * dt;
        double f = f0 * pow(r, t / T);     /* 当前瞬时频率 */
        phase += 2.0 * M_PI * f * dt;      /* 相位累加 */

        phase -= 2.0 * M_PI * floor(phase / (2.0 * M_PI));  /* 归约到 [0, 2π) */
    }

    *out_n = n;
    return y;
}


// 根据进制数值获取字符串
const char *string_from_radix(int radix) {
	switch (radix)
	{
	case 2 :	return "BIN";
	case 8 :	return "OCT";
	case 10:	return "DEC";
	case 16:	return "HEX";
	
	default:	return "UNS";	//it actually is UNS_DEC
		break;
	}
}

typedef struct {
	uint32_t sample_rate;
	uint32_t sample_points;
	int16_t voltage_offset;
    int8_t addr_radix;
    int8_t data_radix;
} WsxHeader;

WsxHeader header;

int main() {
    FILE* fp = fopen("main_gen.wsx", "w");
    if (fp == NULL) {
        perror("打开文件失败");
        return 1;
    }



	//Wave Information Begin
	#define WAVE_FREQUENCY	3000000

	header.sample_rate = 125000000;	//MAX = 125000000, MIN = 488281, and should be (125000000 / n)
	header.sample_points = (125000000 / WAVE_FREQUENCY);	//MAX = 4194304
	header.voltage_offset = 0;		//-512 ~ 511
	header.addr_radix = 10;			// 地址格式：UNS(无符号)/HEX/BIN/DEC
	header.data_radix = 10;			// 数据格式：BIN/HEX/DEC

	int16_t *data;
	
	// data = generate_sawtooth(header.sample_points, -1.0, 1.0, 0x1ff);
	data = generate_sin(header.sample_points, 0, 2.0 * M_PI, 0x1ff);
	// data = generate_sinc(header.sample_points, -5.0, 5.0, 0x1ff);
	// data = generate_chirp_accum(header.sample_rate, 0.000008, 100000, 5000000, 0x1ff, &(header.sample_points));
	// data = generate_log_chirp_accum(header.sample_rate, 0.000008, 100000, 5000000, 0x1ff, &(header.sample_points));

	//Wave Information End



	uint32_t actual_sample_rate = 125000000 / header.sample_rate;
	actual_sample_rate = 125000000 / actual_sample_rate;
	double actual_frequency = ((double)actual_sample_rate) / header.sample_points;

    fprintf(fp, "//Wave Generated From main_gen_wsx.cpp\n");
    fprintf(fp, "//ACTUAL_SAMPLE_RATE = %d;//Hz\n", actual_sample_rate);
    fprintf(fp, "//ACTUAL_FREQUENCY = %.3f;//Hz\n", actual_frequency);
    fprintf(fp, "SAMPLE_RATE\t\t=\t%d;//Hz\n", header.sample_rate);
    fprintf(fp, "SAMPLE_POINTS\t=\t%d;\n", header.sample_points);
    fprintf(fp, "VOLTAGE_OFFSET\t=\t%d;\n", header.voltage_offset);
    fprintf(fp, "ADDRESS_RADIX\t=\t%s;\n", string_from_radix(header.addr_radix));
    fprintf(fp, "DATA_RADIX\t\t=\t%s;\n\n", string_from_radix(header.data_radix));
    fprintf(fp, "DATA\n");
    fprintf(fp, "BEGIN\n");
	for(int i = 0; i < header.sample_points; i++)
	{
		fprintf(fp, "%d\t:\t%d;\n", i, data[i]);
	}
    fprintf(fp, "END\n");
	free(data);

    fclose(fp);
    return 0;
}
