#include "../../libthecore/include/stdafx.h"
#include <stdio.h>
#include <string.h>
#include "targa.h"

CTargaImage::CTargaImage()
	: m_pbuf(NULL), m_x(0), m_y(0)
{
	memset( &m_header, 0, sizeof(m_header) );
}

CTargaImage::~CTargaImage()
{
    if (m_pbuf)
	delete [] m_pbuf;
}

char * CTargaImage::GetBasePointer(int line)
{
    return m_pbuf + (m_x * line) * sizeof(DWORD);
}

void CTargaImage::Create(int x, int y)
{
    memset(&m_header, 0, sizeof(m_header));

    m_header.imgType	= 2;
    m_header.width	= x;
    m_header.height	= y;
    m_header.colorBits	= 32;
    m_header.desc	= 0x20;

    m_pbuf = new char[x * y * sizeof(DWORD)];
    memset(m_pbuf, 0, x * y * sizeof(DWORD));

    m_x = x;
    m_y = y;
}

bool CTargaImage::Save(const char * filename)
{
    FILE * fp = fopen(filename, "wb");

    if (!fp)
	return false;

    fwrite(&m_header, sizeof(TGA_HEADER), 1, fp);
    fwrite(m_pbuf, m_x * m_y * sizeof(DWORD), 1, fp);

    fclose(fp);
    return true;
}

