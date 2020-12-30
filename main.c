#include <stdio.h>
#include <stdlib.h>

/* run this program using the console pauser or add your own getch, system("pause") or input loop */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef long LONG;


#define BM_WORD 0x4D42

typedef struct tagBitmapHeader
{
	//BITMAP HEADER
//	WORD bfType; //Constant = 0x424D or  'BM'
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
	// BITMAP INFOR
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes; //Constant = 1
	WORD biBitCount;
	DWORD  biCompression;
	DWORD  biSizeImage;
	LONG  biXPelsPerMeter;
	LONG  biYPelsPerMeter;
	DWORD  biClrUsed;
	DWORD  biClrImportant;
}BITMAPHEADER;
typedef struct tagRGBQUAD
{
	BYTE rgbBlue;
	BYTE rgbGreen;
	BYTE rgbRed;
	BYTE rgbReserved;
}RGBQUAD;
 
typedef struct tagC3RGB
{
	BYTE red;
	BYTE green;
	BYTE blue;
}C3RGB;

typedef struct tagBufferDims
{
	int width;
	int height;
	int depth;
}BUFFERDIMS;


void printType(const int t);
void ShowBitmapInfo(BITMAPHEADER* pBitmapHeader);
int OpenBitmapFile(const char* szFileName, FILE** ppFile);
int ReadHeaderInfo(FILE* pFile, BITMAPHEADER* pBitmapHeader, int dumpInfo);
int ReadImageData(FILE* pFile, BITMAPHEADER* pBitmapHeader, BYTE** ppBuffer, size_t *bufferSize, BUFFERDIMS* pBufferDims);
int ReadBitmapFile(const char* szFileName, BYTE** ppBuffer, BUFFERDIMS* pBufferDims, int dumpInfo);
int DownScaledImage(BYTE* pImageData, size_t bufferSize, BITMAPHEADER* pBitmapHeader, BUFFERDIMS* pBufferDims);


int main(int argc, char *argv[]) {
	FILE *pFile = NULL;
    BYTE *pImageData = NULL;
    DWORD bOffSize = 0;
    size_t bufferSize = 0;
    
    int i = 0;
    BITMAPHEADER bitMapHeader;
    BUFFERDIMS bufferDims;
    
    if( OpenBitmapFile(argv[1], &pFile) )
    {
    	printf("open file ok\n");	
	    ReadHeaderInfo(pFile, &bitMapHeader, 1 );
		bOffSize = bitMapHeader.bfOffBits;
		printf("off size %d \n", bOffSize);
		ReadImageData(pFile, &bitMapHeader, &pImageData, &bufferSize, &bufferDims);
//		for(i = 0; i < bufferSize; i++)
//		{
//			printf(" %d ", pImageData[i]);
//		}
        DownScaledImage(pImageData, bufferSize, &bitMapHeader, &bufferDims);
		if(pImageData != NULL)
		   free(pImageData);
		fclose(pFile);
    }
	system("pause");
	return 0;
}

int OpenBitmapFile(const char* szFileName, FILE** ppFile)
{
	*ppFile = fopen(szFileName, "rb");
	if (*ppFile == NULL)
	{
		printf(">>>ERROR:\nFailed to open file %s\n", szFileName);
		return 0;
	}
	return 1;
}

int ReadHeaderInfo(FILE* pFile, BITMAPHEADER* pBitmapHeader, int dumpInfo)
{
	WORD bfType;
	fread(&bfType, sizeof(WORD), 1, pFile);
	//if ( BM_WORD != (pBitmapHeader->bfType) )
	if (BM_WORD!=bfType)
	{
		printf(">>>INVALID:\nThis is not a valid bitmap.\n");
		return 0;
	}
 
	fread(pBitmapHeader, sizeof(BITMAPHEADER), 1, pFile);
	if (dumpInfo)
	{
		ShowBitmapInfo(pBitmapHeader);
	}
 
 	return 1;
}
int ReadImageData(FILE* pFile, BITMAPHEADER* pBitmapHeader, BYTE** ppBuffer, size_t *bufferSize, BUFFERDIMS* pBufferDims)
{
	int i = 0;
	pBufferDims->width = pBitmapHeader->biWidth;
	pBufferDims->height = pBitmapHeader->biHeight;
	pBufferDims->depth = (pBitmapHeader->biBitCount) >> 3;
	if (pBufferDims->depth == 1)
	{
		printf("This is a grayscale image\n");
	}
	else if (pBufferDims->depth == 3)
	{
		printf("This is a RGB colored image\n");
	}
	else
	{
		printf(">>>INVALID:\nThis is not a 8/24 bit image.\n");
		return 0;
	}
 
	if (pBufferDims->depth == 1)
	{
		RGBQUAD palette[256];
		int i;
		for (/*int */i = 0; i<pBitmapHeader->biClrUsed; ++i)
		{
			fread(&palette[i], sizeof(RGBQUAD), 1, pFile);
		}
	}
 
//	const int nLineBytes = ((pBitmapHeader->biWidth)* (pBitmapHeader->biBitCount) + 31) / 8;
	const int nLineBytes = ((pBitmapHeader->biWidth)* (pBitmapHeader->biBitCount)) / 8;
	*bufferSize = nLineBytes*(pBitmapHeader->biHeight);
	printf("Trying to allocate %d bytes memory...",*bufferSize);
	*ppBuffer = (BYTE*)malloc(*bufferSize);
	if (*ppBuffer == NULL)
	{
		printf("Failed.\n");
		printf(">>>ERROR:\nFailed to allocate memory.\n");
		return 0;
	}
 
	printf("OK.\nPrepare to load image data...\n");
	fread(*ppBuffer, *bufferSize, 1, pFile);
	printf("Image data loaded.\n");
 
	return 1;
}

