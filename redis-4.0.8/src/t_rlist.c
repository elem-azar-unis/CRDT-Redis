//
// Created by user on 20-04-12.
//
#include "server.h"
#include "RWFramework.h"
#include "list_basics.h"

#define DEFINE_NPR(p) vc *p##_t;int p;
#define DEFINE_PR(p) vc *p##_t;
typedef struct rlist_aset_element
{
    vc* t;
    FORALL_NPR(DEFINE_NPR);
    FORALL_PR(DEFINE_PR);
    int property;
}rl_ase;
#undef DEFINE_NPR
#undef DEFINE_PR

rl_ase * rl_aseNew(vc* t)
{
    rl_ase *a = zmalloc(sizeof(rl_ase));
    a->t = duplicateVC(t);
#define TMP_ACTION(p) a->p##_t = duplicateVC(t);
    FORALL(TMP_ACTION);
#undef TMP_ACTION
    return a;
}

typedef struct rw_list_element
{
    sds oid;
    leid *pos_id;
    sds content;

    vc *current;
    rl_ase *value;
    list *aset;
    list *rset;
    list *ops;

    struct rw_list_element *prev;
    struct rw_list_element *next;
}rle;

rle* rleNew()
{
    rle* e = zmalloc(sizeof(rle));
    e->oid = NULL;
    e->pos_id = NULL;
    e->content = NULL;

    e->current = l_newVC;
    e->value = NULL;
    e->aset = listCreate();
    e->rset = listCreate();
    e->ops = listCreate();

    e->prev = NULL;
    e->next = NULL;
    return e;
}
