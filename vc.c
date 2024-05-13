//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de fun��es n�o seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Alocar mem�ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *)malloc(sizeof(IVC));

	if (image == NULL)
		return NULL;
	if ((levels <= 0) || (levels > 255))
		return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *)malloc(image->width * image->height * image->channels * sizeof(char));

	if (image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}

// Libertar mem�ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if (image != NULL)
	{
		if (image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;

	for (;;)
	{
		while (isspace(c = getc(file)))
			;
		if (c != '#')
			break;
		do
			c = getc(file);
		while ((c != '\n') && (c != EOF));
		if (c == EOF)
			break;
	}

	t = tok;

	if (c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while ((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if (c == '#')
			ungetc(c, file);
	}

	*t = 0;

	return tok;
}

long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}

void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = width * y + x;

			if (countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				// datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if ((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}

IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if ((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if (strcmp(tok, "P4") == 0)
		{
			channels = 1;
			levels = 1;
		} // Se PBM (Binary [0,1])
		else if (strcmp(tok, "P5") == 0)
			channels = 1; // Se PGM (Gray [0,MAX(level,255)])
		else if (strcmp(tok, "P6") == 0)
			channels = 3; // Se PPM (RGB [0,MAX(level,255)])
		else
		{
#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
#endif

			fclose(file);
			return NULL;
		}

		if (levels == 1) // PBM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			if ((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if (sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
				sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if (image == NULL)
				return NULL;

#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
#endif

			size = image->width * image->height * image->channels;

			if ((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
#endif
	}

	return image;
}

int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;

	if (image == NULL)
		return 0;

	if ((file = fopen(filename, "wb")) != NULL)
	{
		if (image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *)malloc(sizeofbinarydata);
			if (tmp == NULL)
				return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if (fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if (fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Funções para negativo da imagem
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para gerar negativo da imagem Gray ----- 1 canal
int vc_gray_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 1)
		return 0;

	// Inverte a imagem Grey
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}

// Função para gerar negativo da imagem RGB ---- 3 canais
int vc_rgb_negative(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 3)
		return 0;

	// Inverte a imagem RGB
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = 255 - data[pos];		 // Componente R
			data[pos + 1] = 255 - data[pos + 1]; // Componente G
			data[pos + 2] = 255 - data[pos + 2]; // Componente B
		}
	}

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Obter componentes de uma imagem RGB e converter em tons de cinza
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para obter o componente vermelho e convertê-lo em tons de cinza
int vc_rgb_get_red_gray(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 3)
		return 0;

	// Converte o componente vermelho em tons de cinza
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos + 1] = data[pos]; // Componente G
			data[pos + 2] = data[pos]; // Componente B
		}
	}

	return 1;
}

// Função para obter o componente verde e convertê-lo em tons de cinza
int vc_rgb_get_green_gray(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 3)
		return 0;

	// Converte o componente verde em tons de cinza
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = data[pos];	   // Componente R
			data[pos + 2] = data[pos]; // Componente B
		}
	}

	return 1;
}

// Função para obter o componente azul e convertê-lo em tons de cinza
int vc_rgb_get_blue_gray(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 3)
		return 0;

	// Converte o componente azul em tons de cinza
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data[pos] = data[pos];	   // Componente R
			data[pos + 1] = data[pos]; // Componente G
		}
	}

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Conversão de imagem RGB para imagem HSV
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para converter uma imagem RGB para uma imagem HSV --- 3 canais para 3 canais
int vc_rgb_to_hsv(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores máximo e mínimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0f)
		{
			hue = 0.0f;
			saturation = 0.0f;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0f)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}

		// Atribui valores entre [0,255]
		data[i] = (unsigned char)(hue / 360.0f * 255.0f);
		data[i + 1] = (unsigned char)(saturation);
		data[i + 2] = (unsigned char)(value);
	}

	return 1;
}

