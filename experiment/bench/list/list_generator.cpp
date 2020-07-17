//
// Created by admin on 2020/7/16.
//

#include <map>
#include "list_generator.h"

#define DEFINE_ACTION(_name) #_name,
const char *list_type_str[] = {LIST_TYPE_CODEC(DEFINE_ACTION)};
#undef DEFINE_ACTION

#define PA (pattern->PR_ADD)
#define PU (pattern->PR_ADD + pattern->PR_UPD)

list_op_gen_pattern list_pt_dft{
    .PR_ADD = 0.41,
    .PR_UPD = 0.2,
    .PR_REM = 0.39,
};

int list_generator::id_gen::index(thread::id tid)
{
    static int nextindex = 0;
    static mutex my_mutex;
    static map<thread::id, int> ids;
    lock_guard<mutex> lock(my_mutex);
    if (ids.find(tid) == ids.end())
        ids[tid] = nextindex++;
    return ids[tid];
}

void list_generator::gen_and_exec(redis_client &c)
{
    double rand = decide();
    // TODO conflicts?
    if (rand <= PA)
    {
        string prev = list.random_get(),
               id = new_id.get(),
               content = strRand();
        int font = intRand(TOTAL_FONT_TYPE),
            size = intRand(MAX_FONT_SIZE),
            color = intRand(MAX_COLOR);
        bool bold = boolRand(),
             italic = boolRand(),
             underline = boolRand();
        list_add_cmd(type, list, prev, id, content,
                     font, size, color, bold, italic, underline)
            .exec(c);
    }
    else if (rand <= PU)
    {
        string id = list.random_get();
        if (id == "null") return;
        string upd_type;
        int value;
        switch (intRand(6))
        {
        case 0:
            upd_type = "font";
            value = intRand(TOTAL_FONT_TYPE);
            break;
        case 1:
            upd_type = "size";
            value = intRand(MAX_FONT_SIZE);
            break;
        case 2:
            upd_type = "color";
            value = intRand(MAX_COLOR);
            break;
        case 3:
            upd_type = "bold";
            value = boolRand();
            break;
        case 4:
            upd_type = "italic";
            value = boolRand();
            break;
        default:
            upd_type = "underline";
            value = boolRand();
            break;
        }
        list_update_cmd(type, list, id, upd_type, value).exec(c);
    }
    else
    {
        string id = list.random_get();
        if (id == "null") return;
        list_remove_cmd(type, list, id).exec(c);
    }
}
