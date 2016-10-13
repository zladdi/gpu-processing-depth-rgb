typedef struct grid_surface_s
{
  int startx, starty, countx, county;
} grid_surface;

void * openni_init(unsigned int *mapX, unsigned int  *mapY, int *xRes, int *yRes);
void openni_getframe (void* mapBuf, unsigned int mapX, unsigned int mapY);
int openni_process (void* mapBuf, unsigned int mapX, unsigned int mapY);
int openni_process_grid (void* mapBuf, unsigned int mapX, unsigned int mapY, int grids, int gridSize, int tolerance, int * dir, int dirCount[4]);
int openni_rgb (void* mapBuf, unsigned int mapX, unsigned int mapY, int gridSize);

int openni_process_grid2 (void* mapBuf, unsigned int mapX, unsigned int mapY, int grids, int gridSize, int tolerance, int * dir, int dirCount[4]);
 
