#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h> 
#include "lodepng.h"
#include "lodepng.c"
/*Здравствуйте!*/
/* Сначала объявляем структуры:
1. для работы с изображением;
2. для работы с элементами DSU;
3. для самой DSU;*/

typedef struct{
    int width;
    int height;
    unsigned char* data; // массив пикселей изображения (RGBA)
} Image;

typedef struct{
    int parent;
    int rank;
} DSUElement;

typedef struct{
    DSUElement* elements;
    int size;
} DSU;

/*Далее описываются все функции*/

// Функция для загрузки изображения
Image load_png(const char* filename){
    Image img = {0};
    unsigned error = lodepng_decode32_file(&img.data, &img.width, &img.height, filename);
    if (error) {
        printf("Error %u: %s\n", error, lodepng_error_text(error));
    }
    return img;
}

// Функция для освобождения памяти изображения
void free_image(Image* img){
    if (img && img->data) {
        free(img->data);
        img->data = NULL;
    }
}

// Функция для копирования изображения
Image copy_image(const Image* src){
    Image dst = {0};
    dst.width = src->width;
    dst.height = src->height;
    size_t size = src->width * src->height * 4;
    dst.data = (unsigned char*)malloc(size);
    if (dst.data) {
        memcpy(dst.data, src->data, size);
    }
    return dst;
}

// Функция для записи обработанного изображения
void write_png(const char* filename, const Image* img){
    if (!img || !img->data) return;
    unsigned error = lodepng_encode32_file(filename, img->data, img->width, img->height);
    if (error) {
        printf("Error %u: %s\n", error, lodepng_error_text(error));
    }
}

// Инициализация DSU
DSU* create_dsu(int size){
    DSU* dsu = (DSU*)malloc(sizeof(DSU));
    if (!dsu) return NULL;
    
    dsu->elements = (DSUElement*)malloc(size * sizeof(DSUElement));
    if (!dsu->elements) {
        free(dsu);
        return NULL;
    }
    
    dsu->size = size;
    for (int i = 0; i < size; i++) {
        dsu->elements[i].parent = i;
        dsu->elements[i].rank = 0;
    }
    
    return dsu;
}

// Найти представителя элемента
int dsu_find(DSU* dsu, int x){
    if (x < 0 || x >= dsu->size) return -1;
    if (dsu->elements[x].parent != x) {
        dsu->elements[x].parent = dsu_find(dsu, dsu->elements[x].parent);
    }
    return dsu->elements[x].parent;
}

// Объединить два множества
void dsu_union(DSU* dsu, int x, int y){
    int xroot = dsu_find(dsu, x);
    int yroot = dsu_find(dsu, y);
    
    if (xroot == yroot || xroot == -1 || yroot == -1) return;
    
    if (dsu->elements[xroot].rank < dsu->elements[yroot].rank) {
        dsu->elements[xroot].parent = yroot;
    } else if (dsu->elements[xroot].rank > dsu->elements[yroot].rank) {
        dsu->elements[yroot].parent = xroot;
    } else {
        dsu->elements[yroot].parent = xroot;
        dsu->elements[xroot].rank++;
    }
}

// Освободить память DSU
void free_dsu(DSU* dsu){
    if (dsu) {
        free(dsu->elements);
        free(dsu);
    }
}

// Поиск компонент связности
DSU* find_connected_components(const Image* img){
    if (!img || !img->data) return NULL;
    
    int size = img->width * img->height;
    DSU* dsu = create_dsu(size);
    if (!dsu) return NULL;
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int current_pos = y * img->width + x;
            unsigned char current_pixel = img->data[current_pos * 4];
            
            // Проверяем соседей (4-связность)
            if (x > 0) { // Левый сосед
                int left_pos = y * img->width + (x - 1);
                unsigned char left_pixel = img->data[left_pos * 4];
                if (current_pixel == left_pixel) {
                    dsu_union(dsu, current_pos, left_pos);
                }
            }
            
            if (y > 0) { // Верхний сосед
                int top_pos = (y - 1) * img->width + x;
                unsigned char top_pixel = img->data[top_pos * 4];
                if (current_pixel == top_pixel) {
                    dsu_union(dsu, current_pos, top_pos);
                }
            }
        }
    }
    
    return dsu;
}

