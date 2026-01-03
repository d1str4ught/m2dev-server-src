#include "stdafx.h"

#ifdef OS_FREEBSD

LPFDWATCH fdwatch_new(int nfiles)
{
    LPFDWATCH fdw;
    int kq;

    kq = kqueue();

    if (kq == -1)
    {
	sys_err("%s", strerror(errno));
	return NULL;
    }

    CREATE(fdw, FDWATCH, 1);

    fdw->kq = kq;
    fdw->nfiles = nfiles;
    fdw->nkqevents = 0;

    CREATE(fdw->kqevents, KEVENT, nfiles * 2);
    CREATE(fdw->kqrevents, KEVENT, nfiles * 2);
    CREATE(fdw->fd_event_idx, int, nfiles);
    CREATE(fdw->fd_rw, int, nfiles);
    CREATE(fdw->fd_data, void*, nfiles);

    return (fdw);
}

void fdwatch_delete(LPFDWATCH fdw)
{
    free(fdw->fd_data);
    free(fdw->fd_rw);
    free(fdw->kqevents);
    free(fdw->kqrevents);
    free(fdw->fd_event_idx);
    free(fdw);
}

int fdwatch(LPFDWATCH fdw, struct timeval *timeout)
{
    int	i, r;
    struct timespec ts;

    if (fdw->nkqevents)
	sys_log(2, "fdwatch: nkqevents %d", fdw->nkqevents);

    if (!timeout)
    {
	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	r = kevent(fdw->kq, fdw->kqevents, fdw->nkqevents, fdw->kqrevents, fdw->nfiles, &ts);
    }
    else
    {
	ts.tv_sec = timeout->tv_sec;
	ts.tv_nsec = timeout->tv_usec;

	r = kevent(fdw->kq, fdw->kqevents, fdw->nkqevents, fdw->kqrevents, fdw->nfiles, &ts);
    }

    fdw->nkqevents = 0;

    if (r == -1)
	return -1;

    memset(fdw->fd_event_idx, 0, sizeof(int) * fdw->nfiles);

    for (i = 0; i < r; i++)
    {
	int fd = fdw->kqrevents[i].ident;

	if (fd >= fdw->nfiles)
	    sys_err("ident overflow %d nfiles: %d", fdw->kqrevents[i].ident, fdw->nfiles);
	else
	{
	    if (fdw->kqrevents[i].filter == EVFILT_WRITE)
		fdw->fd_event_idx[fd] = i;
	}
    }

    return (r);
}

void fdwatch_register(LPFDWATCH fdw, int flag, int fd, int rw)
{
    if (flag == EV_DELETE)
    {
	if (fdw->fd_rw[fd] & FDW_READ)
	{
	    fdw->kqevents[fdw->nkqevents].ident = fd;
	    fdw->kqevents[fdw->nkqevents].flags = flag;
	    fdw->kqevents[fdw->nkqevents].filter = EVFILT_READ;
	    ++fdw->nkqevents;
	}

	if (fdw->fd_rw[fd] & FDW_WRITE)
	{
	    fdw->kqevents[fdw->nkqevents].ident = fd;
	    fdw->kqevents[fdw->nkqevents].flags = flag;
	    fdw->kqevents[fdw->nkqevents].filter = EVFILT_WRITE;
	    ++fdw->nkqevents;
	}
    }
    else
    {
	fdw->kqevents[fdw->nkqevents].ident = fd;
	fdw->kqevents[fdw->nkqevents].flags = flag;
	fdw->kqevents[fdw->nkqevents].filter = (rw == FDW_READ) ? EVFILT_READ : EVFILT_WRITE; 

	++fdw->nkqevents;
    }
}

void fdwatch_clear_fd(LPFDWATCH fdw, socket_t fd)
{
    fdw->fd_data[fd] = NULL;
    fdw->fd_rw[fd] = 0;
}

void fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void * client_data, int rw, int oneshot)
{
	int flag;

	if (fd >= fdw->nfiles)
	{
		sys_err("fd overflow %d", fd);
		return;
	}

	if (fdw->fd_rw[fd] & rw)
		return;

	fdw->fd_rw[fd] |= rw;
	sys_log(2, "FDWATCH_fdw %p fd %d rw %d data %p", fdw, fd, rw, client_data);

	if (!oneshot)
		flag = EV_ADD;
	else
	{
		sys_log(2, "ADD ONESHOT fd_rw %d", fdw->fd_rw[fd]);
		flag = EV_ADD | EV_ONESHOT;
		fdw->fd_rw[fd] |= FDW_WRITE_ONESHOT;
	}

	fdw->fd_data[fd] = client_data;
	fdwatch_register(fdw, flag, fd, rw);
}

