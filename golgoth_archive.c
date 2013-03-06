/*
  �����F
     golgoth studio���p����A�[�J�C�u���𓀂���

  �ŏI�X�V���F2012�N11��17��
  �쐬���@�@�F2012�N11��17��
  �쐬�ҁ@�@�Fgbm
*/


/****************************************************/
/*                     include                      */
/****************************************************/
#include <stdio.h>
#include <stdlib.h> // exit
#include <string.h> // memset
#include <getopt.h> // getopt_long



/****************************************************/
/*                      define                      */
/****************************************************/
//#define DEBUG_ON

#define RET_OK 0
#define RET_ERROR -1
#define RET_FAILURE 1

#define FOUR_BYTE 0x04

#define FILE_EXTENSION_MAX 32
#define DEFAULT_FILENAME "output"
#define NUM_FORMAT "%03d"

#define GARC_SIZE_INDEX  0x10
#define GARC_SIZE_HEADER 0x08

#define GARC_ID "gfp0"
#define GARC_HEADER_ID_SIZE 4

#define LONGOPT_EXTENSION 0          // long opt num
#define OPTFLAG_EXTENSION 0x01       // optflag

#define LONGOPT_OUTPUT_FILENAME 1    // long opt num
#define OPTFLAG_OUTPUT_FILENAME 0x02 // optflag

#define LONGOPT_VERBOSE 2            // long opt num
#define OPTFLAG_VERBOSE 0x04         // optflag

#define REVERSE_ENDIAN(n)				\
	(									\
		  ((n & 0xFF000000) >> 24)		\
		| ((n & 0x00FF0000) >> 8)		\
		| ((n & 0x0000FF00) << 8)		\
		| ((n & 0x000000FF) << 24)		\
	)

/****************************************************/
/*                      struct                      */
/****************************************************/

/* GARCHEADER ************************

	 �擪8�o�C�g�Œ�
     �E���ʎq      "gfp0"
	 �E�t�@�C����  4byte

**************************************/
typedef struct {
	unsigned char id[GARC_HEADER_ID_SIZE];
	unsigned int num;
}GARCHEADER;

/* GARCINDEX *************************

     16�o�C�g*�t�@�C�������C���f�b�N�X
	 �i�[�A�h���X�̓w�b�_�ƃC���f�b�N�X�������Čv�Z�����
     �E�`�F�b�N�T��    4byte
	 �E�i�[�A�h���X    4byte
	 �E�t�@�C���T�C�Y  4byte

**************************************/
typedef struct {
	unsigned int checksum;
	unsigned int address;
	unsigned int size;
	unsigned int padding;
}GARCINDEX;


/****************************************************/
/*                   prototype                      */
/****************************************************/
int golgoth_archive_extract(FILE *fpr, const GARCHEADER *header);

int fncopy(FILE *fpw, FILE *fpr, size_t n);
int make_filename(char *filename, const char *name, const char *extension, unsigned int num);

int read_golgoth_header(GARCHEADER *header, FILE *fp);
int read_golgoth_index(GARCINDEX *index, FILE *fp);
int seek_address(FILE *fp, const GARCHEADER *header, const GARCINDEX *index);

int check_golgoth_id(const GARCHEADER *header);

/****************************************************/
/*                     Global                       */
/****************************************************/
static unsigned char g_flag = 0;
static char g_filename[FILENAME_MAX];
static char g_extension[FILE_EXTENSION_MAX];



/****************************************************/
/*                    Process                       */
/****************************************************/

// Usage
void usage(const char *this) {
	fprintf(stderr, "Usage: %s [option] filename\n", this);
	fprintf(stderr, "  -e FILE_EXTENSION, --extension FILE_EXTENSION: set file extension\n");
	fprintf(stderr, "  -n FILENAME, --filename FILENAME: set output filename.\n");
	fprintf(stderr, "      output -> FILENAME_001[.EXT] FILENAME_002[.EXT]... \n");
	fprintf(stderr, "  -v, --verbose : Verbose mode.\n");
	exit(EXIT_FAILURE);
}


