#include <csignal>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <unordered_map>
#include <unordered_set>
#include <sys/epoll.h>

#define MAX_HDR				1024
#define MAX_EVENTS			20
#define PERSISTENT_CONN		0b01

using namespace std;
namespace fs = filesystem;

auto root_dir_str = "./"; /* root directory */
auto errMessage400 = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
auto errMessage403 = "HTTP/1.0 403 Forbidden\r\nConnection: close\r\n\r\n";
auto errMessage404 = "HTTP/1.0 404 Not Found\r\nConnection: close\r\n\r\n";
auto errMessage500 = "HTTP/1.0 500 Internal Server Error\r\nConnection: close\r\n\r\n";

typedef struct SContext {
	char *buf;
	long int buf_len;
	long int bytes_handled;
	short context_info;

	SContext(): buf(static_cast<char *>(malloc(MAX_HDR))), buf_len(MAX_HDR), bytes_handled(0), context_info(0) {
	};
} SContext;

void print_usage(const char* prog_name) {
	cout << "usage: " << prog_name << " -p PORT (-d WORKING_DIRECTORY)\n";
}

int main(int argc, char **argv) {
	const char *port = nullptr;
	/* argument parsing */
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-p") == 0 && (i + 1) < argc) {
			port = argv[i + 1];
			i++;
		} else if (strcmp(argv[i], "-d") == 0 && (i + 1) < argc) {
			root_dir_str = argv[i + 1];
			i++;
		}
	}
	if (!port || strtol(port, nullptr, 10) <= 0 || strtol(port, nullptr, 10) > 65535) {
		print_usage(argv[0]);
		exit(-1);
	}

	const fs::path root_dir = canonical(static_cast<fs::path>(root_dir_str));
	if(access(root_dir.c_str(), R_OK | X_OK) < 0) {
		cerr << "dir " << root_dir << " is not accessable\n";
		print_usage(argv[0]);
		exit(-1);
	}
	signal(SIGPIPE, SIG_IGN);

	// following beej's networking guide
	addrinfo hints = {0}, *res;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(nullptr, port, &hints, &res) < 0) {
		cerr << "can't get an address for port " << port << '\n';
		exit(-1);
	}

	int listening_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(listening_sock < 0) {
		cerr << "can't not create a listening socket\n";
		exit(-1);
	}

	if(bind(listening_sock, res->ai_addr, res->ai_addrlen) < 0) {
		cerr << "can't bind socket to port " << port << '\n';
		exit(-1);
	}

	if (fcntl(listening_sock, F_SETFL, O_NONBLOCK) < 0) {
		cerr << "can't set listening socket to non-blocking\n";
		close(listening_sock);
		exit(-1);
	}

	listen(listening_sock, MAX_EVENTS);
	const int epollfd = epoll_create(1);
	epoll_event ev = {0};
	ev.events = EPOLLIN;
	ev.data.fd = epollfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listening_sock, &ev) < 0) {
		cerr << "Cannot add listening socket to epoll\n";
		exit(-1);
	}
	epoll_event events[MAX_EVENTS];

	int conn_s;
	unordered_map<int, SContext*> sock_ctx;

	for(;;) {
		const int fds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
		int inc = 0;
		for(int i = 0; i < fds; ++i) {
			if(events[i].data.fd == listening_sock)
				// new client asking for a connection
				++inc;
			else if(events[i].events == EPOLLIN) {
				// handle an incoming request
				const int cur_s = events[i].data.fd;
				auto cur_context = sock_ctx[cur_s];

			}
		}
	}
}