void fdwatch_del_fd(LPFDWATCH fdw, socket_t fd)
{
    fdwatch_register(fdw, EV_DELETE, fd, 0);
    fdwatch_clear_fd(fdw, fd);
}

void fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
    assert(event_idx < fdw->nfiles * 2);

    if (fdw->kqrevents[event_idx].ident != fd)
	return;

    fdw->kqrevents[event_idx].ident = 0;
}

int fdwatch_check_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
    assert(event_idx < fdw->nfiles * 2);

    if (fdw->kqrevents[event_idx].ident != fd)
	return 0;

    if (fdw->kqrevents[event_idx].flags & EV_ERROR)
	return FDW_EOF;

    if (fdw->kqrevents[event_idx].flags & EV_EOF)
	return FDW_EOF;

    if (fdw->kqrevents[event_idx].filter == EVFILT_READ)
    {
	if (fdw->fd_rw[fd] & FDW_READ)
	    return FDW_READ;
    }
    else if (fdw->kqrevents[event_idx].filter == EVFILT_WRITE)
    {   
	if (fdw->fd_rw[fd] & FDW_WRITE)
	{ 
	    if (fdw->fd_rw[fd] & FDW_WRITE_ONESHOT)
		fdw->fd_rw[fd] &= ~FDW_WRITE;

	    return FDW_WRITE;
	}
    }
    else
	sys_err("fdwatch_check_event: Unknown filter %d (descriptor %d)", fdw->kqrevents[event_idx].filter, fd);

    return 0;
}

int fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx)
{
    assert(event_idx < fdw->nfiles * 2);
    return fdw->kqrevents[event_idx].ident;
}

int fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd)
{
    int event_idx = fdw->fd_event_idx[fd];

    if (fdw->kqrevents[event_idx].filter == EVFILT_WRITE)
	return fdw->kqrevents[event_idx].data;

    return 0;
}

void * fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx)
{
    int fd;

    assert(event_idx < fdw->nfiles * 2);

    fd = fdw->kqrevents[event_idx].ident;

    if (fd >= fdw->nfiles)
	return NULL;

    return (fdw->fd_data[fd]);
}

void fdwatch_insert_fd(LPFDWATCH fdw, socket_t fd)
{
}
#endif	// ifdef OS_FREEBSD

#ifdef OS_WINDOWS
static int win32_init_refcount = 0;

static bool win32_init()
{
    if (win32_init_refcount > 0)
    {
	win32_init_refcount++;
	return true;
    }

    WORD wVersion = MAKEWORD(2, 0);
    WSADATA wsaData;

    if (WSAStartup(wVersion, &wsaData) != 0)
	return false;

    win32_init_refcount++;
    return true;
}

static void win32_deinit()
{
    if (--win32_init_refcount <= 0)
	WSACleanup();
}

LPFDWATCH fdwatch_new(int nfiles)
{
    LPFDWATCH fdw;

#ifdef OS_WINDOWS
    if (!win32_init())
	return NULL;
#endif
	// nfiles value is limited to FD_SETSIZE (64)
    CREATE(fdw, FDWATCH, 1);
	fdw->nfiles = MIN(nfiles, FD_SETSIZE);

    FD_ZERO(&fdw->rfd_set);
    FD_ZERO(&fdw->wfd_set);

    CREATE(fdw->select_fds, socket_t, nfiles);
    CREATE(fdw->select_rfdidx, int, nfiles);

	fdw->nselect_fds = 0;

    CREATE(fdw->fd_rw, int, nfiles);
    CREATE(fdw->fd_data, void*, nfiles);

    return (fdw);
}

void fdwatch_delete(LPFDWATCH fdw)
{
    free(fdw->fd_data);
    free(fdw->fd_rw);
    free(fdw->select_fds);
    free(fdw->select_rfdidx);
    free(fdw);

#ifdef OS_WINDOWS
    win32_deinit();
#endif
}

static int fdwatch_get_fdidx(LPFDWATCH fdw, socket_t fd) {
	int i;
	for (i = 0; i < fdw->nselect_fds; ++i) {
		if (fdw->select_fds[i] == fd) {
			return i;
		}
	}
	return -1;
}

void fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void* client_data, int rw, int oneshot)
{
	int idx = fdwatch_get_fdidx(fdw, fd);
	if (idx < 0) {
		if (fdw->nselect_fds >= fdw->nfiles) {
			return;
		}
		idx = fdw->nselect_fds;
		fdw->select_fds[fdw->nselect_fds++] = fd;
		fdw->fd_rw[idx] = rw;
	} else {
		fdw->fd_rw[idx] |= rw;
	}
	fdw->fd_data[idx] = client_data;

    if (rw & FDW_READ)
	FD_SET(fd, &fdw->rfd_set);

    if (rw & FDW_WRITE)
	FD_SET(fd, &fdw->wfd_set);
}

void fdwatch_del_fd(LPFDWATCH fdw, socket_t fd)
{
	if (fdw->nselect_fds <= 0) {
		return;
	}
    int idx = fdwatch_get_fdidx(fdw, fd);
	if (idx < 0) {
		return;
	}

	--fdw->nselect_fds;

	fdw->select_fds[idx] = fdw->select_fds[fdw->nselect_fds];
    fdw->fd_data[idx] = fdw->fd_data[fdw->nselect_fds];
    fdw->fd_rw[idx] = fdw->fd_rw[fdw->nselect_fds];

    FD_CLR(fd, &fdw->rfd_set);
    FD_CLR(fd, &fdw->wfd_set);
}

int fdwatch(LPFDWATCH fdw, struct timeval *timeout)
{
    int r, i, event_idx;
    struct timeval tv;

    fdw->working_rfd_set = fdw->rfd_set;
    fdw->working_wfd_set = fdw->wfd_set;

    if (!timeout)
    {
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	r = select(0, &fdw->working_rfd_set, &fdw->working_wfd_set, (fd_set*) 0, &tv);
    }
    else
    {
	tv = *timeout;
	r = select(0, &fdw->working_rfd_set, &fdw->working_wfd_set, (fd_set*) 0, &tv);
    }

    if (r == -1)
	return -1;

    event_idx = 0;

    for (i = 0; i < fdw->nselect_fds; ++i)
    {
		if (fdwatch_check_fd(fdw, fdw->select_fds[i]))
			fdw->select_rfdidx[event_idx++] = i;
    }

    return event_idx;
}

int fdwatch_check_fd(LPFDWATCH fdw, socket_t fd)
{
    int idx = fdwatch_get_fdidx(fdw, fd);
	if (idx < 0) {
		return 0;
	}
	int result = 0;
	if ((fdw->fd_rw[idx] & FDW_READ) && FD_ISSET(fd, &fdw->working_rfd_set)) {
		result |= FDW_READ;
	}
	if ((fdw->fd_rw[idx] & FDW_WRITE) && FD_ISSET(fd, &fdw->working_wfd_set)) {
		result |= FDW_WRITE;
	}
    return result;
}

void * fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return NULL;
	}
    return fdw->fd_data[idx];
}

int fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return 0;
	}
	return (int)fdw->select_fds[idx];
}

void fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return;
	}
	socket_t rfd = fdw->select_fds[idx];
	if (fd != rfd) {
		return;
	}
    FD_CLR(fd, &fdw->working_rfd_set);
    FD_CLR(fd, &fdw->working_wfd_set);
}

int fdwatch_check_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx)
{
	int idx = fdw->select_rfdidx[event_idx];
	if (idx < 0 || fdw->nfiles <= idx) {
		return 0;
	}
	socket_t rfd = fdw->select_fds[idx];
	if (fd != rfd) {
		return 0;
	}
	int result = fdwatch_check_fd(fdw, fd);
	if (result & FDW_READ) {
		return FDW_READ;
	} else if (result & FDW_WRITE) {
		return FDW_WRITE;
	}
	return 0;
}

int fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd)
{
    return INT_MAX; // XXX TODO
}

void fdwatch_insert_fd(LPFDWATCH fdw, socket_t fd)
{
}

#endif

#ifdef __linux__

#include <sys/epoll.h>
/* ---------------------------------------------------------------------- */
/* INIT / DESTROY                                                         */
/* ---------------------------------------------------------------------- */

