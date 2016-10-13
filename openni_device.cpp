//---------------------------------------------------------------------------
// Includes
//---------------------------------------------------------------------------
#include <XnOS.h>
#include <math.h>

#include <XnCppWrapper.h>
#include "openni_device.h"

#ifdef USE_SHADERS
#include "openni_gpu_histogram.h"
#endif

using namespace xn;

//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define SAMPLE_XML_PATH "SamplesConfig.xml"


#define DISPLAY_MODE_OVERLAY		1
#define DISPLAY_MODE_DEPTH		2
#define DISPLAY_MODE_IMAGE		3
#define DEFAULT_DISPLAY_MODE	DISPLAY_MODE_DEPTH

#define MAX_DEPTH 10000

//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------

static const char* OPENNI_STR = "openni";

float g_pDepthHist[MAX_DEPTH];

//unsigned int g_nViewState = DEFAULT_DISPLAY_MODE;
//unsigned int g_nViewState = DISPLAY_MODE_OVERLAY;
unsigned int g_nViewState = DISPLAY_MODE_IMAGE;

extern Context g_Context;
ScriptNode g_scriptNode1;
extern DepthGenerator g_Depth;
extern DepthMetaData g_DepthMD;
extern ImageGenerator g_Image;
extern ImageMetaData g_ImageMD;

#ifdef USE_SHADERS
hist_state_t g_histState;
#endif

XnStatus rc;
int cellcounts;
int nMaxValue = 225;

   
 


//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

int openni_rgb (void* pixels, unsigned int mapX, unsigned int mapY, int divs) // draws on the pixels through pTex variable
{
	int nx = g_ImageMD.XRes() / divs; // width of a cell
	int ny = g_ImageMD.YRes() / divs; 
	int cellx, celly;
	long int cellcounts[divs][divs];

        XnRGB24Pixel* pix = (XnRGB24Pixel*)pixels;
	g_Image.GetMetaData(g_ImageMD);

	// Calculate the accumulative histogram (the yellow display...)
//	xnOSMemSet(g_pDepthHist, 0, MAX_DEPTH *sizeof(float));


	// get RGB image
	const XnRGB24Pixel* pImageRow 	= g_ImageMD.RGB24Data();
	XnRGB24Pixel* pTexRow 		= pix + g_ImageMD.YOffset() * mapX*3;
	XnRGB24Pixel* pix_offset	= pix;	
	for (XnUInt y = 0; y < g_ImageMD.YRes(); ++y)
		{
			const XnRGB24Pixel* pImage = pImageRow;
			pix_offset = pTexRow + g_ImageMD.XOffset();

			memcpy(pix_offset, pImage, sizeof(XnRGB24Pixel)*g_ImageMD.XRes());
			pImage+= g_ImageMD.XRes();
			pix_offset += g_ImageMD.XRes();

			pImageRow += g_ImageMD.XRes();
			pTexRow += mapX;
		}



//			(pTex)->nRed = pTex->nRed/4; (pTex)->nBlue = 0;
//			(pTex)->nRed = nHistValue;(pTex)->nGreen = nHistValue;(pTex)->nBlue = 0;

	return celly;
}






