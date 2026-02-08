#ifndef __INC_FIREWALL_MANAGER_H__
#define __INC_FIREWALL_MANAGER_H__

// Kernel-level firewall management.
// Linux: iptables chains. FreeBSD: ipfw rules.
// Installs DROP rules for unsolicited UDP and rate-limits TCP SYN floods.
// Windows: no-op stubs.

class FirewallManager : public singleton<FirewallManager>
{
public:
	FirewallManager();
	~FirewallManager();

	// Install firewall rules — call after config_init()
	bool Initialize(WORD gamePort, WORD p2pPort);

	// Remove firewall rules — call during shutdown
	void Destroy();

private:
#ifndef OS_WINDOWS
	bool RunCommand(const char* fmt, ...);
	void CleanupRules();
#endif

	std::string m_chainName;	// Linux: iptables chain name
	int m_ruleBase = 0;			// FreeBSD: ipfw rule number base
	int m_ruleCount = 0;		// FreeBSD: number of rules installed
	bool m_initialized = false;
};

#endif