/* main *************************************************************
    golgoth studio���p����A�[�J�C�u���𓀂���

	�����̃t�@�C����A�Ԃŏo�͂���
	�t�@�C���^�C�v(�g���q)���I�v�V�����Ŏw�肷�邱�Ƃ��\
********************************************************************/
int main(int argc, char *argv[]) {
	FILE *fpr = NULL;
	GARCHEADER header;

	// getopt_long
	struct option options[] = {
		{"extension", 0, 0, 0},
		{"filename", 0, 0, 0},
		{"verbose", 0, 0, 0},
		{0, 0, 0, 0}
	};
	int opt;
	int optindex;

	if (sizeof(unsigned int) != FOUR_BYTE) {
		fprintf(stderr, "compile error\n");
		return EXIT_FAILURE;
	}

	// ������
	memset(g_filename,  '\0', FILENAME_MAX);
	memset(g_extension, '\0', FILE_EXTENSION_MAX);

	// option���
	while ((opt = getopt_long(argc, argv, "e:n:v", options, &optindex)) != -1){
		switch (opt){
		case 0: //long opt
#ifdef DEBUG_ON
			printf("optindex = %d\n", optindex);
#endif
			switch (optindex){
			case LONGOPT_EXTENSION:
				g_flag |= OPTFLAG_EXTENSION;
				if (!optarg)
					usage(argv[0]);
				strncpy(g_extension, optarg, FILE_EXTENSION_MAX);
				break;
			case LONGOPT_OUTPUT_FILENAME:
				g_flag |= OPTFLAG_OUTPUT_FILENAME;
				if (!optarg)
					usage(argv[0]);
				strncpy(g_filename, optarg, FILENAME_MAX);
				break;
			case LONGOPT_VERBOSE:
				g_flag |= OPTFLAG_VERBOSE;
				break;
			default:
				break;
			}
			break;
		case 'e': // extension opt
			g_flag |= OPTFLAG_EXTENSION;
			if (!optarg)
				usage(argv[0]);
			strncpy(g_extension, optarg, FILE_EXTENSION_MAX);
			break;
		case 'n': // output filename opt
			g_flag |= OPTFLAG_OUTPUT_FILENAME;
			if (!optarg)
				usage(argv[0]);
			strncpy(g_filename, optarg, FILENAME_MAX);
			break;
		case 'v': // verbose opt
			g_flag |= OPTFLAG_VERBOSE;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}
#ifdef DEBUG_ON
	printf("OPT = %02X\n",  g_flag);
	printf("OPTARG = %s\n", g_extension);
#endif

	// file open
	if (optind >= argc) usage(argv[0]); // to exit
	else {
		fpr = fopen(argv[optind], "rb");
		if (fpr == NULL) {
			fprintf(stderr, "file open error : %s\n", argv[optind]);
			usage(argv[0]);
		}
	}

	// golgoth_archive�`���̃t�@�C�����`�F�b�N����
	if (RET_ERROR == read_golgoth_header(&header, fpr)) goto MAIN_EXIT_FAILURE;
	if (RET_ERROR == check_golgoth_id(&header))         goto MAIN_EXIT_FAILURE;

	// �A�[�J�C�u��W�J����
	if (RET_ERROR == golgoth_archive_extract(fpr, &header)) goto MAIN_EXIT_FAILURE;


  MAIN_EXIT_SUCCESS:
	if(fpr != NULL) fclose(fpr);
	return EXIT_SUCCESS;

  MAIN_EXIT_FAILURE:
	if(fpr != NULL) fclose(fpr);
	return EXIT_FAILURE;
}


/* golgoth_archive_extract ****************************
   golgoth_archive�̉𓀏������s��

   �߂�l�F�G���[,RET_ERROR
*******************************************************/
int golgoth_archive_extract(FILE *fpr, const GARCHEADER *header) {
	int i;
	char filename[FILENAME_MAX];
	char *ext = NULL;
	FILE *fpw = NULL;
	GARCINDEX *pindex = NULL;

	if (g_flag & OPTFLAG_EXTENSION) ext = g_extension;

	// �C���f�b�N�X��ǂݍ��ރ��������m��
	pindex = (GARCINDEX*) malloc((size_t)(sizeof(GARCINDEX) * header->num));
	if (pindex == NULL) RET_ERROR;

	// �C���f�b�N�X��S�ēǂݍ���
	for (i = 0; i < header->num; i++)
		if (RET_ERROR == read_golgoth_index(pindex+i, fpr)) goto EXTRACT_ERROR;

	// �C���f�b�N�X�̏��ɏ]���ăt�@�C����W�J����
	for (i = 0; i < header->num; i++) {
		// �W�J��t�@�C���̍쐬
		if (RET_ERROR == make_filename(filename, g_filename, ext, i)) goto EXTRACT_ERROR;
		fpw = fopen(filename, "wb");
		if (fpw == NULL) {
			fprintf(stderr, "file open error : %s\n", filename);
			goto EXTRACT_ERROR;
		}

#ifdef DEBUG_ON
	printf("i = %d  ", i);
#endif
		// �W�J
		if (RET_ERROR == seek_address(fpr, header, pindex+i)) goto EXTRACT_ERROR;
		if (RET_ERROR == fncopy(fpw, fpr, pindex[i].size))    goto EXTRACT_ERROR;
		printf("output -> %s %dbyte\n", filename, pindex[i].size);
		if (fpw != NULL) fclose(fpw);
	}

  EXTRACT_OK:
	free(pindex);
	return RET_OK;

  EXTRACT_ERROR:
	free(pindex);
	if(fpw != NULL) fclose(fpw);
	return RET_ERROR;
}


/* fncopy *********************************************
   fpr�̒��g�� n byte fpw�ɃR�s�[����B

   �߂�l�F�G���[,RET_ERROR
*******************************************************/
int fncopy(FILE *fpw, FILE *fpr, size_t n) {
	int i;
	char buf;

	if ((fpr == NULL) || (fpw == NULL)) return RET_ERROR;

	for (i = 0; i < n; i++) {
		if (1 != fread(&buf, sizeof(char), 1, fpr)) return RET_ERROR;
		if (1 != fwrite(&buf, sizeof(char), 1, fpw)) return RET_ERROR;
	}

	return RET_OK;
}


/* make_filename **************************************
   name_num[.ext]�`���̖��O��filename�ɃZ�b�g����
   extension��NULL�̏ꍇ�ɂ�name_num�ƂȂ�

   �߂�l�F�G���[,RET_ERROR
*******************************************************/
int make_filename(char *filename, const char *name, const char *extension, unsigned int num) {
	if (filename == NULL) return RET_ERROR;
	if (name == NULL) return RET_ERROR;

	if (extension != NULL)
		snprintf(filename, FILENAME_MAX, "%s_" NUM_FORMAT ".%s", name, num, extension);
	else
		snprintf(filename, FILENAME_MAX, "%s_" NUM_FORMAT, name, num);

#ifdef DEBUG_ON
	printf("filename = %s\n", filename);
#endif

	return RET_OK;
}


/* read_golgoth_header **************
   header�Ɋe�f�[�^��ǂݍ���

   �߂�l�F����ł����RET_OK
   ���ӁFread�֐���fpos���ړ�������
*********************************/
int read_golgoth_header(GARCHEADER *header, FILE *fp) {
	if (fp == NULL) return RET_ERROR;

	// id
	if (0 >= fread(header->id, sizeof(header->id), 1, fp)) return RET_ERROR;

	// num
	if (0 >= fread(&(header->num), FOUR_BYTE, 1, fp)) return RET_ERROR;

#ifdef DEBUG_ON
	printf("id = %c%c%c%c\n", header->id[0], header->id[1], header->id[2], header->id[3]);
	printf("num = %08X\n", header->num);
#endif

	return RET_OK;
}


/* read_golgoth_index **************
   index�Ɋe�f�[�^��ǂݍ���

   �߂�l�F����ł����RET_OK
   ���ӁFread�֐���fpos���ړ�������
*********************************/
int read_golgoth_index(GARCINDEX *index, FILE *fp) {
	if (fp == NULL) return RET_ERROR;

	// checksum
	if (0 >= fread(&(index->checksum), FOUR_BYTE, 1, fp)) return RET_ERROR;

	// address
	if (0 >= fread(&(index->address), FOUR_BYTE, 1, fp)) return RET_ERROR;

	// size
	if (0 >= fread(&(index->size), FOUR_BYTE, 1, fp)) return RET_ERROR;

	// padding
	if (0 >= fread(&(index->padding), FOUR_BYTE, 1, fp)) return RET_ERROR;

#ifdef DEBUG_ON
	printf("checksum = %08X\n", index->checksum);
	printf("address = %08X\n", index->address);
	printf("size = %08X\n", index->size);
	printf("padding = %08X\n", index->padding);
#endif

	return RET_OK;
}


/* seek address ******************
   �߂�l�F����,RET_OK �G���[,RET_ERROR
**********************************/
int seek_address(FILE *fp, const GARCHEADER *header, const GARCINDEX *index) {
	if (fp == NULL) return RET_ERROR;

#ifdef DEBUG_ON
	printf("index->address = %08X\n", index->address);
#endif
	if (fseek(fp, index->address + GARC_SIZE_HEADER + (header->num * GARC_SIZE_INDEX), SEEK_SET))
		return RET_ERROR;

	return RET_OK;
}


/* check_golgoth_id ****************************
   goltoth_archive�`���̃t�@�C���ł��邩�m�F����

   �߂�l�F����,RET_OK �G���[,RET_ERROR
************************************************/
int check_golgoth_id(const GARCHEADER *header) {
	if (0 != strncmp(header->id, GARC_ID, GARC_HEADER_ID_SIZE)) return RET_ERROR;

	return RET_OK;
}