int openni_process_grid (void* pixels, unsigned int mapX, unsigned int mapY, int gridFlag, int divs, int tolerance, int *direction, int dirCount[4]) // draws on the pixels through pTex variable
{
	const XnDepthPixel* pDepth = g_DepthMD.Data();
	int nx = g_DepthMD.XRes() / divs; // width of a cell
	int ny = g_DepthMD.YRes() / divs; 
	const XnDepthPixel* pDepthRow = g_DepthMD.Data();
	unsigned int cellcounts[divs][divs];
	int cMax = 225;
	float scale = 0.0225; // cMax/MAX_DEPTH = 225 / 10000
	unsigned int temp;
	int cellx, celly;
	long int pixSum=0;

        XnRGB24Pixel* pix = (XnRGB24Pixel*)pixels;
	g_Depth.GetMetaData(g_DepthMD);
	XnRGB24Pixel* pTexRow = pix + g_DepthMD.YOffset() * mapX*3;
	memset(cellcounts, 0, divs*divs*sizeof(unsigned int));
		

	// draw grid and count pixels that are in cells that are within the threshold distance
	for (XnUInt y = 0; y < g_DepthMD.YRes(); ++y ) // loop counts pixels
	{
		const XnDepthPixel* pDepth = pDepthRow;
		XnRGB24Pixel* pTex = pTexRow + g_DepthMD.XOffset();

		celly = y / ny;

		for (XnUInt x = 0; x < g_DepthMD.XRes()-divs; ++x, pDepth++, ++pTex)
		{
			temp = scale*(*pDepth);  
			pTex->nRed   = 	pTex->nGreen = temp;
			pTex->nBlue = 0;
			
			cellx = x / nx;
			cellcounts[cellx][celly]+= temp;
			pixSum+=temp;

		if (gridFlag==1) {
			if (x % nx == 0)
				for (int j=0; j<3; j++) {
					(pTex+j)->nRed = (pTex+j)->nGreen = cMax;(pTex+j)->nBlue = 0; }

			if (y % ny == 0)
				for (int j=0; j<3; j++) {
					(pTex+j)->nRed = (pTex+j)->nGreen = cMax;(pTex+j)->nBlue = 0;}
		}

		}
		pDepthRow += g_DepthMD.XRes();
		pTexRow += mapX;

	} // end loop count pixels

if (gridFlag==1) { // color grids with red or green depending on the cell neighbour difference and tolerance

        pix = (XnRGB24Pixel*)pixels;
	for (XnUInt y = 0; y < g_DepthMD.YRes(); ++y) // loop colors pixels
	{
		celly = y / ny;
		for (XnUInt x = 0; x < g_DepthMD.XRes(); x++, pix++)
		{
			cellx = x / nx;

			if (celly > 0 && celly < divs - 3) {
				int difCurrent =  abs(cellcounts[cellx][celly] - cellcounts[cellx][celly+1]);
				if (difCurrent < tolerance)
					(pix)->nRed = pix->nRed/4;
				else {
					(pix)->nGreen = pix->nGreen/4;
					}
					 
			}
		}
//		printf("%u ", cellcounts[cellx][celly]);
	}
//	printf("\n---------eof \n ");
	}

	// find out the direction of the planes
	int upCount,downCount, leftCount, rightCount, max; 
	upCount=downCount=leftCount=rightCount=0;

	for (int x=0; x<divs-1; x++)
		for (int y=0; y<divs-1; y++) {
			if (cellcounts[x][y] > cellcounts[x][y+1]) downCount++;
			if (cellcounts[x][y] < cellcounts[x][y+1]) upCount++;
			if (cellcounts[x][y] > cellcounts[x+1][y]) leftCount++;
			if (cellcounts[x][y] < cellcounts[x+1][y]) rightCount++;

		}
	max = downCount; *direction = 0;
	if (upCount > max) {max = upCount; *direction=1;}
	if (leftCount > max) {max = leftCount; *direction=2;}
	if (rightCount > max) {max = rightCount; *direction=3;}
	dirCount[0] = downCount;
	dirCount[1] = upCount;
	dirCount[2] = leftCount;
	dirCount[3] = rightCount;

	return pixSum/(mapX*mapY);
}