// Покраска компонент связности разными цветами
void color_components(Image* img, DSU* dsu){
    if (!img || !img->data || !dsu) return;
    
    int* component_colors = (int*)calloc(dsu->size, sizeof(int));
    if (!component_colors) return;
    
    int color_index = 0;
    
    // Сначала находим всех представителей
    for (int i = 0; i < dsu->size; i++){
        int root = dsu_find(dsu, i);
        if (root == i){ // Это корень
            component_colors[i] = color_index++;
        }
    }
    
    // Генерируем случайные цвета для каждой компоненты
    unsigned char** colors = (unsigned char**)malloc(color_index * sizeof(unsigned char*));
    if (!colors){
        free(component_colors);
        return;
    }
    
    for (int i = 0; i < color_index; i++){
        colors[i] = (unsigned char*)malloc(3);
        if (!colors[i]) {
            for (int j = 0; j < i; j++) free(colors[j]);
            free(colors);
            free(component_colors);
            return;
        }
        colors[i][0] = rand() % 129; // R //можно поменять параметры делителей RGB на наборы: 1.(149, 147, 121) - поголубее цвета 
        colors[i][1] = rand() % 147; // G //или 2. (255, 255, 255) - позеленее
        colors[i][2] = rand() % 121; // B
    }
    
    // Раскрашиваем изображение
    for (int y = 0; y < img->height; y++){
        for (int x = 0; x < img->width; x++){
            int pos = y * img->width + x;
            int root = dsu_find(dsu, pos);
            if (root == -1) continue;
            
            int component = component_colors[root];
            // Пропускаем белые области (контуры)
            if (img->data[pos * 4] == 255){
                img->data[pos * 4] = 255;     // R
                img->data[pos * 4 + 1] = 255; // G
                img->data[pos * 4 + 2] = 255; // B
            }
            else{
            img->data[pos*4] = colors[component][0];
            img->data[pos*4+1] = colors[component][1];
            img->data[pos*4+2] = colors[component][2];// Альфа-канал оставляем без изменений
            }
        }
    }
    
    // Освобождаем память
    for (int i = 0; i < color_index; i++) {
        free(colors[i]);
    }
    free(colors);
    free(component_colors);
}
 
/*нашла в интернете такой метод повышения контраста при помощи подсчёта коэффициента*/
void contrast(Image* img, float factor){//повышаем контрастность -> берём "+" factor, иначе - "-"
    if (!img || !img->data) return;
    // Преобразуем фактор в коэффициент контраста
    float contrast = (100.0f + factor) / 100.0f;
    contrast *= contrast;
    // Обрабатываем все пиксели (4 канала на пиксель - RGBA)
    for (int i = 0; i < img->width * img->height * 4; i += 4){
        // Пропускаем альфа-канал (он остаётся без изменений)
        for (int c = 0; c < 3; c++) {
            // Нормализуем значение пикселя [0, 1]
            float pixel = img->data[i + c] / 255.0f;
            // Применяем преобразование контраста
            pixel = ((pixel - 0.5f) * contrast) + 0.5f;
            // Ограничиваем значения
            pixel = (pixel > 1.0f) ? 1.0f : (pixel < 0.0f) ? 0.0f : pixel;
            // Возвращаем в диапазон [0, 255]
            img->data[i + c] = (unsigned char)(pixel * 255.0f);
        }
    }
}

void Gauss_blur(const Image* src, Image* dst){
    if (!src || !src->data || !dst || !dst->data) return;
    if (src->width != dst->width || src->height != dst->height) return;
    
    // Копируем альфа-канал
    for (int i = 0; i < src->width * src->height; i++){
        dst->data[i*4 + 3] = src->data[i*4 + 3];
    }
    
    // Применяем размытие к каждому цветовому каналу
    for (int c = 0; c < 3; c++){
        for (int y = 1; y < src->height-1; y++){
            for (int x = 1; x < src->width-1; x++){
                int pos = (y * src->width + x) * 4 + c;
                float sum = 0.0f;
                
                // Центральный пиксель
                sum += 0.084f * src->data[pos];
                // Соседи по вертикали и горизонтали
                sum += 0.084f * src->data[pos - src->width*4];
                sum += 0.084f * src->data[pos + src->width*4];
                sum += 0.084f * src->data[pos - 4];
                sum += 0.084f * src->data[pos + 4];
                // Диагональные соседи
                sum += 0.063f * src->data[pos - src->width*4 - 4];
                sum += 0.063f * src->data[pos - src->width*4 + 4];
                sum += 0.063f * src->data[pos + src->width*4 - 4];
                sum += 0.063f * src->data[pos + src->width*4 + 4];
                
                dst->data[pos] = (unsigned char)sum;
            }
        }
    }
}

