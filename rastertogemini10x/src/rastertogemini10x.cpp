//----------------------------------------------------------------//
//                                                                //
//                                                                //                                                                                                                                
//                                                                //                                                               
//                                                                //                                                                                                                               
//                                                                //                                                                
//                                                                //                                                               
//----------------------------------------------------------------//


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cups/raster.h>
#include <stdio.h>
#include <stdlib.h>


//creates Printmatrix
typedef unsigned char ** MATRIZ_IMPRESION;
cups_page_header_t header;


void Empty(MATRIZ_IMPRESION &m, int cant_rows, int cant_cols)
{
	int f, c;
	
	for (f = 0; f < cant_rows; f++)
	{
		for (c = 0; c < cant_cols; c++)
		{
            m[f][c] = 0;
		}
	}
}

MATRIZ_IMPRESION CrearMatrizImpresion(int row_px, int col_px, int &rr, int &rc)
{
	int cant_fil = ((row_px % 8) == 0) ? row_px/8 : row_px/8 + 1;
	int cant_col = col_px;
	MATRIZ_IMPRESION temp = new unsigned char*[cant_fil];
	int i, j;
	for (i = 0; i < cant_col; i++)
	{
		temp[i] = new unsigned char[cant_col];
	}
//	fprintf(stderr,"Cant row = %i Real col = %i\n", cant_fil, cant_col);
	Empty(temp, cant_fil, cant_col);
	rr = cant_fil;
	rc = cant_col;
	return temp;
}

void InsertarPixel(MATRIZ_IMPRESION &m, int row, int col)
{
	char mask[8];
	mask[0] = 0x80;
	mask[1] = 0x40;
	mask[2] = 0x20;
	mask[3] = 0x10;
	mask[4] = 0x08;
	mask[5] = 0x04;
	mask[6] = 0x02;
	mask[7] = 0x01;
	int real_fil = (row / 8 > 0) ? row/8 : 0;
	if (row % 8 == 0) real_fil--;
	int real_col = col;

	int bit_index = ((row % 8) == 0) ? 7 : (row % 8)-1;

	m[real_fil][real_col] |= mask[bit_index];
}

int FilaEstaVacia(MATRIZ_IMPRESION m, int cant_cols, int quefila)
{
	int result = 0;
	for (int i = 0; i < cant_cols; i++)
	{
		if (m[quefila][i] != 0) result = i;
	}
	return result;
}

void PrintMatriz(MATRIZ_IMPRESION m, int cant_rows, int cant_cols, FILE * ff)
{
	int f, c;
	int num1, num2;
	int ancho;
	
	fputc(27, ff);	//reset the printer
	fputc('@',ff);
	fputc(27, ff);	//set the line feed to 24/144 inches
	fputc('3', ff);
	fputc(24, ff);
	fputc(27, ff);	//ignore "paper-out" signal
	fputc(56, ff);
	fputc(27, ff);	//put the header at the begining of the form
	fputc(82, ff);
	fputc(1, ff);

	for (f = 0; f < cant_rows; f++)
	{
		ancho = FilaEstaVacia(m, cant_cols, f);
		if (ancho != 0)
		{			
			fputc(27, ff);	//set the line feed to 24/144 inches
			fputc('3', ff);
			fputc(24, ff);
			
			fputc(27, ff);	//set resolution			
			switch (header.HWResolution[0])
			{
				case 120:
					fputc('L',ff);		//'L' high res 120x72 dpi
				break;
				case 60:
					fputc('K',ff);		//'K' low res 60x72 dpi
				break;
			}	
			
			num2 = ancho / 256;
			num1 = ancho % 256;		
						   
 			fputc(num1, ff); 	//ancho de la linea a imprimir num2 + num1*256
			fputc(num2, ff);
			for (c = 0; c < ancho; c++)
			{
				fputc(m[f][c], ff);
			}
		}
		fputc(10, ff);
	}
	fputc(7, ff); //beep, beep, beep
	fputc(7, ff);
	fputc(7, ff);
	fputc(27, ff); //restore "paper-out" signal
	fputc(57, ff);
}

int main(int argc, char **argv)
{
	
	int           fd;   /* File descriptor */
	cups_raster_t *ras; /* Raster stream for printing */
		
	if (argc < 6 || argc > 7)
	{

		fputs("ERROR: rastertogemini10x job-id user title copies options [file]\n", stderr);
		return (1);
	}
		
	if (argc == 7)
	{
		if ((fd = open(argv[6], O_RDONLY)) == -1)
		{
			perror("ERROR: Unable to open raster file - ");
			return (1);
		}
	}
	else
		fd = 0;
	
	ras = cupsRasterOpen(fd, CUPS_RASTER_READ);
	
	int                  y;
	unsigned char        data[8192];
	char printer_command[4];
	int rr, rc;
	int num_copias = atoi(argv[4]);
	int num_pages  = 1;
	MATRIZ_IMPRESION m;

	FILE * printer = stdout;
		
		
	while (cupsRasterReadHeader(ras, &header))
	{
//  ... initialize the printer ...
		m = CrearMatrizImpresion(header.cupsHeight, header.cupsWidth, rr, rc);
		
		fprintf(stderr,"DEBUG: HWResolution[0] = %i HWResolution[1] = %i\n", header.HWResolution[0], header.HWResolution[1]);
		fprintf(stderr,"DEBUG: cupsBitsPerColor =  %i cupsBytesPerLine = %i\n", header.cupsBitsPerColor, header.cupsBytesPerLine);
		fprintf(stderr,"DEBUG: cupsWidth = %i cupsHeight = %i\n", header.cupsWidth, header.cupsHeight);
		
		for (y = 0; y < header.cupsHeight; y++)
		{
		
			cupsRasterReadPixels(ras, data, header.cupsBytesPerLine);
			
//... send raster line to printer ...

			int i;
			for (i = 0; i < header.cupsBytesPerLine; i++)
			{
				int col = i * 8;
				if ((data[i] & 0x80) > 0)	InsertarPixel(m, y, col);
				if ((data[i] & 0x40) > 0)		InsertarPixel(m, y, col+1);
				if ((data[i] & 0x20) > 0)		InsertarPixel(m, y, col+2);
				if ((data[i] & 0x10) > 0)		InsertarPixel(m, y, col+3);
				if ((data[i] & 0x08) > 0)		InsertarPixel(m, y, col+4);
				if ((data[i] & 0x04) > 0)		InsertarPixel(m, y, col+5);
				if ((data[i] & 0x02) > 0)		InsertarPixel(m, y, col+6);
				if ((data[i] & 0x01) > 0)		InsertarPixel(m, y, col+7);
			}
		}
		PrintMatriz(m, rr, rc, printer);
		//imprimir una pagina
		fprintf(stderr, "PAGE: %i %i", num_pages ,num_copias);
		num_pages++;
	}
	cupsRasterClose(ras);
	fclose(printer);
	return(0);
}