// Função para converter uma imagem HSV para uma imagem RGB --- 3 canais para 3 canais
int vc_hsv_to_rgb(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float hue, saturation, value, c, x, m;
	int i, size;

	// Verificação de erros
	if ((width <= 0) || (height <= 0) || (data == NULL))
		return 0;
	if (channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		hue = (float)data[i] / 255.0f * 360.0f;
		saturation = (float)data[i + 1] / 255.0f;
		value = (float)data[i + 2] / 255.0f;

		c = value * saturation;
		x = c * (1 - fabs(fmod(hue / 60.0f, 2) - 1));
		m = value - c;

		float r, g, b;
		if (hue >= 0 && hue < 60)
		{
			r = c;
			g = x;
			b = 0;
		}
		else if (hue >= 60 && hue < 120)
		{
			r = x;
			g = c;
			b = 0;
		}
		else if (hue >= 120 && hue < 180)
		{
			r = 0;
			g = c;
			b = x;
		}
		else if (hue >= 180 && hue < 240)
		{
			r = 0;
			g = x;
			b = c;
		}
		else if (hue >= 240 && hue < 300)
		{
			r = x;
			g = 0;
			b = c;
		}
		else
		{
			r = c;
			g = 0;
			b = x;
		}

		data[i] = (unsigned char)((r + m) * 255.0f);
		data[i + 1] = (unsigned char)((g + m) * 255.0f);
		data[i + 2] = (unsigned char)((b + m) * 255.0f);
	}

	return 1;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Segmentação de imagem HSV
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// // Função que receba uma imagem HSV e retorne uma imagem com 1 canal (admitindo valores entre 0 e 255 por pixel)
// 		// 255  -  360
// 		//  h    -  x
//         // x = 360*h / 255
int vc_hsv_segmentation(IVC *src, int hmin, int hmax, int smin,
						int smax, int vmin, int vmax)
{

	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	float h, s, v;
	int i, size;

	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 3)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{

		h = (float)data[i] * 360.0f / 255.0f;
		s = (float)data[i + 1] * 100.0f / 255.0f;
		v = (float)data[i + 2] * 100.0f / 255.0f;

		if (h >= hmin && h <= hmax && s >= smin && s <= smax && v >= vmin && v <= vmax)
		{
			data[i] = 255;
			data[i + 1] = 255;
			data[i + 2] = 255;
		}
		else
		{
			data[i] = 0;
			data[i + 1] = 0;
			data[i + 2] = 0;
		}
	}
	return 1;
}

int vc_3channels_to_1channel(IVC *src, IVC *dst)
{
	unsigned char *data_src = (unsigned char *)src->data;
	unsigned char *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;

	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 3)
		return 0;
	if ((dst->width) <= 0 || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if (dst->channels != 1)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			data_dst[y * width + x] = (unsigned char)(0.299 * data_src[pos] + 0.587 * data_src[pos + 1] + 0.114 * data_src[pos + 2]);
		}
	}

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Conversão de imagem Gray para imagem RGB
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para converter uma imagem Gray para uma imagem RGB
int vc_scale_gray_to_rgb(IVC *src, IVC *dst)
{
	int i, size;
	unsigned char *data_src, *data_dst;
	unsigned char red, green, blue;

	if (src == NULL || dst == NULL)
		return 0;
	if (src->channels != 1 || dst->channels != 3)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;

	size = src->width * src->height;
	data_src = (unsigned char *)src->data;
	data_dst = (unsigned char *)dst->data;

	for (i = 0; i < size; i++)
	{
		int d = i * 3;
		unsigned char intensity = data_src[i];

		// Regras de mapeamento conforme os slides
		if (intensity < 64)
		{ // Intensidade baixa -> Azul
			blue = 255;
			green = 4 * intensity;
			red = 0;
		}
		else if (intensity < 128)
		{ // Intensidade média-baixa -> Verde
			blue = 255 - 4 * (intensity - 64);
			green = 255;
			red = 0;
		}
		else if (intensity < 192)
		{ // Intensidade média-alta -> Verde
			blue = 0;
			green = 255;
			red = 4 * (intensity - 128);
		}
		else
		{ // Intensidade alta -> Vermelho
			blue = 0;
			green = 255 - 4 * (intensity - 192);
			red = 255;
		}

		data_dst[d] = red;
		data_dst[d + 1] = green;
		data_dst[d + 2] = blue;
	}

	return 1; // Sucesso
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Calcular pixeis dentro de uma imagem segmentada HSV
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para calcular pixeis dentro de uma imagem segmentada HSV
int vc_pixels_inside_segmented(IVC *srcdst, int *numPixels)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 3)
		return 0;

	// Calcula o número de pixeis dentro do cerebro
	*numPixels = 0;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			if (data[pos] == 255)
			{
				*numPixels = *numPixels + 1;
			}
		}
	}

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Converter uma imagem Gray para Binary
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para converter uma imagem Gray para Binary
int vc_gray_to_binary(IVC *srcdst, int threshold)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 3)
		return 0;

	// Converte a imagem Gray para Binary
	for (y = 0; y < height; y++)
	{ // Percorre a altura da imagem
		for (x = 0; x < width; x++)
		{										   // Percorre a largura da imagem
			pos = y * bytesperline + x * channels; // Posição do pixel -> Decorar equação
			if (data[pos] > threshold)
			{					 // Se o pixel for maior que o threshold
				data[pos] = 255; // Pixel branco
			}
			else
			{				   // Se o pixel for menor que o threshold
				data[pos] = 0; // Pixel preto
			}
		}
	}

	return 1;
}

