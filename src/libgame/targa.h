#ifndef __INC_METIN_II_SERVER_TARGA_H__
#define __INC_METIN_II_SERVER_TARGA_H__

#pragma pack(1)

struct TGA_HEADER
{
    char idLen;		// 0
    char palType;	// 파레트있으면 1, 없음 0
    char imgType;	// 파레트있으면 1, 없음 2
    WORD colorBegin;	// 0
    WORD colorCount;	// 파레트 있으면 256, 없음 0
    char palEntrySize;	// 파레트 있으면 24, 없음 0
    WORD left;
    WORD top;
    WORD width;
    WORD height;
    char colorBits;
    char desc;
};

#define IMAGEDESC_ORIGIN_MASK	0x30
#define IMAGEDESC_TOPLEFT	0x20
#define IMAGEDESC_BOTLEFT	0x00
#define IMAGEDESC_BOTRIGHT	0x10
#define IMAGEDESC_TOPRIGHT	0x30

#pragma pack()

class CTargaImage
{
	public:
		CTargaImage();
		~CTargaImage();

		void	Create(int x, int y);
		char *	GetBasePointer(int line = 0);
		bool	Save(const char * filename);

	protected:
		TGA_HEADER	m_header;
		char *		m_pbuf;
		int		m_x, m_y;
};

#endif
