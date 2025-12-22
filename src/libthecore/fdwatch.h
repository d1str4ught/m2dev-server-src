#pragma once

#ifndef __USE_SELECT__

    typedef struct fdwatch		FDWATCH;
    typedef struct fdwatch *	LPFDWATCH;

    enum EFdwatch
    {
		FDW_NONE			= 0,
		FDW_READ			= 1,
		FDW_WRITE			= 2,
		FDW_WRITE_ONESHOT	= 4,
		FDW_EOF				= 8,
    };

    typedef struct kevent	KEVENT;
    typedef struct kevent *	LPKEVENT;
    typedef int				KQUEUE;

    struct fdwatch
    {
		KQUEUE		kq;

		int		nfiles;

		LPKEVENT	kqevents;
		int		nkqevents;

		LPKEVENT	kqrevents;
		int *		fd_event_idx;

		void **		fd_data;
		int *		fd_rw;
    };

#else

    typedef struct fdwatch		FDWATCH;
    typedef struct fdwatch *	LPFDWATCH;

    enum EFdwatch
    {
		FDW_NONE			= 0,
		FDW_READ			= 1,
		FDW_WRITE			= 2,
		FDW_WRITE_ONESHOT	= 4,
		FDW_EOF				= 8,
    };

    struct fdwatch
    {
		fd_set rfd_set;
		fd_set wfd_set;

		socket_t* select_fds;
		int* select_rfdidx;

		int nselect_fds;

		fd_set working_rfd_set;
		fd_set working_wfd_set;

		int nfiles;

		void** fd_data;
		int* fd_rw;
    };

#endif // WIN32


LPFDWATCH	fdwatch_new(int nfiles);
void		fdwatch_clear_fd(LPFDWATCH fdw, socket_t fd);
void		fdwatch_delete(LPFDWATCH fdw);
int			fdwatch_check_fd(LPFDWATCH fdw, socket_t fd);
int			fdwatch_check_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx);
void		fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx);
void		fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void* client_data, int rw, int oneshot);
int			fdwatch(LPFDWATCH fdw, struct timeval *timeout);
void *		fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx);
void		fdwatch_del_fd(LPFDWATCH fdw, socket_t fd);
int			fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd);
int			fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx);

