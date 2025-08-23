
#include "common/stl.h"

class CMapLocation : public singleton<CMapLocation>
{
	public:
		typedef struct SLocation
		{
			uint32_t        addr;
			uint16_t        port;
		} TLocation;    

		bool    Get(long x, long y, int32_t& lMapIndex, uint32_t& lAddr, uint16_t& wPort);
		bool	Get(int32_t iIndex, uint32_t& lAddr, uint16_t& wPort);
		void    Insert(int32_t lIndex, const char * c_pszHost, uint16_t wPort);

	protected:
		std::map<long, TLocation> m_map_address;
};      