int DownScaledImage(BYTE* pImageData, size_t bufferSize, BITMAPHEADER* pBitmapHeader, BUFFERDIMS* pBufferDims)
{
	FILE *pFileWrite = NULL;
	size_t downScaledImageDataSize = 0;
	BYTE* pDownScaledImageData = NULL, *currentLine = NULL, *nextLine = NULL;
	int row = 0, col = 0, index = 0;
	int red = 0, green = 0, blue = 0;
	BITMAPHEADER DownScaledImageHeader;
	
	
	DownScaledImageHeader = *pBitmapHeader;
	if ( pBitmapHeader->biHeight % 2 == 0 )
	   DownScaledImageHeader.biHeight = pBitmapHeader->biHeight / 2;
	else
	   DownScaledImageHeader.biHeight = ( pBitmapHeader->biHeight + 1 ) / 2;
    if ( pBitmapHeader->biWidth % 2 == 0 )
	   DownScaledImageHeader.biWidth = pBitmapHeader->biWidth / 2;
	else
	   DownScaledImageHeader.biWidth = ( pBitmapHeader->biWidth + 1 ) / 2;
	
	downScaledImageDataSize = ( DownScaledImageHeader.biWidth * DownScaledImageHeader.biBitCount ) / 8;
	downScaledImageDataSize = downScaledImageDataSize * DownScaledImageHeader.biHeight;
	pDownScaledImageData = (BYTE* )malloc(downScaledImageDataSize);
	for( row = 0; row < pBitmapHeader->biHeight; row++)
	{
		currentLine = pImageData + row * pBitmapHeader->biWidth * 3;
		if ( row + 1 < pBitmapHeader->biHeight)
		   nextLine = pImageData + ( row + 1)  * pBitmapHeader->biWidth * 3;

		while(col < pBitmapHeader->biWidth * 3 )
		{
			if ( row + 1 < pBitmapHeader->biHeight )
			{
				//red first pixe     econd pixel          third pixel      fourth pixel
				if( col + 3 < pBitmapHeader->biWidth * 3)
			       pDownScaledImageData[index++] = ( currentLine[col] + currentLine[col + 3] +
				        nextLine[col] + nextLine[col + 3] ) / 4;
				else
				   pDownScaledImageData[index++] = ( currentLine[col] + nextLine[col]  ) / 2;
//				//green
//				green = ( currentLine[col + 1] + currentLine[col + 4] +
//				          nextLine[col + 1] + nextLine[col + 4] ) / 4;
//				//blue
//				blue = ( currentLine[col + 2] + currentLine[col + 5] +
//				         nextLine[col + 2] + nextLine[col + 5] ) / 4;
//				col += 6;// offsize two pixel 3(Byte) * 2
			}
			else
			{
				//red first pixe     econd pixel
				if( col + 3 < pBitmapHeader->biWidth * 3)     
			       pDownScaledImageData[index++] = ( currentLine[col] + currentLine[col + 3] ) / 2;
			    else
			       pDownScaledImageData[index++] = currentLine[col] ;
				//green
//				green = ( currentLine[col + 1] + currentLine[col + 4] ) / 2;
//				//blue
//				blue = (currentLine[col + 2] + currentLine[col + 5] ) / 2;
//				col += 6;// offsize two pixel 3(Byte) * 2
			}
			if ( col % 3 ==  2 )
			   col += 3 + 1;
			else
			   col++;
		}
        row++;
	}
//	pFileWrite = fopen("downscaled.bmp", "w+");
//	if( pFileWrite != NULL )
//	{
//		fwrite( &pDownScaledImageData ,downScaledImageDataSize, 1, pFileWrite);
//		fclose(pFileWrite);
//	}
    if(pDownScaledImageData != NULL)
       free(pDownScaledImageData);
	return 0;
}

void ShowBitmapInfo(BITMAPHEADER* pBitmapHeader)
{
	printf("\n------------------- INFORMATION -------------------\n");
	printf("\tFile volume : %d Bytes\n", pBitmapHeader->bfSize);
	printf("\tContent volume : %d Bytes\n", pBitmapHeader->biSizeImage);
	printf("\tImage size : %d*%d (pixel)\n", pBitmapHeader->biWidth, pBitmapHeader->biHeight);
	printf("\tNumber of color used: %d\n", pBitmapHeader->biClrUsed);
	printf("\tNumber of bit per pixel: %d\n", pBitmapHeader->biBitCount);
//	printf("\t")
//	printf("\tCompress type: %d\n", pBitmapHeader->biCompression);
	printType(pBitmapHeader->biCompression);
	printf("----------------------------------------------------\n");
}
void printType(const int t)
{
	printf("\tCompress type: ");
	switch (t)
	{
	case 0:
		printf("UNCOMPRESSED");
		break;
	case 1:
		printf("BI_RLE8");
		break;
	case 2:
		printf("BI_RLE4");
		break;
	default:
		break;
	}
	printf("\n");
}
