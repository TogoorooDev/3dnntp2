
// basic c libs
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

// other c libs
#include <unistd.h>
#include <limits.h>
#include <poll.h>
#include <errno.h>

// berkeley socket libs
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

// c++ libs
#include <iostream>
#include <sstream>
#include <string>

// 3ds specific libs
#include <3ds.h>

// local libs
#include "nntp.hpp"

using std::vector;
using std::string;

namespace nntp {
	// nntpcon init(const char*server, u16 port){
	// 		struct sockaddr_in addr;
	// 		int sock, pollres, recv_res;
	// 		struct hostent *ip_ent;
	// 		struct pollfd watchlist[1];

	// 		nntpcon con;

	// 		sock = socket(AP_INET, SOCK_STREAM, 0);
	// 		addr.sin_family = AF_INET;
	// 		addr.sin_port = htons(port);
	// 		ip_end = gethostbyname("google.com");

	// }


	nntpcon init(const char *server, u16 port){
		struct sockaddr_in addr;
		int sock, pollres, recv_res;
		struct hostent *ip_ent;
		nntpcon con;
		struct pollfd watchlist[1];

		// con.errno = 0;
		
		sock = socket(AF_INET, SOCK_STREAM, 0);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		ip_ent = gethostbyname(server);
		//ip_ent = gethostbyaddr(server, sizeof(server), AF_INET);
		if (ip_ent == NULL){
			switch (h_errno){
				case HOST_NOT_FOUND:
					printf("Host not found\n");
				case NO_DATA:
					printf("Host has no corresponding address\n");
				case TRY_AGAIN:
					printf("Recoverable error. TODO: recover and try again\n");
				case NO_RECOVERY:
					printf("Unknown error\n");
				default:
					return con;
			}
			printf("Errno: %d\n", h_errno);
			// con.errno = h_errno;
			con.err = NNTPERR_RESOLUTION;
			con.socketfd = -1;
			return con;
		}
		
		memcpy(&addr.sin_addr, ip_ent->h_addr_list[0], ip_ent->h_length);
		printf("connecting\n");
		
		char *discard = (char*)malloc(16);
		
		if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
			con.err = NNTPERR_CON_FAILURE;
			con.socketfd = -1;
			return con;
		}
		
		fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
		
		watchlist[0].fd = sock;
		watchlist[0].events = POLLIN;
		
		// Flush buffer
		while ((pollres = poll(watchlist, 1, 2500)) != 0){
			if (pollres == -1){
				con.err = NNTPERR_POLL;
				return con;
			}
			if ((recv_res = recv(sock, discard, 1, 0)) != 0){
				if (recv_res == -1){
					con.err = NNTPERR_READ;
					return con;
				}
				
			}	
		}
		
		free(discard);
		