// Função para converter uma imagem Gray para Binary com imagens de entrada e saída diferentes
int vc_gray_to_binary_src_dst(IVC *src, IVC *dst, int threshold)
{
	unsigned char *data_src = (unsigned char *)src->data;
	unsigned char *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 1)
		return 0;
	if ((dst->width) <= 0 || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if (dst->channels != 1)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;

	// Converte a imagem Gray para Binary
	for (y = 0; y < height; y++)
	{ // Percorre a altura da imagem
		for (x = 0; x < width; x++)
		{										   // Percorre a largura da imagem
			pos = y * bytesperline + x * channels; // Posição do pixel -> Decorar equação
			if (data_src[pos] > threshold)
			{						 // Se o pixel for maior que o threshold
				data_dst[pos] = 255; // Pixel branco
			}
			else
			{					   // Se o pixel for menor que o threshold
				data_dst[pos] = 0; // Pixel preto
			}
		}
	}

	return 1;
}

// Binarização automática é percorrer a imagem toda.
// E fazer o somatório da intensidade dos pixéis é dividir pelo numero total de pixéis existentes. (Média Global)
int vc_gray_to_binary_global_mean(IVC *srcdst)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;
	int sum = 0;
	int numPixels = width * height; // Número total de pixéis
	int threshold;

	// Verificação de erros
	if ((srcdst->width) <= 0 || (srcdst->height <= 0) || (srcdst->data == NULL))
		return 0;
	if (srcdst->channels != 1)
		return 0;

	// Calcula a média global
	for (y = 0; y < height; y++)
	{ // Percorre a altura da imagem
		for (x = 0; x < width; x++)
		{										   // Percorre a largura da imagem
			pos = y * bytesperline + x * channels; // Posição do pixel -> Decorar equação
			sum += data[pos];					   // Soma a intensidade do pixel, é igual a sum = sum + data[pos]
		}
	}

	threshold = sum / numPixels; // Calcula a média global

	printf("Threshold = %d\n", threshold); // Mostra o threshold calculado

	// Converte a imagem Gray para Binary
	for (y = 0; y < height; y++)
	{ // Percorre a altura da imagem
		for (x = 0; x < width; x++)
		{										   // Percorre a largura da imagem
			pos = y * bytesperline + x * channels; // Posição do pixel -> Decorar equação
			if (data[pos] > threshold)
			{					 // Se o pixel for maior que o threshold
				data[pos] = 255; // Pixel branco
			}
			else
			{				   // Se o pixel for menor que o threshold
				data[pos] = 0; // Pixel preto
			}
		}
	}

	return 1;
}

// Binarização automática com metodo adaptativo (midpoint)
int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel)
{

	unsigned char *data_src = (unsigned char *)src->data;
	unsigned char *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2;
	int min, max;
	long int pos, posk;
	unsigned char threshold;

	// Verificação de erros
	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 1)
		return 0;
	if ((dst->width) <= 0 || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if (dst->channels != 1)
		return 0;
	if (src->width != dst->width || src->height != dst->height)
		return 0;

	// Converte a imagem Gray para Binary
	for (y = 0; y < height; y++)
	{ // Percorre a altura da imagem
		for (x = 0; x < width; x++)
		{										   // Percorre a largura da imagem
			pos = y * bytesperline + x * channels; // Posição do pixel -> Decorar equação -> Só para usar no threshold
			min = 255;
			max = 0;

			for (ky = -offset; ky <= offset; ky++)
			{ // Percorre a altura do kernel
				for (kx = -offset; kx <= offset; kx++)
				{ // Percorre a largura do kernel

					if (y + ky >= 0 && y + ky < height && x + kx >= 0 && x + kx < width)
					{ // Se a posição do pixel for válida (dentro da imagem)

						posk = (y + ky) * bytesperline + (x + kx) * channels; // Posição do pixel do kernel
						if (data_src[posk] < min)
							min = data_src[posk]; // Se a intensidade do pixel for menor que o min
						if (data_src[posk] > max)
							max = data_src[posk]; // Se a intensidade do pixel for maior que o max
					}
				}
			}

			threshold = (min + max) / 2; // Calcula o threshold com formula do midpoint
			if (data_src[pos] > threshold)
			{						 // Se o pixel for maior que o threshold calculado com o midpoint
				data_dst[pos] = 255; // Pixel branco
			}
			else
			{					   // Se o pixel for menor que o threshold
				data_dst[pos] = 0; // Pixel preto
			}
		}
	}
}

