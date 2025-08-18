#pragma once

int		log_init(void);
void	log_destroy(void);
void	log_rotate(void);

void	log_set_level(unsigned int level);
void	log_unset_level(unsigned int level);

void	log_set_expiration_days(unsigned int days);
int		log_get_expiration_days(void);

#ifndef OS_WINDOWS
void	_sys_err(const char *func, int line, const char *format, ...);
#else
void	_sys_err(const char *func, int line, const char *format, ...);
#endif

void	sys_log_header(const char *header);
void	sys_log(unsigned int lv, const char *format, ...);
void	pt_log(const char *format, ...);

#ifndef OS_WINDOWS
	#define sys_err(fmt, args...) _sys_err(__FUNCTION__, __LINE__, fmt, ##args)
#else 
	#define sys_err(fmt, ...) _sys_err(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif	// OS_WINDOWS
