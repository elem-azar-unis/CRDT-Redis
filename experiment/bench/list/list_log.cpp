//
// Created by admin on 2020/1/13.
//

#include "list_log.h"

#define __bold (1<<0)
#define __italic (1<<1)
#define __underline (1<<2)

double list_log::diff(const list_log::element &e, const redisReply *r)
{
    if (e.content != r->element[1]->str)return 1;
    if (e.font != atoi(r->element[2]->str))return 0.5;
    if (e.size != atoi(r->element[3]->str))return 0.5;
    if (e.color != atoi(r->element[4]->str))return 0.5;
    int p = atoi(r->element[5]->str);
    if (e.bold != (bool) (p & __bold))return 0.5;
    if (e.italic != (bool) (p & __italic))return 0.5;
    if (e.underline != (bool) (p & __underline))return 0.5;
    return 0;
}

