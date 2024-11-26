#include <stdio.h>
#include <stdlib.h>
#include "tiffio.h"

int discovery_dimensions_resolutions(char* filename, uint32_t* w, uint32_t* h, float* xres, float* yres) {
	TIFF* tiff = TIFFOpen(filename, "r");
	TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, w);
	TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, h);
	TIFFGetField(tiff, TIFFTAG_XRESOLUTION, xres);
	TIFFGetField(tiff, TIFFTAG_YRESOLUTION, yres);
	printf("Resolution: %fx%f\n", *xres, *yres);
	TIFFClose(tiff);
	return 1;
}

int readTiff(char* filename, uint8_t** pixels) {
	uint32_t w, h;
	float xres, yres;
	TIFF* tiff = TIFFOpen(filename, "r");
	TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &h);
	TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &xres);
	TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &yres);
	uint32_t npixels = w * h;
	*pixels = (uint8_t*) malloc(npixels * sizeof(uint8_t));
	uint32_t* raster = (uint32_t*) _TIFFmalloc(npixels * sizeof(uint32_t));
	TIFFReadRGBAImage(tiff, w, h, raster, 0);
	printf("Successfully read the TIFF image %dx%d.\n", w, h);
	for (uint32_t row = 0; row < h; row++) {
		for (uint32_t col = 0; col < w; col++) {
			uint32_t pixel = raster[row * w + col];
			uint8_t r = TIFFGetR(pixel);
			uint8_t g = TIFFGetG(pixel);
			uint8_t b = TIFFGetB(pixel);
			(*pixels)[row * w + col] = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
		}
	}
	_TIFFfree(raster);
	TIFFClose(tiff);
	return 1;
}

int writeTiff(char* filename, uint32_t w, uint32_t h, float xres, float yres, uint8_t* pixels) {
	TIFF* tiff = TIFFOpen(filename, "w");
	TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, w);
	TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, h);
	TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_BOTLEFT);
	TIFFSetField(tiff, TIFFTAG_XRESOLUTION, xres);
	TIFFSetField(tiff, TIFFTAG_YRESOLUTION, yres);
	TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	for (uint32_t row = 0; row < h; row++) {
		TIFFWriteScanline(tiff, &pixels[row * w], row, 0);
	}
	TIFFClose(tiff);
	return 1;
}

int interpolate(uint8_t* pixels, uint32_t oldW, uint32_t oldH,
				uint8_t* newPixels, uint32_t newW, uint32_t newH,
				float xres, float yres, float newres) {
	for (uint32_t row = 0; row < newH; row++) {
		for (uint32_t col = 0; col < newW; col++) {
			uint32_t x1, x2, y1, y2;
			float x, y;
			x = ((float) col) * (xres / newres);
			y = ((float) row) * (yres / newres);
			x1 = (uint32_t) x;
			x2 = x1 + 1;
			y1 = (uint32_t) y;
			y2 = y1 + 1;
			if (x2 >= oldW) x2 = oldW - 1;
			if (y2 >= oldH) y2 = oldH - 1;
			newPixels[row * newW + col] = (uint8_t)(pixels[y1 * oldW + x1] * (x2 - x) * (y2 - y) +
													pixels[y1 * oldW + x2] * (x - x1) * (y2 - y) +
													pixels[y2 * oldW + x1] * (x2 - x) * (y - y1) +
													pixels[y2 * oldW + x2] * (x - x1) * (y - y1));
		}
	}
	return 1;
}

int main(int argc, char* argv[]) {
	
	char* filename = argv[1];
	float newdpi = (float) atoi(argv[2]);

	uint32_t oldW, oldH;
	float xres, yres;
	uint8_t* oldPixels;
	discovery_dimensions_resolutions(filename, &oldW, &oldH, &xres, &yres);
	oldPixels = (uint8_t*) malloc(oldW * oldH * sizeof(uint8_t));
	readTiff(filename, &oldPixels);

	uint32_t newW, newH;
	newW = (uint32_t) (oldW * (newdpi / xres));
	newH = (uint32_t) (oldH * (newdpi / yres));
	printf("new w and h: %d %d\n", newW, newH);
	uint8_t* newPixels = (uint8_t*) malloc(newW * newH * sizeof(uint8_t));
	printf("Interpolating...\n");
	interpolate(oldPixels, oldW, oldH, newPixels, newW, newH, xres, yres, newdpi);
	printf("Writing the new image...\n");
	writeTiff("output.tif", newW, newH, newdpi, newdpi, newPixels);
	free(oldPixels);
	free(newPixels);
	return 0;
}
