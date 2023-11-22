/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec:  Name:
//
//
//
// Date:
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
//
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
  int width;
  int height;
  int maxval;    // maximum gray value (pixels with maxval are pure WHITE)
  uint8* pixel;  // pixel data (a raster scan)
};

// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
//
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char* errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char* ImageErrMsg() {  ///
  return errCause;
}

// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success =
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
//
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
//
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
//
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)

// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char* failmsg) {
  errCause = (char*)(condition ? "" : failmsg);
  return condition;
}

//Calcular o tamanho de uma imagem
static int GetSize(Image img){
  return img->height*img->width;
}

/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) {  ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
  InstrName[1] = "comparações"; 
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
#define comps InstrCount[1]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) {  ///
  assert(width >= 0);
  assert(height >= 0);
  assert(0 < maxval && maxval <= PixMax);
  // Insert your code here!

  // Allocate memory for the pixel data *
  uint8_t* pixel = malloc(width * height * sizeof(uint8_t));
  if (pixel == NULL) {
    errCause = "Falhou a alocação de memória para o pixel";
    return NULL;
  }

  // Alocar memória para o struct Image
  Image img = malloc(sizeof(struct image));

  if (img == NULL) {
    errCause = "Falhou a alocação de memória para a struct Image";
    // Usei errCause para não ter que criar uma variável nova
    free(pixel);
    // Preciso de dar free ao pixel porque não foi atribuido ao img
    // Não dar memleak
    return NULL;
  }

  // Set the Image properties *
  img->width = width;
  img->height = height;
  img->maxval = maxval;
  img->pixel = pixel;

  return img;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image* imgp) {  ///
  assert(imgp != NULL);
  // Insert your code here!
  free((*imgp)->pixel);
  free(*imgp);
  *imgp = NULL;
  // Mesmo dando free ao imgp, o valor do imgp não é NULL
  // Então dou set ao valor do pointer como NULL
}

