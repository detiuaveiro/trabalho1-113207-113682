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
// Funções auxiliares criadas:
//Calcular o tamanho de uma imagem
static int GetSize(Image img){
  return img->height*img->width;
}
//Conseguir máximo entre 2 valores
int max(double a, double b) {
  return a > b ? a : b;
}
//Conseguir minímo entre 2 valores
int min(double a, double b) {
  return a < b ? a : b;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) {  ///
  InstrCalibrate();
  InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
  // Name other counters here...
  InstrName[1] = "comparações"; 
  InstrName[2] = "somas";
  InstrName[3] = "divisões";  
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
#define comps InstrCount[1]
#define somas InstrCount[2]
#define divs InstrCount[3]
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

  // Alocar memória para o array pixel
  uint8_t* pixel = malloc(width * height * sizeof(uint8_t));
  if (pixel == NULL) {
    errCause = "Falhou a alocação de memória para o pixel";
    return NULL;
  }

  // Alocar memória para o struct Image
  Image img = malloc(sizeof(struct image));

  if (img == NULL) {
    errCause = "Falhou a alocação de memória para a struct Image";
    free(pixel);
    // Dar free ao pixel porque não foi atribuido ao img
    // no caso de erro
    return NULL;
  }

  // Definir as propriedades da imagem
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
  // Libertar memória do array e da imagem
  free((*imgp)->pixel);
  free(*imgp);
  // Dar set ao valor do pointer como NULL
  *imgp = NULL;
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
  // Iterar pela imagem e mudar o valor do minimo e 
  // maximo caso encontremos valores menores ou maiores, respetivamente
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
  //Verificar se os cantos do retângulo pertencem à imagem
  return ImageValidPos(img, x, y) && ImageValidPos(img, x + w, y + h);
  
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
  // Posição (1,1) -> corresponde ao meio numa matriz 3x3 -> indíce 4 do array
  // Posição (0,0) corresponderia ao indice 0
  // O indice da posição de cada pixel é igual à linha em que se encontra 
  // vezes o comprimento da imagem  mais a coluna onde se encontra
  index = y * (img->width) + x;
  assert(0 <= index && index < img->width * img->height);
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
  //Para cada pixel, fazemos com que o seu valor
  // se torne o inverso, subtraíndo o valor máximo(branco)
  // pelo atual e aplicando a diferença ao pixel
  for (int i = 0; i < size; i++) {
    img->pixel[i] = img->maxval - img->pixel[i];
  }
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
  int size = GetSize(img);
  for (int i = 0; i < size; i++) {
    img->pixel[i] = img->pixel[i] * factor + 0.5;
    // Adicionamos +0.5 para contornar erros 
    // de arredondamento
    // Caso o valor do pixel supere o maxval,
    // igualamo-lo ao mesmo
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

  for (int i = 0; i < img->width; i++) {
    for (int j = 0; j < img->height; j++) {
      //Revertemos as coordenadas x e y
      int x = img->height - 1 - j;
      int y = i;
      // Pixel da nova imagem
      ImageSetPixel(new_img, i, j, ImageGetPixel(img, x, y));
    }
  }

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

  for (int i = 0; i < img->width * img->height; i++) {
    // Obter o x da imagem original
    int x = i % img->width;
    // Obter o y da imagem original
    int y = i / img->height;
    int new_x = img->width - 1 -x;  // O x da nova imagem é o x da imagem original invertido
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
    // Obter o x do retângulo
    int new_x = i % w;  
    // Obter o y do retângulo
    int new_y = i / w;
    // Dar set ao valor de cada pixel da imagem nova, obtendo cada pixel dentro 
    // do retângulo
    ImageSetPixel(new_img, new_x, new_y, ImageGetPixel(img, x + new_x, y + new_y));
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
  //Verificar se a img2 cabe na img1 na posição x,y
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));
  // Insert your code here!
  for (int i = 0; i < img2->width * img2->height; i++) {
    int new_x = i % img2->width;
    int new_y = i / img2->width;
    // Colar os pixéis da img2 na img1, começando em x,y
    ImageSetPixel(img1, x + new_x, y + new_y, ImageGetPixel(img2, new_x, new_y));

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
      // Cálculo do level do pixel, somamos +0.5 por causa de arredondamentos.
      int saturation = (ImageGetPixel(img1, x + j, y + i) * (1-alpha) + ImageGetPixel(img2, j, i) * (alpha)) + 0.5;
      // Verificar se existe overflow
      if (saturation > img1->maxval) {
        saturation = img1->maxval;
      }
      // Verificar se existe underflow
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
      comps+=1;
      // verificamos se os pixeis a partir de (x,y) da imagem1 
      // correspondem à imagem2, parando a execução no caso 
      // de se encontrar uma diferença
      if (ImageGetPixel(img1, x + j, y + i) != ImageGetPixel(img2, j, i)) {     
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
  // Iteramos cada pixel da imagem1 e, para cada um,
  // verificamos se a imagem2 se pode encontrar a começar nesse pixel
  for (int i = 0; i < img1->height - img2->height+1; i++) {
    for (int j = 0; j < img1->width - img2->width+1; j++) {
      if (ImageMatchSubImage(img1, j, i, img2)) {
        *px = j;
        *py = i;
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
  assert(img != NULL);
  assert(dx >= 0);
  assert(dy >= 0);

 // Criação do array que vai guardar os valores das somas cumulativas
  double *sums = malloc(GetSize(img) * sizeof(double)); 
  assert(sums != NULL);
  int index;
  // cálculo das somas cumulativas
   for (int i = 0; i < img->height; i++) {
     for (int j = 0; j < img->width; j++) {
       index = G(img, j, i);
       sums[index] = ImageGetPixel(img, j, i);

       // soma cumulativa dos pixeis acima
       if (i > 0) {
         sums[index] += sums[index - img->width];
       }

       // adicionar à soma os pixeis da esquerda
       if (j > 0) {
         sums[index] += sums[index - 1];
       }

       // subtrair a soma cumulativa do pixel diagonal à esquerda
      if (j > 0 && i > 0) {
        sums[index] -= sums[index - 1 - img->width];
      }
    }
   }
  // aplicação do filtro blur 
  for (int row = 0; row < img->height; row++) {
    for (int col = 0; col < img->width; col++) {
      // calcular as bordas da janela
      int x_min = max(0, col - dx - 1);
      int x_max = min(col + dx, img->width - 1);
      int y_min = max(0, row - dy - 1);
      int y_max = min(row + dy, img->height - 1);

      // numero de pixeis na janela
      int pixels = (x_max - x_min) * (y_max - y_min);

      //Verificar se algum canto da janela se encontra fora da imagem
      int a = y_min< 0 || x_min< 0 ? 0 : sums[G(img,x_min,y_min)];
      int b = y_min< 0 ? 0 : sums[G(img,x_max,y_min)];
      int c = x_min< 0 ? 0 : sums[G(img,x_min,y_max)];
      int d = sums[G(img,x_max, y_max)];
      // cálculo da médias dos pixéis da janela e associação
      // somando a soma cumulativa do canto inferior direito
      // à do canto superior esquerdo e subtraindo os outros dois
      // No fim dividimos pela área e somamos 0.5 para evitar
      // erros de arredondamento
      int val = d-c-b+a;
      img->pixel[G(img,col, row)]  = (double)val/pixels+0.5;


    }
  }
  // libertar a memória
  free(sums);

}

// #################################################################
// ########### Versões menos eficientes da blur ####################
// Parecida à função de cima, mas a de cima foi refeita até com nomes mais compreensiveis
void ImageBlurNotCorrected(Image img, int dx, int dy) {
   // Alocar memoria para o array de somas cumulativas
  uint8_t* cumsum = malloc(GetSize(img) * sizeof(uint8_t));

   // Variáveis para os indices do pixel e soma cumulativa
  int index;
   uint8_t C1 = 0, C2 = 0, C3, C4;

   // Variáveis para o loop das bordas e indices
   int lex, ldx, lsy, liy;

   // Dimensao do blur
   int ha, ca;

   // Calcular a soma cumulativa de cada pixel
   for (int i = 0; i < img->height; i++) {
     for (int j = 0; j < img->width; j++) {
       index = G(img, j, i);
       cumsum[index] = ImageGetPixel(img, j, i);

       // De resto calcula-se igual à função acima
       if (i > 0) {
         cumsum[index] += cumsum[index - img->width];
       }

      
      if (j > 0) {
        cumsum[index] += cumsum[index - 1];
      }

  
       if (j > 0 && i > 0) {
         cumsum[index] -= cumsum[index - 1 - img->width];
       }
     }
  }

   // Aplicar o blur aos píxeis
   for (int i = 0; i < img->height; i++) {
     lsy = i - dy - 1;
     liy = i + dy;
     ha = 2 * dy + 1;
    
     if (lsy < 0) {
       C2 = 0;
       C1 = 0;
       ha = ha + lsy;
    }

     // Handle boundary conditions for the bottom of the image
     if (liy >= img->height) {
       liy = img->height - 1;
       ha = ha - (dy - (img->height - i - 1));
     }

     for (int j = 0; j < img->width; j++) {
       lex = j - dx - 1;
       ldx = j + dx;
      // Condição de bordas
     
       if (ldx >= img->width) {
        ldx = img->width - 1;
       }

    
       C4 = cumsum[G(img, ldx, liy)];

     
       if (lex < 0) {
         C1 = 0;
         C3 = 0;

        
         if (lsy >= 0) {
           C2 = cumsum[G(img, ldx, lsy)];
         }
       } else {
        
         if (lsy >= 0) {
           C1 = cumsum[G(img, lex, lsy)];
           C2 = cumsum[G(img, ldx, lsy)];
           C3 = cumsum[G(img, lex, liy)];
         } else {
           C3 = cumsum[G(img, lex, liy)];
         }
       }


       ca = 2 * dx + 1;

       
       ImageSetPixel(img, j, i, (C4 - C3 - C2 + C1) / (ha * ca));
     }
   }

  
   free(cumsum);
}


void ImageBlurOld1(Image img, int dx, int dy) { 
  assert(img != NULL);
  assert(dx >= 0);
  assert(dy >= 0);

  // alocar um array que vai conter os valores dos pixeis blurred
  double* blurred_pixels = malloc(img->width * img->height * sizeof(double));
  assert(blurred_pixels != NULL);


  // fazer um loop pelos pixeis da imagem original e calcular a media dos pixeis no retangulo
  // [x-dx, x+dx]x[y-dy, y+dy] x e y sao as coordenadas do pixel
  // dx e dy vai ser o tamanho do retangulo

  for (int pixel = 0; pixel < img->width * img->height; pixel++) {
    int x = pixel % img->width;
    int y = pixel / img->width;
    // printf("x: %d, y: %d\n", x, y);
    // soma dos pixeis
    int sum = 0;
    // count para pixeis validos
    int count = 0;
    // loop no retangulo
    for (int i = -dx; i <= dx; i++) {
      for (int j = -dy; j <= dy; j++) {
        //primeiro vamos ter que verificar se a posicão é valida
        if (ImageValidPos(img, x + i, y + j)) {
          // adcioanr o pixel a soma
          sum += ImageGetPixel(img, x + i, y + j);
          //incrementar o count
          count++;
          somas++;
        }
      }
    }
    // calcular a media e arredondar
    // dar cast a double
    blurred_pixels[pixel] = (double)sum / count;
    divs++;

  }

  FILE* f = fopen("../counts.txt", "w");
  // //copy the blurred pixels to the original image
  for (int pixel = 0; pixel < img->width * img->height; pixel++) {
    img->pixel[pixel] = blurred_pixels[pixel] + 0.5;
    fprintf(f, "%d\n", img->pixel[pixel]);
  }
  free(blurred_pixels);
}





void ImageBlurOld3(Image img, int dx, int dy) {  ///
  // Insert your code here!
  // Criar uma imagem nova para os pixeis que já foram blurred não influenciarem os píxeis que vão ser blurred...
    Image img2 = ImageCreate(img->width, img->height, img->maxval);

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
    // int size = (2 * dx + 1) * (2 * dy + 1);
    for (int i = 0; i < img->height; i++) {
      for (int j = 0; j < img->width; j++) {
        for (int k = -dx; k <= dx; k++) {
          for (int l = -dy; l <= dy; l++) {
            if (ImageValidPos(img, j + k, i + l)) {
              sum+= ImageGetPixel(img2, j + k, i + l);
              count++;
              somas++;
            }
          }
        }
        ImageSetPixel(img, j, i, (((double)sum / count) + 0.5));
        divs++;
        count = 0;
        sum = 0;
      }
    }
  
}



