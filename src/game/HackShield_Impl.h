
#ifndef HACK_SHIELD_IMPL_H_
#define HACK_SHIELD_IMPL_H_

#include <unordered_map>

#ifdef OS_FREEBSD
// Live build only
#define UNIX
#include <AntiCpXSvr.h>
#undef UNIX
#endif

#pragma pack(1)

typedef struct SPacketGCHSCheck
{
	BYTE	bHeader;
#ifdef OS_FREEBSD
	AHNHS_TRANS_BUFFER Req;
#endif
} TPacketGCHSCheck;

#pragma pack()

class CHackShieldImpl
{
	public:
		bool Initialize ();
		void Release ();

		bool CreateClientHandle (DWORD dwPlayerID);
		void DeleteClientHandle (DWORD dwPlayerID);

		bool SendCheckPacket (LPCHARACTER ch);
		bool VerifyAck (LPCHARACTER ch, TPacketGCHSCheck* buf);

	private:
#ifdef OS_FREEBSD
		AHNHS_SERVER_HANDLE handle_;

		typedef std::unordered_map<DWORD, AHNHS_CLIENT_HANDLE> ClientHandleContainer;
		ClientHandleContainer CliehtHandleMap_;

		typedef std::unordered_map<DWORD, bool> ClientCheckContainer;
		ClientCheckContainer ClientCheckMap_;
#endif
};

#endif /* HACK_SHIELD_IMPL_H_ */

