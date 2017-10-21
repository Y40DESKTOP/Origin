#include <stdio.h>
#include <stdlib.h>

#include "mathlib.h"                // DSP 数学函数库
#include "gray_bmp_rotater.h"
#include "log.h"

#define max(a,b) (a > b ? a : b)
#define findmax(a,b,c,d) (max(a,b) > max(c,d) ? max(a,b) : max(c,d))

#define min(a,b) (a < b ? a : b)
#define findmin(a,b,c,d) (min(a,b) < min(c,d) ? min(a,b) : min(c,d))

typedef struct tagRGBQUAD
{
	unsigned char rgbBlue;
	unsigned char rgbGreen;
	unsigned char rgbRed;
	unsigned char rgbReserved;
}RGBQUAD;

typedef struct tagBITMAPFILEHEADER
{
	unsigned short int		bfType;         // 位图文件的类型，必须为 BM
	unsigned int			bfSize;       	// 文件大小，以字节为单位
	unsigned short int		bfReserverd1; 	// 位图文件保留字，必须为0
	unsigned short int		bfReserverd2; 	// 位图文件保留字，必须为0
	unsigned int 			bfbfOffBits;  	// 位图文件头到数据的偏移量，以字节为单位
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
	int 		biSize;					    // 该结构大小，字节为单位
	int 		biWidth;				    // 图形宽度以象素为单位
	int 		biHeight;				    // 图形高度以象素为单位
	short int 	biPlanes;				    // 目标设备的级别，必须为1
	short int 	biBitcount;				    // 颜色深度，每个象素所需要的位数
	int 		biCompression;			    // 位图的压缩类型
	int 		biSizeImage;			    // 位图的大小，以字节为单位
	int 		biXPelsPermeter;		    // 位图水平分辨率，每米像素数
	int 		biYPelsPermeter;		    // 位图垂直分辨率，每米像素数
	int 		biClrUsed;				    // 位图实际使用的颜色表中的颜色数
	int 		biClrImportant;			    // 位图显示过程中重要的颜色数
} __attribute__((packed)) BITMAPINFOHEADER;

typedef struct
{
	BITMAPFILEHEADER file;                  // 文件信息区
	BITMAPINFOHEADER info;                  // 图象信息区
	RGBQUAD *pColorTable;                   // 调色板
	unsigned char *imgBuf;					// 位图数据
}bmp;

/****************************************************************************/
/*                                                                          */
/*              图像旋转                                                    */
/*                                                                          */
/****************************************************************************/

