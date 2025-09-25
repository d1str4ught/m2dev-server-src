#include "stdafx.h"
#include "MarkImage.h"

#include "crc32.h"
#include "lzo_manager.h"
#define CLZO LZOManager

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

CGuildMarkImage * NewMarkImage()
{
	return new CGuildMarkImage;
}

void DeleteMarkImage(CGuildMarkImage * pkImage)
{
	delete pkImage;
}

CGuildMarkImage::CGuildMarkImage()
{
	memset( &m_apxImage, 0, sizeof(m_apxImage) );
}

CGuildMarkImage::~CGuildMarkImage()
{
	Destroy();
}

void CGuildMarkImage::Destroy()
{
	memset(m_apxImage, 0, sizeof(m_apxImage));
}

void CGuildMarkImage::Create()
{
	memset(m_apxImage, 0, sizeof(m_apxImage));
}

bool CGuildMarkImage::Save(const char* c_szFileName) 
{
	if (stbi_write_tga(c_szFileName, WIDTH, HEIGHT, 4, m_apxImage) == 0)
		return false;
	return true;
}

bool CGuildMarkImage::Build(const char * c_szFileName)
{
	sys_log(0, "GuildMarkImage: creating new file %s", c_szFileName);

	memset(m_apxImage, 0, sizeof(m_apxImage));
	if (!Save(c_szFileName)) {
		sys_err("GuildMarkImage: cannot initialize image");
		return false;
	}
	return true;
}

bool CGuildMarkImage::Load(const char * c_szFileName) 
{
	int w, h, channels;
	unsigned char* data = stbi_load(c_szFileName, &w, &h, &channels, 4);
	if (!data) {
		sys_err("GuildMarkImage: %s cannot open file.", c_szFileName);
		return false;
	}

	if (w != WIDTH || h != HEIGHT) {
		sys_err("GuildMarkImage: %s wrong dimensions (%d x %d)", c_szFileName, w, h);
		stbi_image_free(data);
		return false;
	}

	memcpy(m_apxImage, data, WIDTH * HEIGHT * 4);
	stbi_image_free(data);

	BuildAllBlocks();
	return true;
}

void CGuildMarkImage::PutData(UINT x, UINT y, UINT width, UINT height, void * data)
{
	for (UINT row = 0; row < height; ++row) {
		Pixel* dst = m_apxImage + (y + row) * WIDTH + x;
		Pixel* src = (Pixel*)data + row * width;
		memcpy(dst, src, width * sizeof(Pixel));
	}
}

void CGuildMarkImage::GetData(UINT x, UINT y, UINT width, UINT height, void * data)
{
	for (UINT row = 0; row < height; ++row) {
		Pixel* src = m_apxImage + (y + row) * WIDTH + x;
		Pixel* dst = (Pixel*)data + row * width;
		memcpy(dst, src, width * sizeof(Pixel));
	}
}

// 이미지 = 512x512
//   블럭 = 마크 4 x 4
//   마크 = 16 x 12
// 한 이미지의 블럭 = 8 x 10

// SERVER
bool CGuildMarkImage::SaveMark(DWORD posMark, BYTE * pbImage)
{
	if (posMark >= MARK_TOTAL_COUNT)
	{
		sys_err("GuildMarkImage::CopyMarkFromData: Invalid mark position %u", posMark);
		return false;
	}

	// 마크를 전체 이미지에 그린다.
	DWORD colMark = posMark % MARK_COL_COUNT;
	DWORD rowMark = posMark / MARK_COL_COUNT;

	printf("PutMark pos %u %ux%u\n", posMark, colMark * SGuildMark::WIDTH, rowMark * SGuildMark::HEIGHT);
	PutData(colMark * SGuildMark::WIDTH, rowMark * SGuildMark::HEIGHT, SGuildMark::WIDTH, SGuildMark::HEIGHT, pbImage);

	// 그려진 곳의 블럭을 업데이트
	DWORD rowBlock = rowMark / SGuildMarkBlock::MARK_PER_BLOCK_HEIGHT;
	DWORD colBlock = colMark / SGuildMarkBlock::MARK_PER_BLOCK_WIDTH;

	Pixel apxBuf[SGuildMarkBlock::SIZE];
	GetData(colBlock * SGuildMarkBlock::WIDTH, rowBlock * SGuildMarkBlock::HEIGHT, SGuildMarkBlock::WIDTH, SGuildMarkBlock::HEIGHT, apxBuf);
	m_aakBlock[rowBlock][colBlock].Compress(apxBuf);
	return true;
}

bool CGuildMarkImage::DeleteMark(DWORD posMark)
{
	Pixel image[SGuildMark::SIZE];
	memset(&image, 0, sizeof(image));
	return SaveMark(posMark, (BYTE *) &image);
}