/*

// this processes a depth image if one pixel is one byte
int openni_process (void* pixels, unsigned int mapX, unsigned int mapY) // draws on the pixels through pTex variable
{
	unsigned int avg = 0;  
	int n = 1;

	unsigned char* pix = (unsigned char*)pixels;

	// Already reading frames in the ReadFrame ONI function
	g_Depth.GetMetaData(g_DepthMD);

	const XnDepthPixel* pDepth = g_DepthMD.Data();
	int x, y;

	const XnDepthPixel * pDepthRow = g_DepthMD.Data();
	int nHistValue = 255;



	for (XnUInt y = 0; y < g_DepthMD.YRes(); ++y)
	{
		const  XnDepthPixel * pDepth = pDepthRow;

		for (XnUInt x = 0; x < g_DepthMD.XRes(); ++x, ++pDepth, ++pix)
		{
			*pix  = 225 *(*pDepth)/MAX_DEPTH;
			//pTex->nRed   = 225 *(*pDepth)/MAX_DEPTH;
			avg += *pix;
			n++;

		}
                pDepthRow += g_DepthMD.XRes();
//                pTexRow += mapX;
	}

	return avg/n;

}

*/
// this processes a depth image if one pixel has 3 components R G B 
int openni_process (void* pixels, unsigned int mapX, unsigned int mapY) // draws on the pixels through pTex variable
{
	unsigned int avg = 0;  
	int n = 1;

	XnRGB24Pixel* pix = (XnRGB24Pixel*)pixels;

	// Already reading frames in the ReadFrame ONI function
	g_Depth.GetMetaData(g_DepthMD);

	const XnDepthPixel* pDepth = g_DepthMD.Data();

	xnOSMemSet(g_pDepthHist, 0, MAX_DEPTH *sizeof(float));

	int x, y;

	const XnDepthPixel* pDepthRow = g_DepthMD.Data();
	XnRGB24Pixel* pTexRow = pix + g_DepthMD.YOffset() * mapX;
	int nHistValue = 255;



	for (XnUInt y = 0; y < g_DepthMD.YRes(); ++y)
	{
		const XnDepthPixel* pDepth = pDepthRow;
		XnRGB24Pixel* pTex = pTexRow + g_DepthMD.XOffset();

		for (XnUInt x = 0; x < g_DepthMD.XRes(); ++x, ++pDepth, ++pTex)
		{
			pTex->nRed   = 225 *(*pDepth)/MAX_DEPTH;
			pTex->nGreen = 225 *(*pDepth)/MAX_DEPTH;

			avg += pTex->nRed;
			n++;

		}
                pDepthRow += g_DepthMD.XRes();
                pTexRow += mapX;
	}

	return avg/n;

}