static bmp Rotate(bmp *m, float fAngle)
{
	int i,j;
	bmp newm;
	int bmpWidth;		// 图像的宽
	int bmpHeight;		// 图像的高
	int biBitCount;		// 图像类型，每像素位数
	int lineByte;
	int newBmpWidth; 	// 新图像的宽
	int newBmpHeight; 	// 新图像的高
	int newLineByte;

	// 获取图像宽、高、每像素所占位数等信息
	bmpWidth = m->info.biWidth;
	bmpHeight = m->info.biHeight;
	biBitCount = m->info.biBitcount;

	// 计算图像每行像素所占的字节数（必须是4的倍数）
	lineByte=(bmpWidth*biBitCount/8+3)/4*4;

	// 图像旋转参数
	float cosAngle;   // fAngle 的 cos 值
	float sinAngle;   // fAngle 的 sin 值
	float f1;         // 中间值
	float f2;         // 中间值
	int x1, x2, x3, x4;
	int y1, y2, y3, y4;
	int xmin, xmax, ymin, ymax;
	int intCapX, intCapY;
	int oldX0;
	int oldY0;
	int newX0;
	int newY0;

	// 原图像的中心坐标
	oldX0 = (bmpWidth - 1) / 2;
	oldY0  = (bmpHeight - 1) / 2;

	// 下面假设原图像与新图像的中心坐标都为（oldX0，oldY0）
	cosAngle = cossp(fAngle);   // fAngle的 cos 值
	sinAngle = sinsp(fAngle);   // fAngle的 sin 值

	f1 = oldX0 * (1 - cosAngle) + oldY0 * sinAngle;
	f2 = -oldX0 * sinAngle + oldY0 * (1-cosAngle);

	// 图像旋转后四个角的坐标（相对于原图像的坐标系）
	x1 = f1;
	y1 = f2;
	x2 = bmpWidth * cosAngle + f1;
	y2 = bmpWidth * sinAngle + f2;
	x3 = -bmpHeight * sinAngle + f1;
	y3 = bmpHeight * cosAngle + f2;
	x4 = bmpWidth * cosAngle - bmpHeight * sinAngle + f1;
	y4 = bmpWidth * sinAngle + bmpHeight * cosAngle + f2;

	xmax = findmax(x1, x2, x3, x4);
	xmin = findmin(x1, x2, x3, x4);

	ymax = findmax(y1, y2, y3, y4);
	ymin = findmin(y1, y2, y3, y4);

	// 新图像的宽高
	newBmpWidth = xmax - xmin;
	newBmpHeight = ymax - ymin;
	newLineByte = (newBmpWidth * biBitCount / 8 + 3 ) / 4 * 4;
	// 申请位图数据区
    unsigned char *newBmpBuf = (unsigned char *)malloc(newLineByte * newBmpHeight);

	// 新图像的中心坐标（相对新图像的左上角为原点的坐标系）
	newX0 = (newBmpWidth - 1) / 2;
	newY0  = (newBmpHeight - 1) / 2;

	// 计算旋转参数（用于从新图像的点找到原图像的点）
	f1 = -newX0 * cosAngle - newY0*sinAngle + oldX0;
	f2 = newX0 * sinAngle - newY0*cosAngle  + oldY0;

	// 进行图像旋转，重新赋值
	for(i = 0; i < newBmpHeight; i++)
	{
		for(j = 0; j < newBmpWidth; j++)
		{
			// 新图像中（i,j）对应于原图像（intCapX，intCapY）
			intCapX = (int)(j*cosAngle + i*sinAngle + f1 + 0.5);
			intCapY = (int)(i*cosAngle - j*sinAngle + f2 + 0.5);

			// 判断是否在原图范围内
			if((intCapX >= 0) && (intCapX < bmpWidth) && (intCapY >= 0) && (intCapY < bmpHeight))
			{
				// 传送亮度信号
				*(unsigned char *)(newBmpBuf + (i)*newLineByte + (j)) = \
						(*(unsigned char *)(m->imgBuf + intCapY * lineByte + intCapX));
			}
			else
			{
				*(unsigned char *)(newBmpBuf + (i)*newLineByte + (j)) = 0xFF;
			}

		}
	}

	newm.file.bfType = 0x4d42;							// 类型
	newm.file.bfSize = 54 + 246 *4 +
						newLineByte * newBmpHeight;		// 文件长度
	newm.file.bfReserverd1 = 0;							// 保留字 1
	newm.file.bfReserverd2 = 0;							// 保留字 2
	newm.file.bfbfOffBits = 54 + 256 * 4;				// 偏移量
	newm.info.biSize = 40;								// 此结构大小
	newm.info.biWidth = newBmpWidth;					// 位图的宽度
	newm.info.biHeight = newBmpHeight;					// 位图的高度
	newm.info.biPlanes = 1;								// 目标设备位图数
	newm.info.biBitcount = 8;							// 颜色深度
	newm.info.biCompression = 0;						// 位图压缩类型
	newm.info.biSizeImage = newLineByte * newBmpHeight;	// 位图大小
	newm.info.biXPelsPermeter = newBmpWidth;			// 位图水平分辨率
	newm.info.biYPelsPermeter = newBmpHeight;			// 位图垂直分辨率
	newm.info.biClrUsed = 256;							// 位图实际使用颜色数
	newm.info.biClrImportant = 256;						// 位图显示中比较重要颜色数

	// 写入调色板
    //unsigned char newpColorTable[256*4];
    unsigned char *newpColorTable = (unsigned char *)malloc(256*4);
	memcpy((void *)(newpColorTable), (void *)(m->pColorTable), 256 * 4);
	newm.pColorTable = (RGBQUAD*)newpColorTable;

	// 写入位图数据
	newm.imgBuf = newBmpBuf;

	return newm;
}

struct _GrayBmpRotater {
    uint8_t hold;
};

