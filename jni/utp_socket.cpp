#include <android/log.h>

#include "utp_socket.h"
#include "utp_context.h"

//#ifdef DEBUG_UTP
#define debug(...) warnx(__VA_ARGS__)
//#else
//#define debug(...) (void) 0
//#endif

#define logd(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) 
#define warnx(...) logd("native", __VA_ARGS__)

/* Constructor for outgoing connections (as sender) */
Socket_Wrapper::Socket_Wrapper(Context_Wrapper *ctx, struct sockaddr_storage
	    *saremote_p, ssize_t write_size)
{
	struct sockaddr_in *sin;

	if ((this->context_w = ctx) == NULL)
		err(1, "ctx null");
	if ((this->saremote_p = saremote_p) == NULL)
		err(1, "saremote_p null");
	if ((this->write_size = write_size) <= 0)
		err(1, "write_size must be greater than zero");

	/* Create utp_socket */
	
	sin = (struct sockaddr_in *) this->saremote_p;
	debug("Create utp_socket");		
	if ((this->socket_u = utp_create_socket(this->context_w->context_u)) == NULL) {
		delete this;
		errx(1, "utp_create_socket() failed");
	}
	debug("Connect utp_socket");
	if (utp_connect(this->socket_u, (struct sockaddr *) sin,
	    sizeof(*sin)) < 0) {
		delete this;
		errx(1, "utp_connect() failed");
	}

	utp_set_userdata(this->socket_u, (void *) this);
	this->context_w->socket_tmp[1] = this->context_w->socket_tmp[0];
	this->context_w->socket_tmp[0] = this->socket_u;
}

Socket_Wrapper::~Socket_Wrapper()
{
	if (this->socket_u != NULL)
		free(this->socket_u);
	if (this->context_w != NULL)
		free(this->context_w);
}

utp_socket_stats*
Socket_Wrapper::get_stats()
{
	return utp_get_stats(this->socket_u);
}
