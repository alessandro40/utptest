#include<android/log.h>

#include "utp_socket.h"
#include "utp_context.h"

#define logd(LOG_TAG, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__) 
#define warnx(...) logd("native", __VA_ARGS__)

int
send()
{
	warnx("main");
	struct sockaddr_storage salocal, saremote1, saremote2;
	struct sockaddr_in * sin;
	ssize_t write_size = 256;
	bool is_listener = false;

	memset(&salocal, 0, sizeof (struct sockaddr_storage));
	sin = (struct sockaddr_in *) &salocal;
	sin->sin_family = AF_INET;
	sin->sin_port = htons(54321);

	memset(&saremote1, 0, sizeof (struct sockaddr_storage));
	sin = (struct sockaddr_in *) &saremote1;
	sin->sin_family = AF_INET;
	sin->sin_port = htons(54321);
	if (inet_pton(AF_INET, "192.168.1.108", &sin->sin_addr) < 1)
		err(1, "inet_pton() failed");

	warnx("Context_Wrapper");
	Context_Wrapper *context_sender = new Context_Wrapper(2, &salocal,
	    write_size, false);

	warnx("Socket_Wrapper");
	Socket_Wrapper *socket_1 = new Socket_Wrapper(context_sender, &saremote1,
	    write_size);
	
	warnx("Dispatch");

	context_sender->dispatch();

	return 0;
}

int
listen()
{
	warnx("main");
	struct sockaddr_storage salocal, saremote1, saremote2;
	struct sockaddr_in * sin;
	ssize_t write_size = 256;
	bool is_listener = false;

	memset(&salocal, 0, sizeof (struct sockaddr_storage));
	sin = (struct sockaddr_in *) &salocal;
	sin->sin_family = AF_INET;
	sin->sin_port = htons(54321);

	warnx("Context_Wrapper");
	Context_Wrapper *context_sender = new Context_Wrapper(2, &salocal,
	    write_size, true);

	warnx("Dispatch");

	context_sender->dispatch();

	return 0;
}
