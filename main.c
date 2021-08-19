#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//struct for storing the bmp data
struct bmp_header {
	int header;	//2 bytes
	int size;	//4 bytes
	int res_1;	//2 bytes
	int res_2;	//2 bytes
	int adress;	//4 bytes
};

//struct for storing the dib data
struct dib_header {
	int size;
	int b_width;
	int b_height;
	int clrpln;
	int bitperpix;
	///
	int comprmeth;
	int imsize;
	int horres;	//horizontal resolution in pixels per meter
	int verres;	//vertical resolution in pixels per meter
	int colornum;
	int impcolor;
};

void argumentshandler(int argcount, char const *argvector[], char filename[100]); //handles the arguments
void populate_bmp_head(struct bmp_header * data, FILE * fp); //reads the bmp data from the image file and fills in the bmp_header struct
void populate_dib_head(struct dib_header * data, FILE * fp); //reads the dib data from the image file and fills in the dib_header struct
void scan_colors(int arraypos, int rowsize, int rowpadding, int width, int color, FILE * fp, FILE * fr, FILE * fg, FILE * fb); //scans the pixels and adds the corrext ones to the rgb files

int main(int argc, char const *argv[])
{
	char filename[100] = "";
	char redname[100] = "";
	char greenname[100] = "";
	char bluename[100] = "";
	int dotpoint = 0; 

	argumentshandler(argc, argv, filename); //calls the function to check the arguments
	dotpoint = strcspn(filename, ".b");// looks for the "." in ".bmp" to correctly add the _color
	//edits the strings the correct names
	strncpy(redname, filename, dotpoint);
	strcat(redname, "_red.bmp");
	strncpy(greenname, filename, dotpoint);
	strcat(greenname, "_green.bmp");
	strncpy(bluename, filename, dotpoint);
	strcat(bluename, "_blue.bmp");
	//creates pointers
	FILE * fp = NULL;
	FILE * fr = NULL;
	FILE * fg = NULL;
	FILE * fb = NULL;
	//tries to open the file and checks if it's valid
	fp = fopen(filename, "rb");
	if(ftell(fp) == -1){
		printf("error file cannot be opened\n");
		exit(1);
	}
	else{
		printf("file %s found!\n", filename);
	}
	//creates and opening the color files
	fr = fopen(redname, "wb");
	fg = fopen(greenname, "wb");
	fb = fopen(bluename, "wb");
	printf("The new filenames will be %s, %s and %s\n", redname, greenname, bluename);
	//defining the structs and their pointers
	struct bmp_header bmp_head;
	struct bmp_header * bmp_hd_ptr;
	bmp_hd_ptr = &bmp_head;
	//
	struct dib_header dib_head;
	struct dib_header * dib_hd_ptr;
	dib_hd_ptr = &dib_head;
	//calls function to fill the struct with data
	populate_bmp_head(bmp_hd_ptr, fp);
	populate_dib_head(dib_hd_ptr, fp);


	//prints the content of the structs, used for debugging
	/*
	printf("de positie in de file is %ld\n", ftell(fp));

	printf("de waarde van de header is: 0x%x\n", bmp_head.header);
	printf("de waarde van de size is: 0x%x\n", bmp_head.size);
	printf("de waarde van de res_1 is: 0x%x\n", bmp_head.res_1);
	printf("de waarde van de res_2 is: 0x%x\n", bmp_head.res_2);
	printf("de waarde van de adress is: 0x%x\n", bmp_head.adress);
	///
	printf("de dib size is 0x%x\n", dib_head.size);
	printf("de width is 0x%x\n", dib_head.b_width);
	printf("de height is 0x%x\n", dib_head.b_height);
	printf("de colorplane is 0x%x\n", dib_head.clrpln);
	printf("de dib bits per pixel is 0x%x\n", dib_head.bitperpix);
	///
	printf("de compressionmethod is 0x%x\n", dib_head.comprmeth);
	printf("de image size is 0x%x\n", dib_head.imsize);
	printf("de horizontal resolution is 0x%x\n", dib_head.horres);
	printf("de vertical resolution is 0x%x\n", dib_head.verres);
	printf("de colornumbers is 0x%x\n", dib_head.colornum);
	printf("de important colors is 0x%x\n", dib_head.impcolor);
	*/

	//calculates the rowsizes and padding to correctly scan and print the pixel array
	int rowsize = (dib_head.b_width * (dib_head.bitperpix / 8));
	int rowpadding = rowsize % 4;

	//debug printing
	//printf("de rowsize is: 0x%x\n", rowsize);
	//printf("de padding is: 0x%x\n", rowpadding);

	//scans the metadata from the image file and adds it to the color files
	rewind(fp);
	void * header_storage = malloc(bmp_head.adress);
	fread(header_storage, bmp_head.adress, 1, fp);
	fwrite(header_storage, bmp_head.adress, 1, fr);
	fwrite(header_storage, bmp_head.adress, 1, fg);
	fwrite(header_storage, bmp_head.adress, 1, fb);
	free(header_storage);

	//calls the function to make the colored files
	scan_colors(bmp_head.adress, rowsize, rowpadding, dib_head.b_width, 0, fp, fr, fg, fb);

	fclose(fp);
	return 0;
}