// CLIENT
bool CGuildMarkImage::SaveBlockFromCompressedData(DWORD posBlock, const BYTE * pbComp, DWORD dwCompSize)
{
	if (posBlock >= BLOCK_TOTAL_COUNT)
		return false;

	Pixel apxBuf[SGuildMarkBlock::SIZE];
	lzo_uint sizeBuf = sizeof(apxBuf);

	if (LZO_E_OK != lzo1x_decompress_safe(pbComp, dwCompSize, (BYTE *) apxBuf, &sizeBuf, CLZO::Instance().GetWorkMemory()))
	{
		sys_err("GuildMarkImage::CopyBlockFromCompressedData: cannot decompress, compressed size = %u", dwCompSize);
		return false;
	}

	if (sizeBuf != sizeof(apxBuf))
	{
		sys_err("GuildMarkImage::CopyBlockFromCompressedData: image corrupted, decompressed size = %u", sizeBuf);
		return false;
	}

	DWORD rowBlock = posBlock / BLOCK_COL_COUNT;
	DWORD colBlock = posBlock % BLOCK_COL_COUNT;

	PutData(colBlock * SGuildMarkBlock::WIDTH, rowBlock * SGuildMarkBlock::HEIGHT, SGuildMarkBlock::WIDTH, SGuildMarkBlock::HEIGHT, apxBuf);

	m_aakBlock[rowBlock][colBlock].CopyFrom(pbComp, dwCompSize, GetCRC32((const char *) apxBuf, sizeof(Pixel) * SGuildMarkBlock::SIZE));
	return true;
}

void CGuildMarkImage::BuildAllBlocks() // 이미지 전체를 블럭화
{
	Pixel apxBuf[SGuildMarkBlock::SIZE];
	sys_log(0, "GuildMarkImage::BuildAllBlocks");

	for (UINT row = 0; row < BLOCK_ROW_COUNT; ++row)
		for (UINT col = 0; col < BLOCK_COL_COUNT; ++col)
		{
			GetData(col * SGuildMarkBlock::WIDTH, row * SGuildMarkBlock::HEIGHT, SGuildMarkBlock::WIDTH, SGuildMarkBlock::HEIGHT, apxBuf);
			m_aakBlock[row][col].Compress(apxBuf);
		}
}

DWORD CGuildMarkImage::GetEmptyPosition()
{
	SGuildMark kMark;

	for (DWORD row = 0; row < MARK_ROW_COUNT; ++row)
	{
		for (DWORD col = 0; col < MARK_COL_COUNT; ++col)
		{
			GetData(col * SGuildMark::WIDTH, row * SGuildMark::HEIGHT, SGuildMark::WIDTH, SGuildMark::HEIGHT, kMark.m_apxBuf);

			if (kMark.IsEmpty())
				return (row * MARK_COL_COUNT + col);
		}
	}

	return INVALID_MARK_POSITION;
}

void CGuildMarkImage::GetDiffBlocks(const uint32_t* crcList, std::map<BYTE, const SGuildMarkBlock *> & mapDiffBlocks)
{
	BYTE posBlock = 0;

	for (DWORD row = 0; row < BLOCK_ROW_COUNT; ++row)
		for (DWORD col = 0; col < BLOCK_COL_COUNT; ++col)
		{
			if (m_aakBlock[row][col].m_crc != *crcList)
			{
				mapDiffBlocks.insert(std::map<BYTE, const SGuildMarkBlock *>::value_type(posBlock, &m_aakBlock[row][col]));
			}
			++crcList;
			++posBlock;
		}
}

void CGuildMarkImage::GetBlockCRCList(DWORD * crcList)
{
	for (DWORD row = 0; row < BLOCK_ROW_COUNT; ++row)
		for (DWORD col = 0; col < BLOCK_COL_COUNT; ++col)
			*(crcList++) = m_aakBlock[row][col].GetCRC();
}

////////////////////////////////////////////////////////////////////////////////
void SGuildMark::Clear()
{
	for (DWORD iPixel = 0; iPixel < SIZE; ++iPixel)
		m_apxBuf[iPixel] = 0xff000000;	
}

bool SGuildMark::IsEmpty()
{
	for (DWORD iPixel = 0; iPixel < SIZE; ++iPixel)
		if (m_apxBuf[iPixel] != 0x00000000)
			return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////
DWORD SGuildMarkBlock::GetCRC() const
{
	return m_crc;
}

void SGuildMarkBlock::CopyFrom(const BYTE * pbCompBuf, DWORD dwCompSize, DWORD crc)
{
	if (dwCompSize > MAX_COMP_SIZE)
		return;

	m_sizeCompBuf = dwCompSize;
	thecore_memcpy(m_abCompBuf, pbCompBuf, dwCompSize);
	m_crc = crc;
	//printf("SGuildMarkBlock::CopyFrom: %u > %u crc %u\n", sizeof(Pixel) * SGuildMarkBlock::SIZE, m_sizeCompBuf, m_crc);
}

void SGuildMarkBlock::Compress(const Pixel * pxBuf)
{
	m_sizeCompBuf = MAX_COMP_SIZE;

	if (LZO_E_OK != lzo1x_1_compress((const BYTE *) pxBuf, sizeof(Pixel) * SGuildMarkBlock::SIZE, m_abCompBuf, &m_sizeCompBuf, CLZO::Instance().GetWorkMemory()))
	{
		sys_err("SGuildMarkBlock::Compress: Error! %u > %u", sizeof(Pixel) * SGuildMarkBlock::SIZE, m_sizeCompBuf);
		return;
	}

	//sys_log(0, "SGuildMarkBlock::Compress %u > %u", sizeof(Pixel) * SGuildMarkBlock::SIZE, m_sizeCompBuf);
	m_crc = GetCRC32((const char *) pxBuf, sizeof(Pixel) * SGuildMarkBlock::SIZE);
}