static bmp *gray_bmp_rotater_extract(GrayBmpRotater *rotater, uint8_t *input) {
	bmp m;			// 定义一个位图结构
	unsigned char *BmpBuf;
	int i;
	int bmpWidth;	// 图像的宽
	int bmpHeight;	// 图像的高
	int biBitCount;	// 图像类型，每像素位数
	int lineByte;

    m.file = *(BITMAPFILEHEADER *)input;
    m.info = *(BITMAPINFOHEADER *)(input + sizeof(BITMAPFILEHEADER));

	// 获取图像宽、高、每像素所占位数等信息
	bmpWidth = m.info.biWidth;
	bmpHeight = m.info.biHeight;
	biBitCount = m.info.biBitcount;

	// 定义变量，计算图像每行像素所占的字节数（必须是4的倍数）
	lineByte = (bmpWidth*biBitCount/8+3)/4*4;

	// 灰度图像有颜色表，且颜色表表项为256
	if(biBitCount == 8)
	{
		// 分配内存空间
		m.pColorTable = (RGBQUAD *)malloc(256*4);
		// 申请颜色表所需要的空间，读颜色表进内存
        memcpy(m.pColorTable, input + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), 256 * 4);
	}

	// 分配内存空间
	m.imgBuf = (unsigned char *)malloc(m.info.biSizeImage);
	BmpBuf = (unsigned char *)malloc(m.info.biSizeImage);

	// 读位图数据
    memcpy(BmpBuf, input + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * 4, lineByte * bmpHeight);

	// 将读到的数据垂直镜像翻转
	for(i = 0; i < bmpHeight; i++)
		memcpy((void *)(m.imgBuf + lineByte * i), (void *)(BmpBuf + lineByte * (bmpHeight - i - 1)), lineByte);
    free(BmpBuf);

	bmp *new_m = (bmp*)calloc(1, sizeof(bmp));			// 定义一个位图结构
    *new_m = m;
	return new_m;
}

static void gray_bmp_rotater_write(GrayBmpRotater *rotater, bmp *m, uint8_t *output) {
	unsigned char *BmpBuf;
	int i;
	int bmpWidth;
	int bmpHeight;
	int biBitCount;
	RGBQUAD *pColorTable;

	bmpWidth = m->info.biWidth;
	bmpHeight = m->info.biHeight;
	biBitCount = m->info.biBitcount;
	pColorTable = m->pColorTable;

	// 如果位图数据指针为0，则没有数据传入，函数返回
	if(! m->imgBuf)
		return;

	// 待存储图像数据每行字节数为4的倍数
	int lineByte = (bmpWidth * biBitCount / 8 + 3) / 4 * 4;

    *(BITMAPFILEHEADER *)output = m->file;
    *(BITMAPINFOHEADER *)(output + sizeof(BITMAPFILEHEADER)) = m->info;

	// 如果灰度图像，有颜色表，写入文件
	if(biBitCount==8)
        memcpy(output + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER), pColorTable, 256 * 4);

	// 分配数据缓冲
	BmpBuf = (unsigned char *)malloc(m->info.biSizeImage);
	// 将位图数据垂直镜像翻转再写入
	for(i = 0; i < bmpHeight; i++)
		memcpy((void *)(BmpBuf + lineByte * i),
				(void *)(m->imgBuf + lineByte * (bmpHeight - i - 1)), lineByte);

	// 写位图数据进文件
    memcpy(output + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 256 * 4, BmpBuf, lineByte * bmpHeight);
    free(BmpBuf);
}

GrayBmpRotater *gray_bmp_rotater_new() {
    GrayBmpRotater *rotater = (GrayBmpRotater *)calloc(1, sizeof(GrayBmpRotater));
    return rotater;
}

void gray_bmp_rotater_destroy(GrayBmpRotater *rotater) {
    if (!rotater)
        return;

    free(rotater);
}

bool gray_bmp_rotater_process(GrayBmpRotater *rotater, uint8_t angle, uint8_t *input, uint8_t *output) {
    bmp out;
    bmp *bmp = gray_bmp_rotater_extract(rotater, input);
    out = Rotate(bmp, PI);
    gray_bmp_rotater_write(rotater, &out, output);

    if (bmp->pColorTable)
        free(bmp->pColorTable);
    free(bmp->imgBuf);
    free(bmp);
    free(out.pColorTable);
    free(out.imgBuf);

    return true;
}
