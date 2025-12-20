#ifndef __INC_MESSENGER_MANAGER_H
#define __INC_MESSENGER_MANAGER_H

#include "db.h"

#include <unordered_map>
#include <vector>

class MessengerManager : public singleton<MessengerManager>
{
	public:
		typedef std::string keyT;
		typedef const std::string & keyA;

		MessengerManager();
		virtual ~MessengerManager();

	public:
		void	P2PLogin(keyA account);
		void	P2PLogout(keyA account);

		void	Login(keyA account);
		void	Logout(keyA account);

		void	RequestToAdd(LPCHARACTER ch, LPCHARACTER target);

		void	RegisterRequestToAdd(const char* szAccount, const char* szTarget);
		void	P2PRequestToAdd_Stage1(LPCHARACTER ch, const char* targetName);
		void	P2PRequestToAdd_Stage2(const char* characterName, LPCHARACTER target);

		// void	AuthToAdd(keyA account, keyA companion, bool bDeny);
		bool	AuthToAdd(keyA account, keyA companion, bool bDeny);

		void	__AddToList(keyA account, keyA companion, bool isRequester = true);	// 실제 m_Relation, m_InverseRelation 수정하는 메소드
		void	AddToList(keyA account, keyA companion);

		void	__RemoveFromList(keyA account, keyA companion, bool isRequester = true); // 실제 m_Relation, m_InverseRelation 수정하는 메소드
		void	RemoveFromList(keyA account, keyA companion);

		void	RemoveAllList(keyA account);

		void	Initialize();
		
		bool	IsInList(MessengerManager::keyA account, MessengerManager::keyA companion);

	private:
		void	SendList(keyA account);
		void	SendLogin(keyA account, keyA companion);
		void	SendLogout(keyA account, keyA companion);

		void	LoadList(SQLMsg * pmsg);

		void	Destroy();

		// Helpers to manage friend-request index so requests involving a disconnecting character can be removed
		void	RegisterRequestComplex(DWORD dw1, DWORD dw2, DWORD dwComplex);
		void	RemoveComplex(DWORD dwComplex);
		void	EraseRequestsForAccount(keyA account);
		void 	EraseIncomingRequestsForTarget(const char* targetName);

		std::set<keyT>			m_set_loginAccount;
		std::map<keyT, std::set<keyT> >	m_Relation;
		std::map<keyT, std::set<keyT> >	m_InverseRelation;
		std::set<DWORD>			m_set_requestToAdd;

		// Map complex -> (dw1, dw2)
		std::unordered_map<DWORD, std::pair<DWORD, DWORD>>	m_map_requestComplex;
		// requester CRC -> set of complex values
		std::unordered_map<DWORD, std::set<DWORD>>			m_map_requestsFrom;
		// target CRC -> set of complex values
		std::unordered_map<DWORD, std::set<DWORD>>			m_map_requestsTo;
};

#endif