LPFDWATCH fdwatch_new(int nfiles) {
  int epfd = epoll_create1(0);
  if (epfd == -1) {
    return nullptr;
  }

  LPFDWATCH fdw = new FDWATCH;
  fdw->epfd = epfd;
  fdw->nfiles = nfiles;

  fdw->events = new epoll_event[nfiles]();
  fdw->fd_registered = new bool[nfiles]();
  fdw->fd_rw_current = new int[nfiles]();
  fdw->fd_rw_pending = new int[nfiles]();
  fdw->fd_pending_op = new EPendingOp[nfiles]();
  fdw->fd_data = new void *[nfiles]();

  // nuovi
  fdw->dirty_fds = new int[nfiles]();
  fdw->fd_dirty = new bool[nfiles]();
  fdw->dirty_count = 0;

  fdw->nevents = 0;

  return fdw;
}

void fdwatch_delete(LPFDWATCH fdw) {
  if (!fdw)
    return;

  delete[] fdw->fd_registered;
  delete[] fdw->events;
  delete[] fdw->fd_data;
  delete[] fdw->fd_rw_current;
  delete[] fdw->fd_rw_pending;
  delete[] fdw->fd_pending_op;

  delete[] fdw->dirty_fds;
  delete[] fdw->fd_dirty;

  close(fdw->epfd);
  delete fdw;
}

static inline void fdwatch_mark_dirty(LPFDWATCH fdw, int fd) {
  if ((fd < 0) || (fd >= fdw->nfiles))
    return;

  if (!fdw->fd_dirty[fd]) {
    fdw->fd_dirty[fd] = true;
    fdw->dirty_fds[fdw->dirty_count++] = fd;
  }
}

/* ---------------------------------------------------------------------- */
/* SYNC INTERNO: costruzione della struct epoll_event da fd_rw_pending    */
/* (non fa syscall, solo helper)                                          */
/* ---------------------------------------------------------------------- */

static void fdwatch_build_event(LPFDWATCH fdw, socket_t fd,
                                struct epoll_event *ev) {

  // memset(ev, 0, sizeof(*ev));
  const int mask = fdw->fd_rw_pending[fd];
  uint32_t events = EPOLLRDHUP;

  if (mask & (FDW_READ | FDW_READ_ONESHOT))
    events |= EPOLLIN;
  if (mask & (FDW_WRITE | FDW_WRITE_ONESHOT))
    events |= EPOLLOUT;
  if (mask & (FDW_READ_ONESHOT | FDW_WRITE_ONESHOT))
    events |= EPOLLONESHOT;

  ev->events = events;
  ev->data.fd = fd;
}

/* ---------------------------------------------------------------------- */
/* FASE DI APPLICAZIONE DELLE MODIFICHE PENDENTI PRIMA DI epoll_wait      */
/* ---------------------------------------------------------------------- */

static void fdwatch_apply_pending(LPFDWATCH fdw) {

  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));

  // iteriamo solo sui fd "sporchi"
  for (int i = 0; i < fdw->dirty_count; ++i) {
    int fd = fdw->dirty_fds[i];

    // resettiamo subito il flag di "dirty":
    fdw->fd_dirty[fd] = false;

    EPendingOp op = fdw->fd_pending_op[fd];

    if (op == FDW_OP_NONE)
      continue; // nessuna op pendente (può succedere se è stata azzerata dopo
                // il mark)

    switch (op) {
      
      case FDW_OP_ADD:
        fdwatch_build_event(fdw, fd, &ev);
        epoll_ctl(fdw->epfd, EPOLL_CTL_ADD, fd, &ev);
        fdw->fd_registered[fd] = true;
        break;
      case FDW_OP_MOD:
        fdwatch_build_event(fdw, fd, &ev);
        epoll_ctl(fdw->epfd, EPOLL_CTL_MOD, fd, &ev);
        break;
      case FDW_OP_DEL:
        epoll_ctl(fdw->epfd, EPOLL_CTL_DEL, fd, nullptr);
        fdw->fd_registered[fd] = false;
        break;
    };

    fdw->fd_rw_current[fd] = fdw->fd_rw_pending[fd];
    fdw->fd_pending_op[fd] = FDW_OP_NONE;


  }

  // dopo aver processato tutti, azzeriamo il contatore
  fdw->dirty_count = 0;
}

/* ---------------------------------------------------------------------- */
/* fdwatch: unica funzione che fa epoll_ctl (batch) + epoll_wait          */
/* ---------------------------------------------------------------------- */

