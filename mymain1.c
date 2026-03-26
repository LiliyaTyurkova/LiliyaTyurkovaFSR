#include <stdio.h>
#include <stdlib.h> 
#include <strings.h>
#include <math.h> 
#include "lodepng.h" 

//ввод
unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height) 
{
  unsigned char* image = NULL; 
  int error = lodepng_decode32_file(&image, width, height, filename);
  if(error != 0) {
    printf("error %u: %s\n", error, lodepng_error_text(error)); 
  }
  return (image);
}

//вывод
void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
    unsigned char* png;
    size_t pngsize;
    int error = lodepng_encode32(&png, &pngsize, image, width, height);
    if(error == 0) {
        lodepng_save_file(png, pngsize, filename);
    } else { 
        printf("error %u: %s\n", error, lodepng_error_text(error));
    }
    free(png);
}

//контраст изображения
void contrast(unsigned char *col, int bw_size)
{ 
    int i;
    for(i = 0; i < bw_size; i++)
    {
        if(col[i] < 60) col[i] = 0; 
        if(col[i] > 130) col[i] = 255;
    } 
} 

// Гауссово размытие
void Gauss_blur(unsigned char *col, unsigned char *blr_pic, int width, int height)
{ 
    int i, j; 
    for(i = 1; i < height-1; i++) 
        for(j = 1; j < width-1; j++)
        { 
            blr_pic[width*i+j] = 0.084*col[width*i+j] + 0.084*col[width*(i+1)+j] + 0.084*col[width*(i-1)+j]; 
            blr_pic[width*i+j] = blr_pic[width*i+j] + 0.084*col[width*i+(j+1)] + 0.084*col[width*i+(j-1)]; 
            blr_pic[width*i+j] = blr_pic[width*i+j] + 0.063*col[width*(i+1)+(j+1)] + 0.063*col[width*(i+1)+(j-1)]; 
            blr_pic[width*i+j] = blr_pic[width*i+j] + 0.063*col[width*(i-1)+(j+1)] + 0.063*col[width*(i-1)+(j-1)]; 
        } 
   return; 
} 

void color(unsigned char *blr_pic, unsigned char *res, int size)
{ 
    int i;
    for(i = 0; i < size; i++) 
    { 
        res[i*4]=45+blr_pic[i]+0.5*(i>0 ? blr_pic[i-1] : 0); 
        res[i*4+1]=60+blr_pic[i]; 
        res[i*4+2]=162+blr_pic[i]; 
        res[i*4+3]=255; 
    } 
    return;
}

void to_adj_mat(unsigned char* gray, int* vertex, int size, int maybe_ship)
{
    int i;
    for(i = 0;i < size;i++)
        vertex[i] = gray[i] > maybe_ship ? 1 : 0;
    return;
}

//обход в ширину
void bfs(int* vertex, int* used, int width, int height, int start)
{
    int max = width*height;
    int* queue = (int*)malloc(max*sizeof(int));
    int head=0, tail=0;
    queue[tail++] = start;
    used[start] = 1;
    int dx[4] = {-1,1,0,0}; 
    int dy[4] = {0,0,-1,1}; 
    int v, x, y, nx, ny, k, ni;
    while(head < tail) {
        v = queue[head++];
        x = v % width;
        y = v / width;
        for(k = 0; k < 4; k++) {
            nx = x + dx[k];
            ny = y + dy[k];
            if(nx >= 0 && nx < width && ny >= 0 && ny < height) {
                ni = ny*width + nx;
                if(vertex[ni] == 1 && used[ni] == 0) {
                    used[ni] = 1;
                    queue[tail++] = ni;
                }
            }
        }
    }
    free(queue);
    return;
}

// считаю компоненты связности == танкеры 
int count_components(int* vertex, int width, int height)
{
    int size = width*height;
    int* used = (int*)calloc(size,sizeof(int));
    int count = 0;
    int i;
    for(i = 0; i < size; i++) {
        if(vertex[i] == 1 && used[i] == 0) {
            bfs(vertex,used,width,height,i);
            count++;
        }
    }
    free(used);
    return count;
}

int main() 
{ 
    const char* filename = "skull.png"; 
    unsigned int width, height;
    int size;
    int bw_size;
    int i;
    unsigned char* picture = load_png(filename, &width, &height); 
    if (!picture) return -1;
    size = width * height;

    unsigned char* gray = (unsigned char*)malloc(size);
    int* vertex = (int*)malloc(size * sizeof(int));

    // перевод изображения в ЧБ
    for(i = 0; i < size; i++) gray[i] = (picture[4*i]+picture[4*i+1]+picture[4*i+2])/3;

    // создаю матрицу смежности
    to_adj_mat(gray, vertex, size, 118);

    // считаю компоненты == танкеры
    int ships = count_components(vertex, width, height);
    printf("Ships count: %d\n", ships);

    bw_size = size;
    int rgba_size = width * height * 4;

    unsigned char* bw_pic = (unsigned char*)malloc(bw_size);
    unsigned char* blr_pic = (unsigned char*)malloc(bw_size);
    unsigned char* finish = (unsigned char*)malloc(rgba_size);

    for(i = 0; i < bw_size; i++) bw_pic[i] = gray[i];

    contrast(bw_pic, bw_size);

    for(i = 0; i < bw_size; i++) finish[4*i] = finish[4*i+1] = finish[4*i+2] = bw_pic[i], finish[4*i+3] = 255;
        
    // картинка с выкрученным контрастом
    write_png("contrast.png", finish, width, height);

    Gauss_blur(bw_pic, blr_pic, width, height);

    for(i = 0; i < bw_size; i++) finish[4*i] = finish[4*i+1] = finish[4*i+2] = blr_pic[i], finish[4*i+3] = 255;

    // размытая картинка
    write_png("gauss.png", finish, width, height);

    // итоговая картинка
    color(blr_pic, finish, bw_size);
    write_png("picture_out.png", finish, width, height);

    // чищу память
    free(gray); 
    free(vertex);
    free(bw_pic); 
    free(blr_pic);
    free(finish);
    free(picture);
    return 0; 
}