// Binarização automática com metodo adaptativo (bernsen)
int vc_gray_to_binary_bersen(IVC *src, IVC *dst, int kernel, int cmin)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;

	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int bytesperline = src->width * src->channels;

	int x, y, kx, ky;
	int offset = (kernel - 1) / 2;
	int min, max;
	long int pos, kpos;

	unsigned char threshold;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if ((dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			min = 255;
			max = 0;

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						kpos = (y + ky) * bytesperline + (x + kx) * channels;

						if (datasrc[kpos] < min)
							min = datasrc[kpos];
						if (datasrc[kpos] > max)
							max = datasrc[kpos];
					}
				}
			}

			if ((max - min) < cmin)
				threshold = src->levels / 2;
			else
				threshold = ((float)(max + min) / (float)2);

			if (datasrc[pos] <= threshold)
				datadst[pos] = 0;
			else
				datadst[pos] = 255;
		}
	}

	return 1;
}

// Binarização automática com metodo adaptativo (niblack)
int vc_gray_to_binary_niblack(IVC *src, IVC *dst, int kernel, float k)
{

	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int bytesperline = src->bytesperline;
	int x, y, kx, ky;
	int offset = (kernel - 1) / 2;
	int max, min;
	long int pos, kpos;
	float mean, stdDev;
	unsigned char threshold;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if ((dst->channels != 1))
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			max = datasrc[pos];
			min = datasrc[pos];

			float mean = 0.0f;

			// Counter = (kernel * kernel)

			// Vizinhos
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						kpos = (y + ky) * bytesperline + (x + kx) * channels;

						if (datasrc[kpos] < min)
							min = datasrc[kpos];
						if (datasrc[kpos] > max)
							max = datasrc[kpos];

						mean += datasrc[kpos];
					}
				}
			}

			mean = mean / (kernel * kernel);

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						kpos = (y + ky) * bytesperline + (x + kx) * channels;

						stdDev += pow(datasrc[kpos] - mean, 2);
					}
				}
			}

			stdDev = sqrt(stdDev / (kernel * kernel));

			threshold = mean + k * stdDev;

			if (datasrc[pos] <= threshold)
				datadst[pos] = 0;
			else
				datadst[pos] = 255;
		}
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Operadores Morfológicos em imagens binárias
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para aplicar um operador morfológico de Dilatação em imagens binárias
int vc_binary_dilate2(IVC *src, IVC *dst, int kernelSize)
{

	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int bytesperline = src->bytesperline;
	int x, y, i, j, kx, ky;
	int offset;
	long int pos, kpos;
	unsigned char pixel;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if ((dst->channels != 1))
		return 0;

	offset = (kernelSize - 1) / 2;

	// Copia a imagem original para a imagem destino
	// Imagem de destino é igual a imagem original
	memcpy(datadst, datasrc, bytesperline * height);

	// Calcula da erosão
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = datasrc[pos];

			for (ky = -offset; ky <= offset; ky++)
			{
				j = y + ky;

				if ((j < 0) && (j >= height))
					continue; // Operador lógico AND

				for (kx = -offset; kx <= offset; kx++)
				{
					i = x + kx;

					if ((i < 0) && (i >= width))
						continue; // Operador lógico AND

					kpos = j * bytesperline + i * channels;

					pixel |= datasrc[kpos]; // Operador lógico OR =
				}
			}

			// Se um qualquer pixel da vizinhança for zero então..
			if (pixel == 0)
				datadst[pos] = 0; // Pixel preto
			else
				datadst[pos] = 255; // Pixel branco
		}
	}
}

