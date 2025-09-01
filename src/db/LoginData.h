// vim:ts=8 sw=4
#ifndef __INC_METIN_II_DB_LOGINDATA_H__
#define __INC_METIN_II_DB_LOGINDATA_H__

class CLoginData
{
    public:
	CLoginData();

	TAccountTable & GetAccountRef();
	void            SetClientKey(const uint32_t * c_pdwClientKey);

	const uint32_t*   GetClientKey();
	void            SetKey(DWORD dwKey);
	DWORD           GetKey();

	void            SetConnectedPeerHandle(DWORD dwHandle);
	DWORD		GetConnectedPeerHandle();

	void            SetLogonTime();
	DWORD		GetLogonTime();

	void		SetIP(const char * c_pszIP);
	const char *	GetIP();

	void		SetPlay(bool bOn);
	bool		IsPlay();

	void		SetDeleted(bool bSet);
	bool		IsDeleted();

	time_t		GetLastPlayTime() { return m_lastPlayTime; }

	void            SetPremium(int * paiPremiumTimes);
	int             GetPremium(BYTE type);
	int *           GetPremiumPtr();

	DWORD		GetLastPlayerID() const { return m_dwLastPlayerID; }
	void		SetLastPlayerID(DWORD id) { m_dwLastPlayerID = id; }

    private:
	DWORD           m_dwKey;
	uint32_t           m_adwClientKey[4];
	DWORD           m_dwConnectedPeerHandle;
	DWORD           m_dwLogonTime;
	char		m_szIP[MAX_HOST_LENGTH+1];
	bool		m_bPlay;
	bool		m_bDeleted;

	time_t		m_lastPlayTime;
	int		m_aiPremiumTimes[PREMIUM_MAX_NUM];

	DWORD		m_dwLastPlayerID;

	TAccountTable   m_data;
};

#endif