int fdwatch(LPFDWATCH fdw, struct timeval *timeout) {

  int timeout_ms;

  if (!timeout) {
    timeout_ms = 0;
  } else {
    timeout_ms = timeout->tv_sec * 1000 + (timeout->tv_usec + 999) / 1000;
  }

  // Applica tutte le modifiche pendenti in una volta
  fdwatch_apply_pending(fdw);

  int r;
  do {
    r = epoll_wait(fdw->epfd, fdw->events, fdw->nfiles, timeout_ms);
  } while ((r == -1) && (errno == EINTR));


  //per ogni evento, se c'è stato un oneshot, prova a riabilitare gli altri non oneshot
  for (int event_idx = 0; event_idx < r; ++event_idx)
  {
    struct epoll_event &ev = fdw->events[event_idx];
    const uint32_t events = ev.events;
    const int fd = ev.data.fd;

    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    {
      fdw->fd_rw_pending[fd] = 0;
      fdw->fd_pending_op[fd] = FDW_OP_MOD;
      fdwatch_mark_dirty(fdw, fd);
      continue;
    }

    

    const int old_mask = fdw->fd_rw_pending[fd];
    int new_mask = old_mask;

    // EPOLLIN
    if (events & EPOLLIN) {
      new_mask &= ~FDW_READ_ONESHOT;
    }

    // EPOLLOUT
    if (events & EPOLLOUT) {
      new_mask &= ~FDW_WRITE_ONESHOT;
    }

    

    if (new_mask != old_mask)
    {
      fdw->fd_rw_pending[fd] = new_mask;
      fdw->fd_pending_op[fd] = FDW_OP_MOD;
      fdwatch_mark_dirty(fdw, fd);
    }


  }


  fdw->nevents = r;
  return r;
}

/* ---------------------------------------------------------------------- */
/* fdwatch_check_event                                                    */
/* - Usa fd_rw_current per sapere cosa era effettivamente registrato      */
/* - Aggiorna fd_rw_pending se togliamo i flag ONESHOT                    */
/* - NON chiama epoll_ctl: le modifiche verranno applicate nel prossimo   */
/*   fdwatch() tramite fdwatch_apply_pending()                             */
/* ---------------------------------------------------------------------- */

int fdwatch_check_event(LPFDWATCH fdw, int fd, unsigned int event_idx) {

  struct epoll_event &ev = fdw->events[event_idx];

  if (ev.data.fd != fd)
    return 0;

  const uint32_t events = ev.events;

  if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
    return FDW_EOF;

  //const int cur_mask = fdw->fd_rw_current[fd];
  //const int old_mask = fdw->fd_rw_pending[fd];
  //int new_mask = old_mask;

  int result = 0;

  // EPOLLIN
  if ((events & EPOLLIN) /*&& (cur_mask & (FDW_READ | FDW_READ_ONESHOT))*/) {

    //new_mask &= ~FDW_READ_ONESHOT;
    result |= FDW_READ;
  }

  // EPOLLOUT
  if ((events & EPOLLOUT) /*&& (cur_mask & (FDW_WRITE | FDW_WRITE_ONESHOT))*/) {

    //new_mask &= ~FDW_WRITE_ONESHOT;
    result |= FDW_WRITE;
  }

  //if (new_mask == old_mask)
  //  return result;

  //fdw->fd_rw_pending[fd] = new_mask;

  // Nessun interesse rimanente (né READ né WRITE, con o senza ONESHOT)
  //if ((new_mask &
  //     (FDW_READ | FDW_READ_ONESHOT | FDW_WRITE | FDW_WRITE_ONESHOT)) == 0) {
  //  return result;
  //}

  // Gestione operazione pendente verso epoll
  //EPendingOp &op = fdw->fd_pending_op[fd];

  //if (fdw->fd_registered[fd] && op == FDW_OP_NONE) {
  //  op = FDW_OP_MOD;
  //  fdwatch_mark_dirty(fdw, fd);
  //}

  return result;
}

/* ---------------------------------------------------------------------- */
/* fdwatch_register                                                       */
/* - NON chiama epoll_ctl                                                 */
/* - Aggiorna fd_rw_pending, fd_pending_op                                */
/* ---------------------------------------------------------------------- */

