#include "stdafx.h"
#include "config.h"
#include "firewall_manager.h"

#ifndef OS_WINDOWS
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#endif

extern bool g_bFirewallEnable;
extern int  g_iFirewallTcpSynLimit;
extern int  g_iFirewallTcpSynBurst;

FirewallManager::FirewallManager() = default;
FirewallManager::~FirewallManager() = default;

#ifdef OS_WINDOWS

// Windows: no-op stubs
bool FirewallManager::Initialize(WORD, WORD) { return false; }
void FirewallManager::Destroy() {}

#else

bool FirewallManager::RunCommand(const char* fmt, ...)
{
	char cmd[512];
	va_list args;
	va_start(args, fmt);
	vsnprintf(cmd, sizeof(cmd), fmt, args);
	va_end(args);

	// Temporarily set SIGCHLD to SIG_DFL before calling system().
	// The game server sets SIGCHLD to SIG_IGN (auto-reap children), which
	// causes waitpid() inside system() to return -1/ECHILD on FreeBSD
	// because the child gets reaped before status can be collected.
	struct sigaction saved, dflt;
	memset(&dflt, 0, sizeof(dflt));
	dflt.sa_handler = SIG_DFL;
	sigemptyset(&dflt.sa_mask);
	sigaction(SIGCHLD, &dflt, &saved);

	int status = system(cmd);

	sigaction(SIGCHLD, &saved, NULL);

	if (status == -1)
	{
		sys_err("FirewallManager: system() failed for: %s", cmd);
		return false;
	}

	return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

#ifdef OS_FREEBSD

// ---------------------------------------------------------------
// FreeBSD: ipfw
// Uses deterministic rule numbers based on port to avoid conflicts
// between multiple game processes on the same machine.
// Rule base = 50000 + (gamePort % 1000) * 10
// ---------------------------------------------------------------

void FirewallManager::CleanupRules()
{
	if (m_ruleBase == 0)
		return;

	for (int i = 0; i < 10; ++i)
		RunCommand("/sbin/ipfw -q delete %d 2>/dev/null", m_ruleBase + i);

	m_ruleCount = 0;
}

bool FirewallManager::Initialize(WORD gamePort, WORD p2pPort)
{
	if (!g_bFirewallEnable)
		return false;

	m_ruleBase = 50000 + (gamePort % 1000) * 10;

	// Clean up stale rules from previous crash
	CleanupRules();

	// Test if ipfw is available (use absolute path — popen() may have minimal PATH)
	if (!RunCommand("/sbin/ipfw -q list >/dev/null 2>&1"))
	{
		sys_err("FirewallManager: ipfw not available (module not loaded? try: kldload ipfw)");
		m_ruleBase = 0;
		return false;
	}

	int r = m_ruleBase;

	// Drop inbound UDP to game and P2P ports
	RunCommand("/sbin/ipfw -q add %d deny udp from any to me dst-port %d in", r++, gamePort);
	RunCommand("/sbin/ipfw -q add %d deny udp from any to me dst-port %d in", r++, p2pPort);

	// Drop ICMP port-unreachable (type 3)
	RunCommand("/sbin/ipfw -q add %d deny icmp from any to me icmptypes 3 in", r++);

	// TCP SYN rate limiting on game port (per-source concurrent connection limit)
	RunCommand("/sbin/ipfw -q add %d allow tcp from any to me dst-port %d setup limit src-addr %d",
		r++, gamePort, g_iFirewallTcpSynLimit);
	RunCommand("/sbin/ipfw -q add %d deny tcp from any to me dst-port %d setup",
		r++, gamePort);

	// TCP SYN rate limiting on P2P port
	RunCommand("/sbin/ipfw -q add %d allow tcp from any to me dst-port %d setup limit src-addr %d",
		r++, p2pPort, g_iFirewallTcpSynLimit);
	RunCommand("/sbin/ipfw -q add %d deny tcp from any to me dst-port %d setup",
		r++, p2pPort);

	m_ruleCount = r - m_ruleBase;
	m_initialized = true;
	sys_log(0, "FirewallManager: ipfw rules %d-%d installed (UDP DROP ports %d/%d, TCP SYN limit %d/src)",
		m_ruleBase, r - 1, gamePort, p2pPort, g_iFirewallTcpSynLimit);
	return true;
}

#else

// ---------------------------------------------------------------
// Linux: iptables
// Uses a dedicated chain per process (e.g., M2_GUARD_11011)
// ---------------------------------------------------------------

void FirewallManager::CleanupRules()
{
	if (m_chainName.empty())
		return;

	// Unhook from INPUT (ignore failure — may not exist)
	RunCommand("iptables -D INPUT -j %s 2>/dev/null", m_chainName.c_str());

	// Flush and delete the chain (ignore failure)
	RunCommand("iptables -F %s 2>/dev/null", m_chainName.c_str());
	RunCommand("iptables -X %s 2>/dev/null", m_chainName.c_str());
}

bool FirewallManager::Initialize(WORD gamePort, WORD p2pPort)
{
	if (!g_bFirewallEnable)
		return false;

	// Chain name includes port to avoid conflicts with multi-channel servers
	char buf[64];
	snprintf(buf, sizeof(buf), "M2_GUARD_%d", gamePort);
	m_chainName = buf;

	// Always clean up stale rules first (handles crash recovery)
	CleanupRules();

	// Create fresh chain
	if (!RunCommand("iptables -N %s", m_chainName.c_str()))
	{
		sys_err("FirewallManager: failed to create chain %s (not root? iptables not installed?)", m_chainName.c_str());
		m_chainName.clear();
		return false;
	}

	// Allow established/related UDP (e.g. DNS replies from outbound queries)
	RunCommand("iptables -A %s -p udp -m state --state ESTABLISHED,RELATED -j ACCEPT",
		m_chainName.c_str());

	// DROP all unsolicited inbound UDP
	RunCommand("iptables -A %s -p udp -j DROP", m_chainName.c_str());

	// Rate-limit ICMP to prevent reflection attacks
	RunCommand("iptables -A %s -p icmp --icmp-type port-unreachable -j DROP",
		m_chainName.c_str());
	RunCommand("iptables -A %s -p icmp -m limit --limit 10/s --limit-burst 20 -j ACCEPT",
		m_chainName.c_str());
	RunCommand("iptables -A %s -p icmp -j DROP", m_chainName.c_str());

	// TCP SYN flood protection on game port
	RunCommand("iptables -A %s -p tcp --dport %d --syn -m limit --limit %d/s --limit-burst %d -j ACCEPT",
		m_chainName.c_str(), gamePort, g_iFirewallTcpSynLimit, g_iFirewallTcpSynBurst);
	RunCommand("iptables -A %s -p tcp --dport %d --syn -j DROP",
		m_chainName.c_str(), gamePort);

	// TCP SYN flood protection on P2P port
	RunCommand("iptables -A %s -p tcp --dport %d --syn -m limit --limit %d/s --limit-burst %d -j ACCEPT",
		m_chainName.c_str(), p2pPort, g_iFirewallTcpSynLimit, g_iFirewallTcpSynBurst);
	RunCommand("iptables -A %s -p tcp --dport %d --syn -j DROP",
		m_chainName.c_str(), p2pPort);

	// Hook chain into INPUT
	if (!RunCommand("iptables -A INPUT -j %s", m_chainName.c_str()))
	{
		sys_err("FirewallManager: failed to hook chain into INPUT");
		CleanupRules();
		m_chainName.clear();
		return false;
	}

	m_initialized = true;
	sys_log(0, "FirewallManager: chain %s installed (UDP DROP, TCP SYN limit %d/s burst %d)",
		m_chainName.c_str(), g_iFirewallTcpSynLimit, g_iFirewallTcpSynBurst);
	return true;
}

#endif // OS_FREEBSD

void FirewallManager::Destroy()
{
	if (!m_initialized)
		return;

	CleanupRules();
	m_initialized = false;

#ifdef OS_FREEBSD
	sys_log(0, "FirewallManager: ipfw rules %d-%d removed", m_ruleBase, m_ruleBase + m_ruleCount - 1);
	m_ruleBase = 0;
	m_ruleCount = 0;
#else
	sys_log(0, "FirewallManager: chain %s removed", m_chainName.c_str());
	m_chainName.clear();
#endif
}

#endif // !OS_WINDOWS