// Функция преобразования Щарра (аналог фильтра Собеля, матрица из Википедии)
void Scharr_filter(const Image* src, Image* dst){
    if (!src || !src->data || !dst || !dst->data) return;
    if (src->width != dst->width || src->height != dst->height) return;
    
    int Gx[3][3] = {{3, 10, 3}, {0, 0, 0}, {-3, -10, -3}};
    int Gy[3][3] = {{3, 0, -3}, {10, 0, -10}, {3, 0, -3}};
    
    // Копируем альфа-канал
    for (int i = 0; i < src->width * src->height; i++){
        dst->data[i*4 + 3] = src->data[i*4 + 3];
    }
    
    for (int y = 1; y < src->height-1; y++){
        for (int x = 1; x < src->width-1; x++){
            int pos = (y * src->width + x) * 4;
            
            // Инициализация градиентов для каждого канала
            float gx_r = 0, gy_r = 0;
            float gx_g = 0, gy_g = 0;
            float gx_b = 0, gy_b = 0;
            
            // Применяем оператор Щарра
            for (int i = -1; i <= 1; i++){
                for (int j = -1; j <= 1; j++){
                    int kernel_pos = ((y + i) * src->width + (x + j)) * 4;
                    int kernel_i = i + 1;
                    int kernel_j = j + 1;
                    
                    gx_r += src->data[kernel_pos] * Gx[kernel_i][kernel_j];
                    gy_r += src->data[kernel_pos] * Gy[kernel_i][kernel_j];
                    
                    gx_g += src->data[kernel_pos + 1] * Gx[kernel_i][kernel_j];
                    gy_g += src->data[kernel_pos + 1] * Gy[kernel_i][kernel_j];
                    
                    gx_b += src->data[kernel_pos + 2] * Gx[kernel_i][kernel_j];
                    gy_b += src->data[kernel_pos + 2] * Gy[kernel_i][kernel_j];
                }
            }
            
            // Вычисляем величину градиента для каждого канала
            int val_r = (int)sqrt(gx_r * gx_r + gy_r * gy_r);
            int val_g = (int)sqrt(gx_g * gx_g + gy_g * gy_g);
            int val_b = (int)sqrt(gx_b * gx_b + gy_b * gy_b);
            

            dst->data[pos] = val_r > 255 ? 255 : val_r;
            dst->data[pos + 1] = val_g > 255 ? 255 : val_g;
            dst->data[pos + 2] = val_b > 255 ? 255 : val_b;
        }
    }
}

void binarize_image(Image* img, unsigned char threshold){
    if (!img || !img->data) return;
    
    for (int i = 0; i < img->width * img->height * 4; i += 4){
        unsigned char r = img->data[i];
        unsigned char g = img->data[i+1];
        unsigned char b = img->data[i+2];
        unsigned char avg = (r + g + b) / 3;
        
        unsigned char binary = (avg > threshold) ? 255 : 0;
        img->data[i] = img->data[i+1] = img->data[i+2] = binary;
    }
}

int main(){
    const char* filename = "image_2.png";
    
    // Загрузка изображения
    Image image = load_png(filename);
    if (!image.data) {
        printf("Failed to load image\n");
        return -1;
    }
    
    // Создаём копии изображений с выделением памяти
    Image blr_pic = {0};
    blr_pic.width = image.width;
    blr_pic.height = image.height;
    blr_pic.data = (unsigned char*)malloc(image.width * image.height * 4);
    if (!blr_pic.data){
        free_image(&image);
        return -1;
    }
    
    Image contrast_pic = {0};
    contrast_pic.width = image.width;
    contrast_pic.height = image.height;
    contrast_pic.data = (unsigned char*)malloc(image.width * image.height * 4);
    if (!contrast_pic.data){
        free_image(&image);
        free(blr_pic.data);
        return -1;
    }
    
    Image finish = {0};
    finish.width = image.width;
    finish.height = image.height;
    finish.data = (unsigned char*)malloc(image.width * image.height * 4);
    if (!finish.data){
        free_image(&image);
        free(blr_pic.data);
        free(contrast_pic.data);
        return -1;
    }
    
    // Обработка изображений
    Gauss_blur(&image, &blr_pic);
    write_png("blurred.png", &blr_pic);
    
    Scharr_filter(&blr_pic, &contrast_pic);
    write_png("scharr.png", &contrast_pic);
    
    contrast(&contrast_pic, 60.0f);
    write_png("contrast.png", &contrast_pic);
    
    // Копируем результат для финальной обработки
    memcpy(finish.data, contrast_pic.data, image.width * image.height * 4);
    
    // Бинаризация
    binarize_image(&finish, 128);
    
    // Поиск и раскраска компонент
    DSU* dsu = find_connected_components(&finish);
    if (dsu) {
        color_components(&finish, dsu);
        free_dsu(dsu);
    }
    
    // Сохранение результата
    write_png("finish.png", &finish);
    
    // Освобождение памяти
    free_image(&image);
    free(blr_pic.data);
    free(contrast_pic.data);
    free(finish.data);
    
    return 0;
}