int vc_binary_dilate(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	long int pos, posk;
	int x, y, ky, kx;
	int offset = (kernel - 1) / 2;

	// Verificação de erros
	if ((src->width) <= 0 || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->channels != 1)
		return 0;

	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x)
		{
			pos = y * bytesperline + x * channels;
			int foundWhite = 0;

			// NxM vizinhança
			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;
						if (datasrc[posk] == 255)
						{
							foundWhite = 255;
							break;
						}
					}
				}
				if (foundWhite)
					break;
			}
			datadst[pos] = foundWhite;
		}
	}
	return 1;
}

// Função para aplicar um operador morfológico de Erosão em imagens binárias
int vc_binary_erode(IVC *src, IVC *dst, int kernelSize)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int channels = src->channels;
	int bytesperline = src->bytesperline;
	int x, y, i, j, kx, ky;
	int offset;
	long int pos, kpos;
	unsigned char pixel;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if ((dst->channels != 1))
		return 0;

	offset = (kernelSize - 1) / 2;

	// Copia a imagem original para a imagem destino
	// Imagem de destino é igual a imagem original
	memcpy(datadst, datasrc, bytesperline * height);

	// Calcula da erosão
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			pixel = datasrc[pos];

			for (ky = -offset; ky <= offset; ky++)
			{
				j = y + ky;

				if ((j < 0) || (j >= height))
					continue; // Operador lógico OU

				for (kx = -offset; kx <= offset; kx++)
				{
					i = x + kx;

					if ((i < 0) || (i >= width))
						continue; // Operador lógico OU

					kpos = j * bytesperline + i * channels;

					pixel &= datasrc[kpos]; // Operador lógico AND =
				}
			}

			// Se um qualquer pixel da vizinhança for zero então..
			if (pixel == 0)
				datadst[pos] = 0; // Pixel preto
			else
				datadst[pos] = 255; // Pixel branco
		}
	}
}

// Função para aplicar uma abertura morfológica em imagens binárias (1º Erosão, 2º Dilatação)
int vc_binary_open(IVC *src, IVC *dst, int sizeerode, int sizedilate)
{
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if (tmp == NULL)
		return 0;

	if (vc_binary_erode(src, tmp, sizeerode) == 0)
	{
		vc_image_free(tmp);
		return 0;
	}

	if (vc_binary_dilate(tmp, dst, sizedilate) == 0)
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);

	return 1;
}

// Função para aplicar um fecho morfológico em imagens binárias (1º Dilatação, 2º Erosão)
int vc_binary_close(IVC *src, IVC *dst, int sizedilate, int sizeerode)
{
	IVC *tmp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if (tmp == NULL)
		return 0;

	if (vc_binary_dilate(src, tmp, sizedilate) == 0)
	{
		vc_image_free(tmp);
		return 0;
	}

	if (vc_binary_erode(tmp, dst, sizeerode) == 0)
	{
		vc_image_free(tmp);
		return 0;
	}

	vc_image_free(tmp);

	return 1;
}

int vc_binary_sub(IVC *imagem1, IVC *imagem2, IVC *destino)
{

	unsigned char *data1 = (unsigned char *)imagem1->data;
	unsigned char *data2 = (unsigned char *)imagem2->data;
	unsigned char *data_destino = (unsigned char *)destino->data;
	int width = imagem1->width;
	int height = imagem1->height;
	int bytesperline = imagem1->width * imagem1->channels;
	int channels = imagem1->channels;
	int i, size;

	if ((imagem1->width) <= 0 || (imagem1->height <= 0) || (imagem1->data == NULL))
		return 0;
	if ((imagem2->width) <= 0 || (imagem2->height <= 0) || (imagem2->data == NULL))
		return 0;
	if ((destino->width) <= 0 || (destino->height <= 0) || (destino->data == NULL))
		return 0;
	if (imagem1->channels != 1 || imagem2->channels != 1 || destino->channels != 1)
		return 0;

	size = width * height * channels;

	for (i = 0; i < size; i = i + channels)
	{
		if (data1[i] == 255 && data2[i] == 0)
		{
			data_destino[i] = 255;
		}
		else
		{
			data_destino[i] = 0;
		}
	}
	return 1;
}