/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE* f) {
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char* filename) {  ///
  int w, h;
  int maxval;
  char c;
  FILE* f = NULL;
  Image img = NULL;

  int success =
      check((f = fopen(filename, "rb")) != NULL, "Open failed") &&
      // Parse PGM header
      check(fscanf(f, "P%c ", &c) == 1 && c == '5', "Invalid file format") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &h) == 1 && h >= 0, "Invalid height") &&
      skipComments(f) >= 0 &&
      check(
          fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax,
          "Invalid maxval") &&
      check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected") &&
      // Allocate image
      (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
      // Read pixels
      check(fread(img->pixel, sizeof(uint8), w * h, f) == w * h,
            "Reading pixels");
  PIXMEM += (unsigned long)(w * h);  // count pixel memory accesses

  // Cleanup
  if (!success) {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL) fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char* filename) {  ///
  assert(img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE* f = NULL;

  int success = check((f = fopen(filename, "wb")) != NULL, "Open failed") &&
                check(fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0,
                      "Writing header failed") &&
                check(fwrite(img->pixel, sizeof(uint8), w * h, f) == w * h,
                      "Writing pixels failed");
  PIXMEM += (unsigned long)(w * h);  // count pixel memory accesses

  // Cleanup
  if (f != NULL) fclose(f);
  return success;
}

/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) {  ///
  assert(img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img) {  ///
  assert(img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) {  ///
  assert(img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8* min, uint8* max) {  ///
  assert(img != NULL);
  // Insert your code here!

  //*min = 0;
  //*max = 255;

  for (int i = 0; i < img->width * img->height; i++) {
    if (img->pixel[i] < *min) {
      *min = img->pixel[i];
    }
    if (img->pixel[i] > *max) {
      *max = img->pixel[i];
    }
  }
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) {  ///
  assert(img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) {  ///
  assert(img != NULL);
  // Insert your code here!
  // return (0 <= w && w < img->width + x) && (0 <= h && h < img->height + y)
  return ImageValidPos(img, x, y) && ImageValidPos(img, x + w, y + h);
  // Assim vemos se o retangulo está dentro da imagem
  // Porque se o x e y forem maiores que a largura e altura da imagem
  // Então o retangulo não está dentro da imagem
  // Isto deve estar mal, tenho que ver melhor, perceber o x,y,w,h que penso que
  // seja o x,y do retangulo e o w,h a largura e altura do retangulo
  // Tá certo, e bué bem resumido dw
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel.
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
  int index;
  // Insert your code here!
  // index = y * (img->width-1) + x;
  // Isto estava mal porque se tivesse um numero numa matriz 3x3 na coordenada
  // (1,1) ele iria estar na posicao index 3
  /*
  0 1 2
  3 4 5
  6 7 8
  */
  // Posição (1,1) -> pode corresponder a 1 ou a 5, mas a posição 3 daria print
  // ao array, 4 ou 3, dependendo se conta index 0 ou 1 Se as imagens forem
  // retangulares sempre, posso fazer img->width * img->height Ou secalhar não
  // porque ia ser sempre o mesmo valor que é o da struct image

  // tendo um array com i linhas j colunas
  // Posição (1,1) -> corresponde ao meio numa matriz 3x3
  // Neste caso o numero ficaria no indice 4
  // Posição 0,0 corresponderia ao indice 0
  // i corresponde à linha onde estas e j corresponde à coluna onde estas
  // 0,0 é o canto superior esquerdo
  index = y * (img->width) + x;

  assert(0 <= index && index < img->width * img->height);
  // Esta função vai transformar uma matriz nalgo linear
  // Se a matriz for 10x10 então o último pixel está no index 99
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) {  ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (read)
  return img->pixel[G(img, x, y)];
}

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) {  ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1;  // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
  // O pixel em x,y vai ser level
}

/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.

/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) {  ///
  assert(img != NULL);
  // Insert your code here!
  int size = GetSize(img);
  for (int i = 0; i < size; i++) {
    img->pixel[i] = img->maxval - img->pixel[i];
  }
  // Testar se dá todos os valores certos
  // Estou em dúvida se converteria realmente para o oposto se já for um light
  // pixel
  // Sim, em princípio está certo, o mais branco, pixel=maxval, vira o mais escuro
  // pixel=0 
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) {  ///
  assert(img != NULL);
  // Insert your code here!
  int size = GetSize(img);
  for (int i = 0; i < size; i++) {
    if (img->pixel[i] < thr) {
      img->pixel[i] = 0;
    } else {
      img->pixel[i] = img->maxval;
    }
  }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) {  ///
  assert(img != NULL);
  assert(factor >= 0.0);
  // Insert your code here!
  // Podemos otimizar o uso da variável size? -> Verificar se podemos usar uma
  // função
  // Podemos, já coloquei
  int size = GetSize(img);
  for (int i = 0; i < size; i++) {
    img->pixel[i] = img->pixel[i] * factor + 0.5;
    // Porque não dava para dar round então + 0.5
    if (img->pixel[i] > img->maxval) {
      img->pixel[i] = img->maxval;
    }
  }
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint:
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees anti-clockwise.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) {
  assert(img != NULL);
  // Insert code here!
  Image new_img = ImageCreate(img->height, img->width, img->maxval);
  assert(new_img != NULL);
  // As imagens vão ser apenas 4 por 4?
  // Anti clockwise??? -> Resultado

  // Usar a função ImageSetPixel e o get i guess
  // img->width == *img.width

  // Exemplo da rotação

  // Esta tem dois for's , vou percorrer como se fosse uma matriz
  // Comparar a complexidade desta, com a anteriormente desenvolvida ->
  // Guilherme
  for (int i = 0; i < img->width; i++) {
    for (int j = 0; j < img->height; j++) {
      // A função ImageSetPixel já vai buscar o pixel da imagem e guarda o pixel
      // anterior Função G mete os pixeis de forma linear Trocar a ordem das
      // coordenadas x e y e reverter
      int x = img->height - 1 - j;
      int y = i;
      // Pixel da nova imagem
      ImageSetPixel(new_img, i, j, ImageGetPixel(img, x, y));
    }
  }
  // Para o Guilherme
  // img é uma imagem
  // print(img) - > 1 2 3 4
  // imagerotate(*img)
  // ou entao img = imagerotate(img)
  // print(img) -> 4 3 2 1
  // mas se for
  // imagerotate(img)
  // print(img) -> 1 2 3 4
  return new_img;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) {
  assert(img != NULL);
  // Insert your code here
  Image new_img = ImageCreate(img->width, img->height, img->maxval);
  assert(new_img != NULL);

  // Fazer um for dentro dum for?

  for (int i = 0; i < img->width * img->height; i++) {
    // Obter o x da imagem original
    int x = i % img->width;
    // Caso não percebas isto gui, é porque imaginei a imagem como uma matriz
    // Mas como é uma matriz linear, então o y é o resto da divisão do i pela
    // largura da imagem No sentido em que tens o indice 3 que corresponde a
    // coordenada 1,0 na matriz e sendo matriz 3x3 3%3 = 0 Obter o y da imagem

    // Exemplo
    // 0 1 2
    // 3 4 5
    // 6 7 8
    // 6 -> 6%3 = 0
    // 6 -> 6/3 = 2
    // 6 -> 2,0
    // x -> colunas
    // y -> linhas

    int y = i / img->height;
    int new_x = img->width - 1 -
                x;  // O x da nova imagem é o x da imagem original invertido
    ImageSetPixel(new_img, new_x, y, ImageGetPixel(img, x, y));
  }

  return new_img;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) {  ///
  assert(img != NULL);
  assert(ImageValidRect(img, x, y, w, h));
  // Insert your code here!
  Image new_img = ImageCreate(w, h, img->maxval);
  assert(new_img != NULL);

  for (int i = 0; i < w * h; i++) {
    int new_x = i % w;  // Como na função acima vamos buscar o x da imagem
                        // original daquela posição
    int new_y = i / w;
    // Eu estou a criar os pixeis, mas depois preciso de dar set a eles
    ImageSetPixel(new_img, new_x, new_y,
                  ImageGetPixel(img, x + new_x, y + new_y));
    // Somar x e y onde começa o retangulo
  }

  return new_img;
}

/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) {  ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));
  // Insert your code here!
  // Aqui não é preciso criar uma imagem nova
  for (int i = 0; i < img2->width * img2->height; i++) {
    int new_x = i % img2->width;
    int new_y = i / img2->width;
    ImageSetPixel(img1, x + new_x, y + new_y,
                  ImageGetPixel(img2, new_x, new_y));
    // Do mesmo método que em ImageCrop
    // Só que depois damos set ao pixel da imagem 1
    // Com os novos pixeis da imagem2
    // Fazemos o x + new_x e y + new_y para sabermos onde começar a colar a
    // imagem
  }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) {
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));
  // Insert your code here!
  for (int j = 0; j < img2->width; j++) {
    for (int i = 0; i < img2->height; i++) {
      int saturation = (ImageGetPixel(img1, x + j, y + i) * (1-alpha) + ImageGetPixel(img2, j, i) * (alpha)) + 0.5;
      if (saturation > img1->maxval) {
        saturation = img1->maxval;
      }
      else if (saturation < 0) {
        saturation = 0;
      }
      ImageSetPixel(img1, x + j, y + i, saturation);
    }
  }
}
/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) {  ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidPos(img1, x, y));
  // Insert your code here!
  for (int i = 0; i < img2->height; i++) {
    for (int j = 0; j < img2->width; j++) {
      if (ImageGetPixel(img1, x + j, y + i) != ImageGetPixel(img2, j, i)) {
        comps+=1;
        return 0;
      }
    }
  }
  return 1;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px,