		printf("connected\n");
		con.err = NNTPERR_OK;
		con.socketfd = sock;
		return con;
		
	}

	nntpgroups get_groups(nntpcon con){
		nntpgroups out;
		out.err = NNTPERR_UNKNOWN;
		out.groups = std::vector<std::string>();
		
		char *working;
		
		u64 bufsize = 8192;
		char *buf = (char*)malloc(bufsize);
		u32 bufpos = 0;
		char *last_buf;
		
		u64 backup_buf_size;
		char *backup_buf;
		
		u32 groupsize = sizeof(char*) * 65534;
		
		int recv_res = 0;
		
		int pollres;
		struct pollfd watchlist[1];
		watchlist[0].fd = con.socketfd;
		watchlist[0].events = POLLIN;
		
		fcntl(con.socketfd, F_SETFL, fcntl(con.socketfd, F_GETFL, 0) & ~O_NONBLOCK);
		
		send(con.socketfd, "LIST\r\n", strlen("LIST\r\n") + 1, 0);
		
		fcntl(con.socketfd, F_SETFL, fcntl(con.socketfd, F_GETFL, 0) | O_NONBLOCK);
		
		while ((pollres = poll(watchlist, 1, 5000)) != 0){
			if (pollres == -1){
				//printf("Poll error\n");
				out.err = NNTPERR_POLL;
				out.groups.clear();
				return out;
			}
			last_buf = buf;
			
			if ((bufsize - bufpos) < 8192){
				bufsize += 8192;
				buf = (char*)realloc(buf, bufsize);
				if (buf == NULL){
					out.err = NNTPERR_ALLOC;
					free(last_buf);
					out.groups.clear();
					
					printf(".");
					fflush(stdout);
					
					return out;
				}
			}
			
			recv_res = recv(con.socketfd, buf + bufpos, bufsize - bufpos, 0);
			if (recv_res == -1){
				printf("Read error\n");
				out.groups.clear();
				out.err = NNTPERR_READ;
				out.errcode = errno;
				return out;
			}
			bufpos += recv_res;
		}
		
		printf("\n");
		
		buf[bufpos] = '\0';
		backup_buf = (char*)malloc(bufsize);
		backup_buf_size = bufsize;
		
		strncpy(backup_buf, buf, backup_buf_size);

		working = strtok(buf, "\n");
		while (working != NULL){
			working[strlen(working)] = '\0';	
			out.groups.push_back(std::string(working));
			working = strtok(NULL, "\n");
		}

		for (u32 i = 0; i < out.groups.size(); i++){
			std::istringstream f(out.groups.at(i));
			std::string ns;

			std::getline(f, ns, ' ');
			out.groups.at(i) = ns;
		}

		free(buf);
		//free(thaiboy);
		out.err = NNTPERR_OK;
		return out;
	}

	nntpgroups find_groups(string group, u16 level, nntpgroups groupslist){
		nntpgroups out;
		out.err = NNTPERR_UNKNOWN;
		out.groups = vector<string>(); 

		auto mgroups = vector<string>();

		for (auto group : groupslist.groups){
			auto levels = vector<string>();
			std::istringstream gstream(group);
			string token;
			while (std::getline(gstream, token, '.')){
				levels.push_back(token);
				std::cout << token << std::endl;
			}
			if (levels.size() >= level && levels.at(level) == group)
				mgroups.push_back(group);
		}



		return out;
	}

	u8 group_select(char *group, nntpcon con){
		u8 out = NNTPERR_UNKNOWN;
		int recv_res;
		
		char *working;
		
		u64 bufsize = 8192;
		char *buf = (char*)malloc(bufsize);
		u32 bufpos = 0;
		char *last_buf;
		
		u64 backup_buf_size;
		char *backup_buf;
		
		int pollres;
		struct pollfd watchlist[1];
		watchlist[0].fd = con.socketfd;
		watchlist[0].events = POLLIN;
		
		fcntl(con.socketfd, F_SETFL, fcntl(con.socketfd, F_GETFL, 0) & ~O_NONBLOCK);
		
		char *g = (char*)calloc(strlen(group) + strlen("GROUP ") + 1, sizeof(char*));
		sprintf(g, "GROUP %s\n", group);
		
		send(con.socketfd, "S\r\n", strlen(g) + 1, 0);
		
		fcntl(con.socketfd, F_SETFL, fcntl(con.socketfd, F_GETFL, 0) | O_NONBLOCK);
		
		while ((pollres = poll(watchlist, 1, 5000)) != 0){
			if (pollres == -1){
				out = NNTPERR_POLL;
				return out;
			}
			last_buf = buf;
			
			if ((bufsize - bufpos) < 8192){
				bufsize += 8192;
				buf = (char*)realloc(buf, bufsize);
				if (buf == NULL){
					out = NNTPERR_ALLOC;
					free(last_buf);

					return out;
				}
			}
			
			recv_res = recv(con.socketfd, buf + bufpos, bufsize - bufpos, 0);
			if (recv_res == -1){
				printf("Read error\n");
				out = NNTPERR_READ;
				return out;
			}
			bufpos += recv_res;
		}
		
		return out;
	}

	nntpnews get_news(nntpcon con){
		nntpnews out;
		out.err = NNTPERR_UNKNOWN;
		out.news = NULL;
		out.len = 0;
		
		char *working;
		
		u64 bufsize = 8192;
		char *buf = (char*)malloc(bufsize);
		u32 bufpos = 0;
		char *last_buf;
		
		u64 backup_buf_size;
		char *backup_buf;
		
		u32 newssize = sizeof(char*) * 65534;
		
		int recv_res = 0;
		
		int pollres;
		struct pollfd watchlist[1];
		watchlist[0].fd = con.socketfd;
		watchlist[0].events = POLLIN;
		
		fcntl(con.socketfd, F_SETFL, fcntl(con.socketfd, F_GETFL, 0) & ~O_NONBLOCK);
		
		send(con.socketfd, "NEWNEWS * 220829 043300\r\n", strlen("NEWNEWS * 220829 043300\r\n") + 1, 0);
		
		fcntl(con.socketfd, F_SETFL, fcntl(con.socketfd, F_GETFL, 0) | O_NONBLOCK);
		
		//out.news = malloc(groupsize);
		
		out.news = (nntparticle*)calloc(sizeof(nntparticle), 65534);
		
		while ((pollres = poll(watchlist, 1, 5000)) != 0){
			if (pollres == -1){
				//printf("Poll error\n");
				out.err = NNTPERR_POLL;
				out.news = NULL;
				return out;
			}
			last_buf = buf;
			
			if ((bufsize - bufpos) < 8192){
				bufsize += 8192;
				buf = (char*)realloc(buf, bufsize);
				if (buf == NULL){
					out.err = NNTPERR_ALLOC;
					free(last_buf);
					free(out.news);
					
					
					printf(".");
					fflush(stdout);
					
					return out;
				}
			}
			
			recv_res = recv(con.socketfd, buf + bufpos, bufsize - bufpos, 0);
			if (recv_res == -1){
				printf("Read error\n");
				out.news = NULL;
				out.err = NNTPERR_READ;
				return out;
			}
			bufpos += recv_res;
		}
		
		printf("\n");
		
		buf[bufpos] = '\0';
		backup_buf = (char*)malloc(bufsize + 1);
		backup_buf_size = bufsize;
		
		strncpy(backup_buf, buf, backup_buf_size);
		//backup_buf[backup_buf_size - 1] = '\0';
		out.len = 0;
		
		working = strtok(buf, "\n");
		while (working != NULL){
			out.news[out.len].article = (char*)calloc(sizeof(char), strlen(working) + 1);
			working[strlen(working)] = '\0';
			last_buf = (char*)memcpy(out.news[out.len].article, working, strlen(working) + 1);
			if (last_buf == NULL){
				out.err = NNTPERR_MEM;
				return out;
			}
			
			out.news[out.len].len = strlen(working) + 1;
			
			out.len++;
			working = strtok(NULL, "\n");
		}

		free(buf);
		out.err = NNTPERR_OK;
		
		return out;
	}
}