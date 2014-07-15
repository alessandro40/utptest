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

class Context_Wrapper;
class Socket_Wrapper {
public:
	Socket_Wrapper(Context_Wrapper *, struct sockaddr_storage *, ssize_t);
	~Socket_Wrapper();
	utp_socket_stats *get_stats();

	/* No more needed */
//	std::function<void(int, short, void *)> _libevent_read; /* Initialize? */
//	std::function<void(int, short, void *)> _libevent_write; /* Initialize? */
//	std::function<void(int, short, void *)> _libevent_timeo; /* Initialize? */
//	std::function<void(int, short, void *)> _libevent_sigint; /* Initialize? */

	utp_socket *socket_u = NULL;
	Context_Wrapper *context_w = NULL;
	
	bool closing = false;
	ssize_t write_size = 128;

	struct sockaddr_storage *saremote_p = NULL;
};
