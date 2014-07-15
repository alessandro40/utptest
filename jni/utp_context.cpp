#include <android/log.h>

#include "utp_context.h"
#include "utp_socket.h"

// #ifdef DEBUG_UTP
#define debug(...) warnx(__VA_ARGS__)
//#else
// #define debug(...) (void) 0
// #endif

#define logd(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) 
#define warnx(...) logd("native", __VA_ARGS__)
#define warn(...) logd("native", __VA_ARGS__)
#define err(a, ...)  logd("err native", __VA_ARGS__)
#define errx(a, ...)  logd("err native", __VA_ARGS__)

void
Context_Wrapper::_handle_read_event(int fd, short event, void *opaque)
{
	byte buffer[8192];
	ssize_t result;
	struct sockaddr_storage sa;
	socklen_t salen;
	Context_Wrapper *context_w = (Context_Wrapper *) opaque;

	if ((event & EV_READ) != 0) {
		debug("_libevent_read");
		memset(&sa, 0, sizeof (sa));
		salen = sizeof (sa);

		if ((result = recvfrom(context_w->fd, (char *) buffer,
		    sizeof (buffer), 0, (struct sockaddr *) &sa, &salen)) > 0) {
			debug("%ld bytes read (UDP)", result);
			result = utp_process_udp(context_w->context_u, buffer,
			    (size_t) result, (const struct sockaddr *) &sa,
			    salen);
			debug("Successfully read utp pck: %ld", result);
			utp_issue_deferred_acks(context_w->context_u);
		}
		else
			warn("recvfrom() failed");
		debug("Out of _libevent_read");
	}	
}


void
Context_Wrapper::_handle_write_event(int fd, short event, void *opaque)
{
	Context_Wrapper *context_w = (Context_Wrapper *) opaque;
	if (context_w->context_u == NULL)
		return;
	if ((event & EV_WRITE) != 0 && !context_w->is_listener) {
//		debug("handle_write");
		if (context_w->is_writable)
			context_w->_utp_write();
//		else debug("UTP_socket not writable");
	}
}

void
Context_Wrapper::_handle_timeo_event(int fd, short event, void *opaque)
{
	//	debug("_libevent_timeo");
	Context_Wrapper *context_w = (Context_Wrapper *) opaque;
	utp_check_timeouts(context_w->context_u);
	/* TODO: fix it */
//	if (!context->is_listener && context->is_writable
//	    && connection->sock != NULL)
//		warnx("Goodput: %f Mbps", connection->get_goodput() / 1000000.0);
}

void
Context_Wrapper::_handle_sigint_event(int signo, short event, void *opaque)
{
	(void) signo;
	(void) event;

	debug("SIGINT caught");
	Context_Wrapper *context_w = (Context_Wrapper *) opaque;
	/* TODO: close all the connections */
}