/// *py). If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int* px, int* py, Image img2) {  ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  // Insert your code here!
  for (int i = 0; i < img1->height - img2->height+1; i++) {
    for (int j = 0; j < img1->width - img2->width+1; j++) {
      if (ImageMatchSubImage(img1, j, i, img2)) {
        *px = i;
        *py = j;
        return 1;
      }
    }
  }
  return 0;
}

/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
void ImageBlur(Image img, int dx, int dy) { 
  int* cumsum = malloc(GetSize(img) * sizeof(int));
  cumsum[0]=ImageGetPixel(img,0,0);
  int C1,C2,C3,C4;
  int lex,ldx,lsy,liy;
  int ha=2*dy +1,ca=2*dx+1;
  for (int i = 0; i < img->height - img->height+1; i++) {
    for (int j = 0; j < img->width - img->width+1; j++) {
      int index=G(img,j,i);
      if(i>0){
        cumsum[index]+=cumsum[index-img->width]+ImageGetPixel(img,j,i);
      }
      if(j>0){
        cumsum[index]+=cumsum[index-1];
      }

    }
  }
  int blurA = (2 * dx + 1) * (2 * dy + 1);
  for (int i = 0; i < img->height; i++) {
      for (int j = 0; j < img->width; j++) {
        
        lex=j-dx-1; ldx= j+dx;
        lsy=i-dy-1; liy=i+dy;
        if(ldx>=img->width){ldx=img->width-1; ca=ca-(j+dx-img->width); }
        if(liy>=img->height){liy=img->height-1;ha= ha-(i+dy-img->height);}
        C4=cumsum[G(img,ldx,liy)];
        if(lex<0){
          C1=0;
          C3=0;
          ca=ca+lex;
          if(lsy<0){
            C2=0;
            ha=ha+lsy;
          } 
          else {
            C2=cumsum[G(img,ldx,lsy)];
          }
        }
        else {
          if(lsy<0){
            C2=0;
            C1=0;   
            C3=cumsum[G(img,lex,liy)];
            ha=ha+lsy;
          }
        }
        if(lsy>=0 && lex>=0){
          C1=cumsum[G(img,lex,lsy)];
          C2=cumsum[G(img,ldx,lsy)];
          C3=cumsum[G(img,lex,liy)];
        } 
        


        blurA=ca*ha;
        ImageSetPixel(img,j,i,(C4-C3-C2+C1)/blurA);
        ca=(2 * dx + 1);
        ha=(2 * dy + 1);
    }
  }

}