void argumentshandler(int argcount, char const *argvector[], char filename[100]){
	//checks the arguments given, if --help is included, the help info will be printed, if the argument contains ".bmp", it will be designated as the image file 
	for (int i = 0; i < argcount; ++i)
	{
		if(strcmp(argvector[i], "--help") == 0){
			printf("----------help----------\n");
			printf("command line arguments:\n");
			printf("--help: shows this help interface\n");
			printf("*image*.bmp: designates the image file for which the red green and blue image should be extraced\n");
			printf("\nThe image requirements:\n");
			printf("The image can be of any size\n");
			printf("The image encoding needs to be of the type 'BITMAPINFOHEADER'\n");
			printf("------------------------\n\n");
		}
		if(strstr(argvector[i], ".bmp") != NULL){
			strcpy(filename, argvector[i]);
		}
	}
}

void populate_bmp_head(struct bmp_header * data, FILE * fp){
	//allocates memory to store the metadata, this data comes in either 2 or 4 bytes.
	int * two_bytes = (int *) malloc(2);
	int * four_bytes = (int *) malloc(4);
	//sets the value in the memory to zero (int 0 = "0000 0000") to prevent unexpected behavior
	*two_bytes = 0;
	*four_bytes = 0;
	//reads the memory, stores it as an int in the struct and reclears the memory
	fread((void *) two_bytes, 2, 1, fp);
	data->header = *two_bytes;
	*two_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->size = *four_bytes;
	*four_bytes = 0;
	fread((void *) two_bytes, 2, 1, fp);
	data->res_1 = *two_bytes;
	*two_bytes = 0;
	fread((void *) two_bytes, 2, 1, fp);
	data->res_2 = *two_bytes;
	*two_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->adress = *four_bytes;
	*four_bytes = 0;
	//frees the memory
	free(two_bytes);
	free(four_bytes);
}

void populate_dib_head(struct dib_header * data, FILE * fp){
	//allocates memory to store the metadata, this data comes in either 2 or 4 bytes.
	int * four_bytes = (int *) malloc(4);
	int * two_bytes = (int *) malloc(2);
	//sets the value in the memory to zero (int 0 = "0000 0000") to prevent unexpected behavior
	*four_bytes = 0;
	*two_bytes = 0;
	//reads the memory, stores it as an int in the struct and reclears the memory
	fread((void *) four_bytes, 4, 1, fp);
	data->size = *four_bytes;
	*four_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->b_width = *four_bytes;
	*four_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->b_height = *four_bytes;
	*four_bytes = 0;
	fread((void *) two_bytes, 2, 1, fp);
	data->clrpln = *two_bytes;
	*two_bytes = 0;
	fread((void *) two_bytes, 2, 1, fp);
	data->bitperpix = *two_bytes;
	*two_bytes = 0;
	///
	fread((void *) four_bytes, 4, 1, fp);
	data->comprmeth = *four_bytes;
	*four_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->imsize = *four_bytes;
	*four_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->horres = *four_bytes;
	*four_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->verres = *four_bytes;
	*four_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->colornum = *four_bytes;
	*four_bytes = 0;
	fread((void *) four_bytes, 4, 1, fp);
	data->impcolor = *four_bytes;
	*four_bytes = 0;
	//frees the memory
	free(four_bytes);
	free(two_bytes);
}

void scan_colors(int arraypos, int rowsize, int rowpadding, int width, int color, FILE * fp, FILE * fr, FILE * fg, FILE * fb){
	//allocates one byte of memory to be used as empty padding
	int * nul = (int *) malloc(1);
	*nul = 0;
	//allocates one byte of memory to be used as storage for one pixel color
	int * one_byte = (int *) malloc(1);
	*one_byte = 0;

	fseek(fp, arraypos, SEEK_SET);//sets the file position to the start of the pixel array
	printf("starting scanning and copying\n");
	//loops through every pixel
	for (int j = 0; j < width; ++j)
	{
		printf("\rCurrently handling row (%d/%d)", j + 1, width);//prints progress
		//scans and prints the pixels to the color files
		for (int i = 0; i < rowsize; ++i)
		{
			/*
				reads the pixel data and writes the correct color to the the correct file.
				The correct color is determined by the constant RGB sequence
				In case the color does not belong on a file, a "0" is written
			*/
			fread((void *) one_byte, 1, 1, fp);
			if(i % 3 == 2){
				//printf("(R:%x,", *one_byte);
				fwrite(one_byte, 1, 1, fr);
				fwrite(nul, 1, 1, fg);
				fwrite(nul, 1, 1, fb);
			}
			else if(i % 3 == 1){
				//printf("G:%x,", *one_byte);
				fwrite(nul, 1, 1, fr);
				fwrite(one_byte, 1, 1, fg);
				fwrite(nul, 1, 1, fb);
			}
			else{
				//printf("B:%x)\n", *one_byte);
				fwrite(nul, 1, 1, fr);
				fwrite(nul, 1, 1, fg);
				fwrite(one_byte, 1, 1, fb);
			}
			*one_byte = 0;
		}
		//adds extra padding if needed
		for (int i = 0; i < rowpadding; ++i)
		{
			fseek(fp, 1, SEEK_CUR);
			fwrite(nul, 1, 1, fr);
			fwrite(nul, 1, 1, fg);
			fwrite(nul, 1, 1, fb);
		}
	}
	free(one_byte);
	printf("\nFile scanned and copied");
}