/// @brief Função que faz a erosão de uma imagem em escala de cinzentos
/// Basta existir um pixel na vizinhança a zero para que o pixel central ser 255
/// @param src
/// @param dst
/// @param kernel
/// @return
int vc_gray_erode(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, kx, ky, min, max;
	long int pos, posk;
	int offset = (kernel - 1) / 2; // Calculo do valor do offset

	if (dst->channels != 1)
	{
		printf("Imagen secundária tem mais do que 1 canal");
		return 0;
	}
	if (src->height <= 0 || src->width <= 0 || src->height <= 0 || src->width <= 0 || src == NULL || dst == NULL)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0; // Verifica se as imagens têm os canais corretos

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			max = 0;
			min = 255;

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						// if(datasrc[posk]>max) max=datasrc[posk];
						if (datasrc[posk] < min)
							min = datasrc[posk];
					}
				}
			}

			datadst[pos] = min;
		}
	}
	return 1;
}

int vc_gray_dilate(IVC *src, IVC *dst, int kernel)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y, kx, ky, min, max;
	long int pos, posk;
	int offset = (kernel - 1) / 2; // Calculo do valor do offset

	if (dst->channels != 1)
	{
		printf("Imagen secundária tem mais do que 1 canal");
		return 0;
	}
	if (src->height <= 0 || src->width <= 0 || src->height <= 0 || src->width <= 0 || src == NULL || dst == NULL)
		return 0;
	if (src->channels != 1 || dst->channels != 1)
		return 0; // Verifica se as imagens têm os canais corretos

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			int flag = 0;
			max = 0;
			min = 255;

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					if ((y + ky >= 0) && (y + ky < height) && (x + kx >= 0) && (x + kx < width))
					{
						posk = (y + ky) * bytesperline + (x + kx) * channels;

						if (datasrc[posk] > max)
							max = datasrc[posk];
					}
				}
			}

			datadst[pos] = max;
		}
	}
	return 1;
}

/// @brief Erosão -> Dilatação para cinzentos
/// @param src
/// @param dst
/// @param kernel
/// @return
int vc_gray_open(IVC *src, IVC *dst, int kernelE, int kernelD)
{
	// precisamos de criar uma imagem temporária para poder passar
	IVC *temp = vc_image_new(dst->width, dst->height, dst->channels, dst->levels);

	vc_gray_erode(src, temp, kernelE);

	vc_gray_dilate(temp, dst, kernelD);

	vc_image_free(temp);
	printf("Abertuta concluida\n");

	return 1;
}

/// @brief Dilatação->Erosão para cinzentos
/// @param src
/// @param dst
/// @param kernel
/// @return
int vc_gray_close(IVC *src, IVC *dst, int kernelE, int kernelD)
{

	IVC *temp = vc_image_new(dst->width, dst->height, dst->channels, dst->levels);

	vc_gray_dilate(src, temp, kernelD);

	vc_gray_erode(temp, dst, kernelE);

	vc_image_free(temp);
	printf("Fecho concluido\n");

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Subtrair 2 Imagens binarias
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_binary_subtract(IVC *src1, IVC *src2, IVC *dst)
{
	unsigned char *data_src1 = (unsigned char *)src1->data;
	unsigned char *data_src2 = (unsigned char *)src2->data;
	unsigned char *data_dst = (unsigned char *)dst->data;
	int width = src1->width;
	int height = src1->height;
	int bytesperline = src1->width * src1->channels;
	int channels = src1->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((src1->width <= 0) || (src1->height <= 0) || (src1->data == NULL))
		return 0;
	if ((src2->width <= 0) || (src2->height <= 0) || (src2->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if (src1->width != src2->width || src1->height != src2->height || src1->channels != src2->channels)
		return 0;
	if (src1->width != dst->width || src1->height != dst->height || src1->channels != dst->channels)
		return 0;

	// Subtrai as duas imagens binárias
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;
			if (data_src1[pos] > 0 && data_src2[pos] == 0)
			{
				data_dst[pos] = 255;
			}
			else
			{
				data_dst[pos] = 0;
			}
		}
	}

	return 1;
}

int vc_exclude_regions(IVC *original, IVC *mask, IVC *final_image)
{
	if (original == NULL || mask == NULL || final_image == NULL)
		return 0;
	if (original->width != mask->width || original->height != mask->height || original->channels != mask->channels)
		return 0;

	for (int y = 0; y < original->height; y++)
	{
		for (int x = 0; x < original->width; x++)
		{
			for (int c = 0; c < original->channels; c++)
			{
				int pos = y * original->bytesperline + x * original->channels + c;
				int pixel_value = (int)original->data[pos];

				// Se o pixel correspondente na imagem de máscara for 0 (preto), mantenha o valor original do pixel
				// Se o pixel correspondente na imagem de máscara for 255 (branco), defina o valor do pixel como 0 (preto) na imagem final
				if (mask->data[pos] == 0)
				{
					final_image->data[pos] = 0;
				}
				else
				{
					final_image->data[pos] = pixel_value;
				}
			}
		}
	}

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Labeling de blobs
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Etiquetagem de blobs
// src		: Imagem bin�ria de entrada
// dst		: Imagem grayscale (ir� conter as etiquetas)
// nlabels	: Endere�o de mem�ria de uma vari�vel, onde ser� armazenado o n�mero de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. � necess�rio libertar posteriormente esta mem�ria.
OVC *vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels) // identifica os blobs apenas
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = {0};
	int labelarea[256] = {0};
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC *blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels))
		return NULL;
	if (channels != 1)
		return NULL;

	// Copia dados da imagem bin�ria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
	// Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
	// Ser�o atribu�das etiquetas no intervalo [1,254]
	// Este algoritmo est� assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0)
			datadst[i] = 255;
	}

	// Limpa os rebordos da imagem bin�ria
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels;		// B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels;		// D
			posX = y * bytesperline + x * channels;				// X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A est� marcado
					if (datadst[posA] != 0)
						num = labeltable[datadst[posA]];
					// Se B est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num))
						num = labeltable[datadst[posB]];
					// Se C est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num))
						num = labeltable[datadst[posC]];
					// Se D est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num))
						num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	// printf("\nMax Label = %d\n", label);

	// Contagem do n�mero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b])
				labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++;						  // Conta etiquetas
		}
	}

	// Se n�o h� blobs
	if (*nlabels == 0)
		return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++)
			blobs[a].label = labeltable[a];
	}
	else
		return NULL;

	return blobs;
}

