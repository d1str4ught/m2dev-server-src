#ifndef __INC_METIN_II_ATTRIBUTE_H__
#define __INC_METIN_II_ATTRIBUTE_H__

enum EDataType
{
    D_DWORD,
    D_WORD,
    D_BYTE
};

//
// 맵 속성들을 처리할 때 사용
//
class CAttribute
{
    public:
	CAttribute(DWORD width, DWORD height); // dword 타잎으로 모두 0을 채운다.
	CAttribute(DWORD * attr, DWORD width, DWORD height); // attr을 읽어서 smart하게 속성을 읽어온다.
	~CAttribute();
	void Alloc();
	int GetDataType();
	void * GetDataPtr();
	void Set(DWORD x, DWORD y, DWORD attr);
	void Remove(DWORD x, DWORD y, DWORD attr);
	DWORD Get(DWORD x, DWORD y);
	void CopyRow(DWORD y, DWORD * row);

    private:
	void Initialize(DWORD width, DWORD height);

    private:
	int dataType;
	DWORD defaultAttr;
	DWORD width, height;

	void * data;

	BYTE **	bytePtr;
	WORD **	wordPtr;
	DWORD ** dwordPtr;
};

#endif