void fdwatch_register(LPFDWATCH fdw, int flag, int fd, int rw) {

  const bool registered = fdw->fd_registered[fd];

  if (flag == EV_DELETE) {
    fdw->fd_rw_pending[fd] = 0;
    fdw->fd_pending_op[fd] = registered ? FDW_OP_DEL : FDW_OP_NONE;
    fdwatch_mark_dirty(fdw, fd);
    return;
  }

  if (flag & EV_ADD) {

    int new_mask = fdw->fd_rw_pending[fd];

    if (rw == FDW_READ) {
      // prima togli entrambe le varianti, poi imposti quella giusta
      new_mask &= ~(FDW_READ | FDW_READ_ONESHOT);
      new_mask |= (flag & EV_ONESHOT) ? FDW_READ_ONESHOT : FDW_READ;

    } else if (rw == FDW_WRITE) {
      new_mask &= ~(FDW_WRITE | FDW_WRITE_ONESHOT);
      new_mask |= (flag & EV_ONESHOT) ? FDW_WRITE_ONESHOT : FDW_WRITE;
    }

    fdw->fd_rw_pending[fd] = new_mask;
    fdw->fd_pending_op[fd] = registered ? FDW_OP_MOD : FDW_OP_ADD;
    fdwatch_mark_dirty(fdw, fd);
  }
}

/* ---------------------------------------------------------------------- */
/* fdwatch_insert_fd                                                      */
/* - Inizializza le strutture interne per un nuovo fd (es. dopo accept()) */
/* - Azzera lo stato eventualmente residuo su quel numero di fd           */
/*   (riuso di descrittori già chiusi)                                    */
/* - Marca il fd come da aggiungere a epoll alla prossima fdwatch()       */
/* - NON imposta interessi READ/WRITE: vanno configurati con              */
/*   fdwatch_add_fd() / fdwatch_register()                                */
/* ---------------------------------------------------------------------- */

void fdwatch_insert_fd(LPFDWATCH fdw, socket_t fd) {
  if ((fd < 0) || (fd >= fdw->nfiles))
    return;

  fdw->fd_registered[fd] = false;
  fdw->fd_rw_pending[fd] = 0;
  fdw->fd_pending_op[fd] = FDW_OP_ADD;
  fdw->fd_data[fd] = nullptr;
  fdwatch_mark_dirty(fdw, fd);
}

/* ---------------------------------------------------------------------- */
/* fdwatch_add_fd                                                         */
/* - Solo gestione strutture dati                                         */
/* ---------------------------------------------------------------------- */

void fdwatch_add_fd(LPFDWATCH fdw, socket_t fd, void *client_data, int rw,
                    int oneshot) {

  if ((fd < 0) || (fd >= fdw->nfiles)) {
    return;
  }

  int flag = EV_ADD | (oneshot ? EV_ONESHOT : 0);

  fdw->fd_data[fd] = client_data;
  fdwatch_register(fdw, flag, fd, rw);
}

/* ---------------------------------------------------------------------- */
/* ACCESSORI                                                              */
/* ---------------------------------------------------------------------- */

void *fdwatch_get_client_data(LPFDWATCH fdw, unsigned int event_idx) {
  if (event_idx >= (unsigned int)fdw->nevents)
    return nullptr;

  int fd = fdw->events[event_idx].data.fd;

  if (fd < 0 || fd >= fdw->nfiles)
    return nullptr;

  return fdw->fd_data[fd];
}

void fdwatch_clear_fd(LPFDWATCH fdw, socket_t fd) {
  if (fd < 0 || fd >= fdw->nfiles)
    return;

  // fdw->fd_registered[fd] = false;
  fdw->fd_rw_current[fd] = 0;
  // fdw->fd_rw_pending[fd] = 0;
  // fdw->fd_pending_op[fd] = FDW_OP_NONE;
  fdw->fd_data[fd] = nullptr;
}

void fdwatch_del_fd(LPFDWATCH fdw, socket_t fd) {
  if (fd < 0 || fd >= fdw->nfiles)
    return;

  // Gestito come EV_DELETE: segniamo solo la DEL pendente
  fdwatch_register(fdw, EV_DELETE, fd, 0);
  fdwatch_clear_fd(fdw, fd);
}

void fdwatch_clear_event(LPFDWATCH fdw, socket_t fd, unsigned int event_idx) {
  (void)fdw;
  (void)fd;
  (void)event_idx;
  return; // PLACEHOLDER
}

int fdwatch_get_ident(LPFDWATCH fdw, unsigned int event_idx) {
  (void)fdw;
  (void)event_idx;
  return 0; // PLACEHOLDER
}

int fdwatch_get_buffer_size(LPFDWATCH fdw, socket_t fd) {
  (void)fdw;
  (void)fd;
  return INT_MAX; // placeholder
}
#endif