int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs) // os blobs acima indentificados sao um corpo
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Conta �rea de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// �rea
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x)
						xmin = x;
					if (ymin > y)
						ymin = y;
					if (xmax < x)
						xmax = x;
					if (ymax < y)
						ymax = y;

					// Per�metro
					// Se pelo menos um dos quatro vizinhos n�o pertence ao mesmo label, ent�o � um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		// blobs[i].xc = (xmax - xmin) / 2;
		// blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MY_MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MY_MAX(blobs[i].area, 1);
	}

	return 1;
}

// Funcao para normalizar a imagem com labels e diferenes escalas de cinzas
int vc_normalizar_imagem_labelling_tonsgray(IVC *src, IVC *dst, int nblobs)
{
	unsigned char *data_src = (unsigned char *)src->data;
	unsigned char *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{

			pos = y * bytesperline + x * channels;

			for (i = 1; i <= nblobs; i++)
			{ // i = labels
				if (data_src[pos] == i)
				{
					data_dst[pos] = (i * 255) / nblobs; // Pinta os objetos de acordo com o numero de blobs --- Ex: layer 2 * 255 / 3 = 170
				}
			}
		}
	}
}

// Função para normalizar a imagem com labels e diferentes escalas de cinzas para branca e preta
int vc_normalizar_imagem_labelling_pretoebranco(IVC *src, IVC *dst, int nblobs)
{
	unsigned char *data_src = (unsigned char *)src->data;
	unsigned char *data_dst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
		return 0;
	if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
		return 0;
	if (channels != 1)
		return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{

			pos = y * bytesperline + x * channels;

			for (i = 1; i <= nblobs; i++)
			{ // i = labels
				if (data_src[pos] == i)
				{
					data_dst[pos] = 255; // Pinta os objetos de acordo com o numero de blobs com o valor 255 (branco)
				}
			}
		}
	}
}

// vc_draw_boundingbox(ImgLabelling, &blobs[i]);
int vc_draw_boundingbox(IVC *src, OVC *blob)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Desenha a bounding box
	for (y = blob->y; y < blob->y + blob->height; y++)
	{
		for (x = blob->x; x < blob->x + blob->width; x++)
		{
			if (y == blob->y || y == blob->y + blob->height - 1 || x == blob->x || x == blob->x + blob->width - 1)
			{
				pos = y * bytesperline + x * channels;
				data[pos] = 255;
			}
		}
	}

	return 1;
}

