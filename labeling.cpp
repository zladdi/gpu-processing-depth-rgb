
/* 
https://github.com/surajdubey/Connected-Components-Labeling

It does first pass of Connected Component Labelling and Second Pass
* Equivalency is recorded over here
*
* Refer images attached in e-mail if you find any diificulty during compilation
* Suraj Dubey suraj.dubey@hotmail.com
*
*/

#include <stdio.h>
typedef struct XnRGB24Pixel
 {
     unsigned char nRed;
     unsigned char nGreen;
     unsigned char nBlue;
 } XnRGB24Pixel;

class Labelling
{

	int max_label;
	int * record;
        int i,j,k;
	XnRGB24Pixel * img, *img_orig;
	int width, height;

        Labelling(void *img , int width , int height)
        {
		this->img = ( XnRGB24Pixel*)img;
		this->img_orig = ( XnRGB24Pixel*)img;
		this->width = width;
		this->height = height;
		int max_label = 100;
        }

public:
	char *  labelImage()
        {
		record = new int[max_label + 1]; // Records equivalency
		/* Initialize record array which stores equivalency */
		for(i=0;i<max_label;i++)
		{
			record[i]=i;
		}


            int label = 0; //used for "labelling" purpose
		int test = 0;
		for(i=1;i<width-1;i++)
		{
			for(j=1;j<height-1;j++, img++)
			{
				if(img->nBlue!=0) // value or point exist
				{
				 	int north =  (img - width)->nBlue;//img[i-1][j];
				  	int west =  (img - 1)->nBlue;
					if(north == 0 && west == 0) // none of cells marked
					{
						img->nBlue = ++label;
					}
					else
					if(north !=0 && west != 0) // both cells marked
					{
						if(record[north] == record[west]) // cells with same label
						{
							img->nBlue = record[west];
						}
						else // cells with different label
						{
							/* 
							* lower value among north cell and west cell is assigned to current 									* cell
							* Also Equivalency is recorded here */
							if(north > west) // west is minimum
							{
								img->nBlue = record[west];
								for(k=0;k<=label;k++)
								{
									if(record[k] == record[north])
									{
										record[k] = record[west];
									}
								}
							}
							else // north is min
							{
								img->nBlue = record[north];
								for(k=0;k<=label;k++)
								{
									if(record[k] == record[west])
									{
										record[k] = record[north];
									}
								}
							}
						}
					}
					else // just one cell marked
					{
						if(north>0)
							img->nBlue = north;
						else
							img->nBlue = west;
					} // else ends
				} // if img ends
			} // j for ends
		} // i for ends


			/* Second pass starts here */
	    img = img_orig;
            for(i=1; i<width-1; i++)
                    for(j=1; j<height-1; j++, img++)
                    {
                            if(img->nBlue != 0)
                            {
                                    img->nBlue = record[img->nBlue];
                            }
                    }

            int *ar = maxRegion();
            int maxLabel = ar[0]; // Returns label with maximum count
            /* Prints only values whose value is maxLabel 
            for(int row[] : img)
            {
                    for(int col : row)
                    {
                            if(col == maxLabel)
                            System.out.print(1+" ");
                            else
                            System.out.print("  ");

                    }
                    System.out.println();
            }

            for(i=0;i<width-1;i++)
            {
                for(j=0;j<height-1;j++)
                {
                    if(img[i][j] == maxLabel)
                        img[i][j] = 1;
                    else
                        img[i][j] = 0;
                }
            }
*/
            int total_pixel = height*width;
            printf("Total number of pixels = %d", total_pixel);
            printf("Number of pixels of biggest component = %d", ar[1]);
            printf("Percentage of pixels of biggest component = %d", (ar[1]*100.0/total_pixel));


            return "ok"; //+"/"+total_pixel;
	} // main ends

        int * maxRegion()
	{
		img= img_orig;
	        int *num = new int[max_label];
	        int maxValue=0 , region = 0;
	        for(int i=1;i<width-1;i++)
	                for(int j=1;j<height-1;j++,img++)
	                        num[ img->nBlue ]++;

	        for(int k=1;k<max_label;k++)
	        {
	                if(num[k] > maxValue)
	                {
	                        maxValue = num[k];
	                        region = k;
	                }
	        }

//	        System.out.println("Region with maximum value is "+region);
//	        System.out.println("Corresponding count is "+ maxValue);

 
                int ar[] = {region,maxValue};
		return ar;	
	}
        
}; // class ends