Context_Wrapper::Context_Wrapper(int version, struct sockaddr_storage
	    *salocal_p, int write_size, bool is_listener)
{
	warnx("Context_Wrapper()");	
	struct sockaddr_in *sin;

	if ((this->salocal_p = salocal_p) == NULL)
		err(1, "salocal_p null");
	if ((this->write_size = write_size) <= 0)
		err(1, "write_size must be greater than zero");
	this->is_listener = is_listener;

	/* Create utp context */
	if ((this->context_u = utp_init(version)) == NULL)
		err(1, "utp_init() failed");

	warnx("Create socket()");	

	/* Create socket */
	if ((this->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err(1, "socket() failed");
	warnx("socket() done");	
	if (evutil_make_socket_nonblocking(this->fd) < 0)
		errx(1, "evutil_make_socket_nonblocking() failed");

	sin = (struct sockaddr_in *) this->salocal_p;	
	if (bind(this->fd, (struct sockaddr *) sin, sizeof(*sin)) < 0)
		err(1, "bind() failed");

        utp_context_set_userdata(this->context_u, (void *) this); /* Or &this? */
        utp_set_callback(this->context_u, UTP_ON_ACCEPT, &this->_utp_accept);
        utp_set_callback(this->context_u, UTP_ON_ERROR, &this->_utp_error);
        utp_set_callback(this->context_u, UTP_SENDTO, &this->_utp_sendto);
        utp_set_callback(this->context_u, UTP_ON_STATE_CHANGE,
            &this->_utp_state_change);

        if (this->is_listener) {
                utp_set_callback(this->context_u, UTP_ON_READ, &this->_utp_read);
        }

	/* Add event handlers */
	if ((this->ev_base = event_base_new()) == NULL) {
                err(1, "event_base_new() failed");
                delete this;
        }

	// _libevent_read = _handle_read_event;
        if ((this->ev_read = event_new(this->ev_base, this->fd, EV_READ |
	    EV_PERSIST, _handle_read_event, this)) == NULL) {
                err(1, "event_new() failed");
                delete this;
        }

        if (event_add(this->ev_read, NULL) < 0) {
                err(1, "event_add() failed");
                delete this;
        }

//	_libevent_read = _handle_read_event;
        if ((this->ev_write = event_new(this->ev_base, this->fd, EV_WRITE |
	    EV_PERSIST, this->_handle_write_event, this)) == NULL) {
                err(1, "event_new() failed");
                delete this;
        }

        if (event_add(this->ev_write, NULL) < 0) {
                err(1, "event_add() failed");
                delete this;
        }

//	_libevent_write = _handle_write_event;
        if ((this->ev_timeo = event_new(this->ev_base, this->fd, EV_PERSIST,
            this->_handle_timeo_event, this)) == NULL) {
                err(1, "event_new() failed");
                delete this;
        }

        memset(&tv, 0, sizeof(tv));
        tv.tv_usec = TIMEO_CHECK_INTERVAL;
        if (event_add(this->ev_timeo, &tv) < 0) {
                err(1, "event_add() failed");
                delete this;
        }
/*
 * #ifndef WIN32
 *	_libevent_sigint = _handle_sigint_event;
 *       if ((this->ev_signal = event_new(this->ev_base, SIGINT, EV_SIGNAL,
 *           this->_handle_sigint_event, this)) == NULL) {
 *               err(1, "event_new() failed");
 *               delete this;
 *       }
 *
 *       if (event_add(this->ev_signal, NULL) != 0) {
 *               err(1, "event_add() failed");
 *               delete this;
 *       }
 * #endif
 */
}

Context_Wrapper::~Context_Wrapper()
{
	if (this->ev_base != NULL)
		free(this->ev_base);
	if (this->ev_read != NULL)
		free(this->ev_read);
	if (this->ev_write != NULL)
		free(this->ev_write);
	if (this->ev_timeo != NULL)
		free(this->ev_timeo);
	if (this->ev_signal != NULL)
		free(this->ev_signal);
		
}

void
Context_Wrapper::dispatch(void)
{
	debug("Dispatch event_base");
	if (event_base_dispatch(this->ev_base) != 0) {
		errx(1, "event_base_dispatch() failed");
		delete this;
	}
}

ssize_t
Context_Wrapper::_utp_write(void)
{
	ssize_t result = 0;
	if (this->is_writable) {
		char buf[this->write_size];
		memset(buf, 'A', this->write_size);
		/* TODO: where to write? */
		if (this->socket_tmp[0] != NULL)
			result = utp_write(this->socket_tmp[0], buf,
			    this->write_size); /*XXX*/
		if (this->socket_tmp[1] != NULL)
			result = utp_write(this->socket_tmp[1], buf,
			    this->write_size); /*XXX*/

	}
	else debug("Socket non writable");
	return (result);
}

utp_context_stats *
Context_Wrapper::get_stats()
{
	return utp_get_context_stats(this->context_u);
}

uint64
Context_Wrapper::_utp_read(utp_callback_arguments *args)
{
	debug("_utp_read");
	const unsigned char *p;
	ssize_t len, left;

	left = args->len;
	p = args->buf;

	while(left) {
		len = write(STDOUT_FILENO, p, left);
		left -= len;
		p += len;
		debug("Wrote %ld bytes, %ld left", len, left);
	}

	utp_read_drained(args->socket);
	return (0);
}

uint64
Context_Wrapper::_utp_error(utp_callback_arguments *args)
{
	fprintf(stderr, "utp_error: %s\n",
	    utp_error_code_names[args->error_code]);
//	connection->disable_sigint_handler(); /* TODO: fix it */

	return (0);
}

uint64
Context_Wrapper::_utp_accept(utp_callback_arguments *args)
{
	debug("_utp_ accept cb");
	if (args->socket == NULL)
		errx(1, "utp_socket null");
	/* 
	 * TODO: does it have to do something? Maybe not, because the BSD socket
	 * is already "controlled" 
	 */

	return (0);
}

uint64
Context_Wrapper::_utp_sendto(utp_callback_arguments *args)
{
	debug("_utp_ sendto cb");
	ssize_t result;
	Context_Wrapper *context_w = (Context_Wrapper *)
	    utp_context_get_userdata(args->context);

	result = sendto(context_w->fd, (char *) args->buf, args->len, 0,
	    args->address, args->address_len);

	if (result < 0)
		warnx("sendto() failed");
	else
		debug("%ld bytes sent (UDP)", result);

	return (0);
}

uint64
Context_Wrapper::_utp_state_change(utp_callback_arguments *args)
{
	debug("state %d: %s\n", args->state, utp_state_names[args->state]);
	utp_socket_stats *socket_stats;
	utp_context_stats *context_stats;

	Socket_Wrapper *socket_w = (Socket_Wrapper *)
	    utp_get_userdata(args->socket);
	Context_Wrapper *context_w = (Context_Wrapper *)
	    utp_context_get_userdata(args->context);
 
	switch (args->state) {
	case UTP_STATE_CONNECT:
		context_w->is_writable = 1;
		break;

	case UTP_STATE_WRITABLE:
		context_w->is_writable = 1;
		break;

	case UTP_STATE_EOF:
		debug("Received EOF from socket; closing\n");
		context_w->is_writable = 0;
		utp_close(socket_w->socket_u); /* TODO: it should be a method of
						Socket_Wrapper */
//		/* TODO: fix it */ connection->disable_sigint_handler();
		break;

	case UTP_STATE_DESTROYING:
		debug("UTP socket is being destroyed; exiting\n");
		context_w->is_writable = 0;

		context_stats = context_w->get_stats();
		if (context_stats) {
			printf("\n           Bucket size:    <23");
			printf("    <373    <723    <1400    >1400\n");
			printf("Number of packets sent:  %5d   %5d   %5d",
			    context_stats->_nraw_send[0],
			    context_stats->_nraw_send[1],
			    context_stats->_nraw_send[2]);
			printf("    %5d    %5d\n", context_stats->_nraw_send[3],
			    context_stats->_nraw_send[4]);
			printf("Number of packets recv:  %5d   %5d   %5d",
			    context_stats->_nraw_recv[0],
			    context_stats->_nraw_recv[1],
			    context_stats->_nraw_recv[2]);
			printf("    %5d    %5d\n", context_stats->_nraw_recv[3],
			    context_stats->_nraw_recv[4]);
		}    		
		else warn("utp_get_socket_stats() failed");

		socket_stats = socket_w->get_stats();
		if (socket_stats) {	
			printf("\nSocket Statistics:\n");
			printf("    Bytes sent:          %lld\n",
			    socket_stats->nbytes_xmit);
			printf("    Bytes received:      %lld\n",
			    socket_stats->nbytes_recv);
			printf("    Packets received:    %d\n",
			    socket_stats->nrecv);
			printf("    Packets sent:        %d\n",
			    socket_stats->nxmit);
			printf("    Duplicate receives:  %d\n",
			    socket_stats->nduprecv);
			printf("    Retransmits:         %d\n",
			    socket_stats->rexmit);
			printf("    Fast Retransmits:    %d\n",
			    socket_stats->fastrexmit);
			printf("    Best guess at MTU:   %d\n",
			    socket_stats->mtu_guess);
		}
		else
			err(1, "utp_get_context_stats() failed");
		break;
	}

	return 0;
}
