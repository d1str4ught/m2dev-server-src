#pragma once

#if defined(OS_FREEBSD)

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

#elif defined(OS_WINDOWS)

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

#else

#define EV_ADD      0x0001  /* aggiungi evento */
#define EV_DELETE   0x0002  /* rimuovi evento */
#define EV_ONESHOT  0x0010  /* evento valido una sola volta */

    typedef struct fdwatch		FDWATCH;
    typedef struct fdwatch *	LPFDWATCH;
	typedef int socket_t;

	enum EFdwatch
    {
		FDW_NONE			= 0,
		FDW_READ			= 1,
		FDW_WRITE			= 2,
		FDW_WRITE_ONESHOT	= 4,
		FDW_EOF				= 8,
		FDW_READ_ONESHOT	= 16,
    };

	enum EPendingOp
	{
		FDW_OP_NONE	= 0,
		FDW_OP_ADD	= 1,
		FDW_OP_MOD	= 2,
		FDW_OP_DEL	= 3,
	};
	
	struct fdwatch
	{
		int epfd;                   // epoll file descriptor
		struct epoll_event *events; // array di eventi restituiti da epoll_wait
		int nfiles;                 // numero massimo di file descriptor gestiti

		// Stato per ogni fd
		bool *fd_registered; // 1 se fd è registrato in epoll, 0 altrimenti

		// Maschere di I/O
		int *fd_rw_current; // maschera attualmente installata in epoll
		int *fd_rw_pending; // maschera che vogliamo installare al prossimo fdwatch

		// Operazioni pendenti verso epoll
		EPendingOp *fd_pending_op; // op pendente per ogni fd

		void **fd_data; // dati come il DESC del client

		int nevents;

		// --- NUOVI CAMPI PER LA DIRTY LIST ---
		int *dirty_fds; // elenco di fd con operazioni pendenti
		int dirty_count;
		bool *fd_dirty; // flag per evitare duplicati in dirty_fds
	};

#endif


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
void 		fdwatch_insert_fd(LPFDWATCH fdw, socket_t fd);