void ImageBlurOld1(Image img, int dx, int dy) {  ///
  // Insert your code here!
  // Criar uma imagem nova para os pixeis que já foram blurred não influenciarem os píxeis que vão ser blurred...
    Image img2 = ImageCreate(img->width, img->height, img->maxval);
    // A alocação vai falhar?
    // Provavelmente não, só se ficar sem memória.
    for (int g = 0; g < img->width * img->height; g++) {
      img2->pixel[g] = img->pixel[g];
    }
    int sum = 0;
    int count = 0;
    int size = (2 * dx + 1) * (2 * dy + 1);
    int* pixels = malloc(size * sizeof(int));
    for (int i = 0; i < img->height; i++) {
      for (int j = 0; j < img->width; j++) {
        for (int k = -dx; k <= dx; k++) {
          for (int l = -dy; l <= dy; l++) {
            if (ImageValidPos(img, j + k, i + l)) {
              pixels[count] = ImageGetPixel(img2, j + k, i + l);
              count++;
            }
          }
        }
        for (int m = 0; m < count; m++) {
          sum += pixels[m];
        }
        ImageSetPixel(img, j, i, (((double)sum / count) + 0.5));
        count = 0;
        sum = 0;
      }
    }
      free(pixels);
      //Conseguimos criar uma versão mais eficiente que não dependa 
      // do tamanho da janela de blur, o stor explicou, vou tentar 
      //implementar depois de guardar esta para o relatorio
}

void ImageBlurOld2(Image img, int dx, int dy) {  ///
  // Insert your code here!
  // Criar uma imagem nova para os pixeis que já foram blurred não influenciarem os píxeis que vão ser blurred...
    Image img2 = ImageCreate(img->width, img->height, img->maxval);
    // A alocação vai falhar?
    // Provavelmente não, só se ficar sem memória.
    for (int g = 0; g < img->width * img->height; g++) {
      img2->pixel[g] = img->pixel[g];
    }
    int sum = 0;
    int count = 0;
    int size = (2 * dx + 1) * (2 * dy + 1);
    for (int i = 0; i < img->height; i++) {
      for (int j = 0; j < img->width; j++) {
        for (int k = -dx; k <= dx; k++) {
          for (int l = -dy; l <= dy; l++) {
            if (ImageValidPos(img, j + k, i + l)) {
              sum+= ImageGetPixel(img2, j + k, i + l);
              count++;
            }
          }
        }
        ImageSetPixel(img, j, i, (((double)sum / count) + 0.5));
        count = 0;
        sum = 0;
      }
    }
      //Conseguimos criar uma versão mais eficiente que não dependa 
      // do tamanho da janela de blur, o stor explicou, vou tentar 
      //implementar depois de guardar esta para o relatorio
}



