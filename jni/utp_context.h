#include <sys/types.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <errno.h>
#include <err.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <functional>

#include "libevent/event.h"
#include "strtonum.h"
#include "libutp/utp.h"

#ifdef DEBUG_UTP
#define debug(...) warnx(__VA_ARGS__)
#else
#define debug(...) (void) 0
#endif

#define TIMEO_CHECK_INTERVAL	500000

class Socket_Wrapper;
class Context_Wrapper {
	struct sockaddr_storage *salocal_p;
	int write_size;

	struct event_base *ev_base = NULL;
	struct event *ev_read = NULL;
	struct event *ev_write = NULL;
	struct event *ev_timeo = NULL;
	struct event *ev_signal = NULL;
	struct timeval tv;
	int start_amount = 0;
	struct timeval start_time;

	/* TODO: how to use C++11 to improve these callbacks? */

	/* utp callbacks */
	static uint64 _utp_accept(utp_callback_arguments *);
	static uint64 _utp_error(utp_callback_arguments *);
	static uint64 _utp_read(utp_callback_arguments *);
	static uint64 _utp_sendto(utp_callback_arguments *);
	static uint64 _utp_state_change(utp_callback_arguments *);

	/* LibEvent callbacks */
	static void _handle_read_event(int, short, void *);
	static void _handle_write_event(int, short, void *);
	static void _handle_timeo_event(int, short, void *);
	static void _handle_sigint_event(int, short, void *);

//	std::function<void(int, short, void *)> _libevent_read;
//	std::function<void(int, short, void *)> _libevent_write;
//	std::function<void(int, short, void *)> _libevent_timeo;
//	std::function<void(int, short, void *)> _libevent_sigint;

    public:
	Context_Wrapper(int, sockaddr_storage *, int, bool);
	~Context_Wrapper(void);
	void dispatch();
	utp_context_stats *get_stats(void);
	ssize_t _utp_write(void);

	utp_socket *socket_tmp[2] = {NULL, NULL} ; /* XXX: just for testing */

	utp_context *context_u;
	int fd = -1;
	bool is_listener = false;
	bool is_writable = false;
};