// vc_draw_centerofgravity(ImgLabelling, &blobs[i]);
int vc_draw_centerofgravity(IVC *src, OVC *blob)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
		return 0;
	if (channels != 1)
		return 0;

	// Desenha o centro de gravidade
	pos = blob->yc * bytesperline + blob->xc * channels;
	data[pos] = 255;

	return 1;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: Histogramas
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Função para imprimir um histograma de uma imagem em tons de cinzento
int vc_gray_histogram_show(IVC *src, IVC *dst)
{
    unsigned char *data_src = (unsigned char *)src->data;
    unsigned char *data_dst = (unsigned char *)dst->data;
    int width = 256; // Largura fixa para o histograma
    int height = dst->height;
    int bytesperline = dst->bytesperline;
    int channels = dst->channels;
    int x, y, i;
    long int pos;
    int max = 0;
    int hist[256] = {0}; // Iniciar array a 0 (vai de 0 a 255)

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;
    if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
        return 0;
    if (src->channels != 1 || dst->channels != 1)
        return 0;

    // Contagem : hist[data_src[i]]+= 1
    for (i = 0; i < src->width * src->height; i++)
    {
        hist[data_src[i]] += 1;
    }

    // Obter max do hist
    for (i = 0; i < 256; i++)
    {
        if (hist[i] > max)
            max = hist[i];
    }

    // Normalização : (hist / max) * height (Quant. Pixeis)
    for (i = 0; i < 256; i++)
    {
        hist[i] = (hist[i] * height) / max;
    }

    // Calcular a posição inicial para centralizar o histograma
    int start_x = (dst->width - width) / 2;

    // Preencher as colunas do histograma (imagem destino)
    // Começar com o 0 em cima e ir até ao valor do histograma
    // Fazer histograma de cima para baixo
    for (x = 0; x < 256; x++)
    {
        for (y = height - 1; y >= 0; y--) // Começa no topo
        {
            if (y >= height - hist[x])
            {
                pos = y * bytesperline + (start_x + x) * channels; // Adicionar start_x
                data_dst[pos] = 255;
            }
            else
            {
                pos = y * bytesperline + (start_x + x) * channels; // Adicionar start_x
                data_dst[pos] = 0;
            }
        }
    }

    // imprimir o maximo do histograma
    printf("Maximo do histograma: %d\n", max);
    return 1;
}

// Função para equalizar o histograma de uma imagem em tons de cinzento
int vc_gray_histogram_equalization(IVC *src, IVC *dst)
{
    unsigned char *data_src = (unsigned char *)src->data;
    unsigned char *data_dst = (unsigned char *)dst->data;
    int width = src->width;
    int height = src->height;
    int bytesperline = src->bytesperline;
    int channels = src->channels;
    int x, y, i;
    long int pos;
    int hist[256] = {0}; // Histograma
    int cdf[256] = {0};  // Função de distribuição cumulativa
    int equalization[256] = {0}; // Mapeamento de equalização
    int min_cdf = -1; // Valor mínimo do CDF
    int max_cdf = 0;  // Valor máximo do CDF

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))
        return 0;
    if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL))
        return 0;
    if (src->width != dst->width || src->height != dst->height || src->channels != dst->channels)
        return 0;
    if (channels != 1)
        return 0;

    // Contagem do histograma
    for (i = 0; i < width * height; i++)
    {
        hist[data_src[i]]++;
    }

    // Cálculo da função de distribuição cumulativa (CDF)
    cdf[0] = hist[0];
    for (i = 1; i < 256; i++)
    {
        cdf[i] = cdf[i - 1] + hist[i];
    }

    // Encontra o valor mínimo e máximo do CDF
    for (i = 0; i < 256; i++)
    {
        if (cdf[i] > 0 && min_cdf == -1) {
            min_cdf = cdf[i];
        }
        if (cdf[i] > max_cdf) {
            max_cdf = cdf[i];
        }
    }

    // Cálculo do mapeamento de equalização
    for (i = 0; i < 256; i++)
    {
        equalization[i] = (int)(((float)cdf[i] - min_cdf) / ((float)max_cdf - min_cdf) * (src->levels - 1));
    }

    // Aplica a equalização aos pixels da imagem de entrada
    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            pos = y * bytesperline + x * channels;
            data_dst[pos] = equalization[data_src[pos]];
        }
    }

    return 1;
}