int openni_process_grid2 (void* pixels, unsigned int mapX, unsigned int mapY, int gridFlag, int divs, int tolerance, int *direction, int dirCount[4]) // draws on the pixels through pTex variable
{
	const XnDepthPixel* pDepth = g_DepthMD.Data();
	int nx = g_DepthMD.XRes() / divs; // width of a cell
	int ny = g_DepthMD.YRes() / divs; 
	const XnDepthPixel* pDepthRow = g_DepthMD.Data();
	unsigned int cellcounts[divs][divs];
	grid_surface surf[divs][divs];
	int cMax = 225;
	float scale = 0.0225; // cMax/MAX_DEPTH = 225 / 10000
	unsigned int temp;
	int cellx, celly;
	long int pixSum=0;

        XnRGB24Pixel* pix = (XnRGB24Pixel*)pixels;
	g_Depth.GetMetaData(g_DepthMD);
	XnRGB24Pixel* pTexRow = pix + g_DepthMD.YOffset() * mapX*3;
	memset(cellcounts, 0, divs*divs*sizeof(unsigned int));
		

	// draw grid and count pixels that are in cells that are within the threshold distance
	for (XnUInt y = 0; y < g_DepthMD.YRes(); ++y ) // loop counts pixels
	{
		const XnDepthPixel* pDepth = pDepthRow;
		XnRGB24Pixel* pTex = pTexRow + g_DepthMD.XOffset();

		celly = y / ny;

		for (XnUInt x = 0; x < g_DepthMD.XRes()-divs; ++x, pDepth++, ++pTex)
		{
			temp = scale*(*pDepth);  
			pTex->nRed   = 	pTex->nGreen = temp;
			pTex->nBlue = 0;
			
			cellx = x / nx;
			cellcounts[cellx][celly]+= temp;
			pixSum+=temp;

		if (gridFlag==1) {
			if (x % nx == 0)
				for (int j=0; j<3; j++) {
					(pTex+j)->nRed = (pTex+j)->nGreen = cMax;(pTex+j)->nBlue = 0; }

			if (y % ny == 0)
				for (int j=0; j<3; j++) {
					(pTex+j)->nRed = (pTex+j)->nGreen = cMax;(pTex+j)->nBlue = 0;}
		}

		}
		pDepthRow += g_DepthMD.XRes();
		pTexRow += mapX;

	} // end loop count pixels

if (gridFlag==1) { // color grids with red or green depending on the cell neighbour difference and tolerance

        pix = (XnRGB24Pixel*)pixels;
	for (XnUInt y = 0; y < g_DepthMD.YRes(); ++y) // loop colors pixels
	{
		celly = y / ny;
		for (XnUInt x = 0; x < g_DepthMD.XRes(); x++, pix++)
		{
			cellx = x / nx;
			int diffCurrentX = abs(cellcounts[cellx][celly] - cellcounts[cellx-1][celly]);
			int diffCurrentY = abs(cellcounts[cellx][celly] - cellcounts[cellx][celly-1]);
			grid_surface* surfLeft = &surf[cellx-1][celly];
			grid_surface* surfUp = &surf[cellx][celly-1];
			grid_surface* curSurf = &surf[cellx][celly];
			grid_surface* maxSurf;
			// expand surface in x or y direction (but not on diagonal)
			if (diffCurrentX < tolerance)
			{
				// check if also should expand on y axis
				if (diffCurrentY < tolerance)
				{
					if ((surfLeft->startx != surfUp->startx) || 
						(surfLeft->starty != surfUp->starty))
					{
					}
				}
				else 
				{
					curSurf->startx = surfLeft->startx;
					curSurf->starty = surfLeft->starty;
					curSurf->countx = surfLeft->countx + 1;
				}
			} 
			else if (diffCurrentY < tolerance)
			{
				curSurf->startx = surfUp->startx;
				curSurf->starty = surfUp->starty;
				curSurf->county = surfUp->county + 1;
			}
	if (celly > 0 && celly < divs - 3) {
		
				int difCurrent =  abs(cellcounts[cellx][celly] - cellcounts[cellx][celly+1]);
				if (difCurrent < tolerance)
					(pix)->nRed = pix->nRed/4;
				else {
					(pix)->nGreen = pix->nGreen/4;
					}
					 
			}
		}
//		printf("%u ", cellcounts[cellx][celly]);
	}
//	printf("\n---------eof \n ");
	}

	// find out the direction of the planes
	int upCount,downCount, leftCount, rightCount, max; 
	upCount=downCount=leftCount=rightCount=0;

	for (int x=0; x<divs-1; x++)
		for (int y=0; y<divs-1; y++) {
			if (cellcounts[x][y] > cellcounts[x][y+1]) downCount++;
			if (cellcounts[x][y] < cellcounts[x][y+1]) upCount++;
			if (cellcounts[x][y] > cellcounts[x+1][y]) leftCount++;
			if (cellcounts[x][y] < cellcounts[x+1][y]) rightCount++;

		}
	max = downCount; *direction = 0;
	if (upCount > max) {max = upCount; *direction=1;}
	if (leftCount > max) {max = leftCount; *direction=2;}
	if (rightCount > max) {max = rightCount; *direction=3;}
	dirCount[0] = downCount;
	dirCount[1] = upCount;
	dirCount[2] = leftCount;
	dirCount[3] = rightCount;

	return pixSum/(mapX*mapY);
}


