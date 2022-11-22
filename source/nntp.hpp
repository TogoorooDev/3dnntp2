#ifndef NNTP_H
#define NNTP_H

#include <vector>

#include <3ds.h>

// const int NNTPERR_OK = 0;

#define NNTPERR_OK				0
#define NNTPERR_CON_FAILURE 	1
#define NNTPERR_INVALID_ADDR	2
#define NNTPERR_READ			3
#define NNTPERR_SEND			4
#define NNTPERR_ALLOC			5
#define NNTPERR_MEM				6
#define NNTPERR_RESOLUTION		7
#define NNTPERR_TIMEOUT			8
#define NNTPERR_SOCKCONF		9
#define NNTPERR_POLL			10

#define NNTPERR_UNKNOWN 		255

namespace nntp {
typedef struct {
	char **strs;
	u8 err;
} nntpstrarray;

typedef struct {
	char *str;
	u8 err;
} nntpstr;

typedef struct{
	char *article;
	u16 len;
	
} nntparticle;

typedef struct{
	nntparticle *news;
	u16 len;
	u8 err;
	
} nntpnews;

typedef struct{
	int socketfd;
	u8 err;
	u8 errno;
	
} nntpcon;

typedef struct{
	u32 size;
	char *contents;
	u8 err;
	
} nntpres;

typedef struct {
	std::vector<std::string> groups;
	u8 err;
	int errcode;
	
} nntpgroups;

nntpcon init(const char*, u16);
nntpres custom_command(char*, nntpcon);
nntpgroups get_groups(nntpcon con);
nntpgroups find_groups(std::string, u16, nntpgroups);


}
//#include "nntp.c"

#endif