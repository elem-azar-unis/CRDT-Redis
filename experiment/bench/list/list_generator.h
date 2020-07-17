//
// Created by admin on 2020/7/16.
//

#ifndef BENCH_LIST_GENERATOR_H
#define BENCH_LIST_GENERATOR_H

#include "../constants.h"
#include "../util.h"
#include "list_basics.h"
#include "list_cmd.h"
#include "list_log.h"

class list_generator : public generator
{
private:
    class id_gen
    {
    private:
        int id, count = 0;

        static int index(thread::id tid);

    public:
        id_gen()
        {
            id = index(this_thread::get_id());
        }

        string get()
        {
            char tmp[16];
            sprintf(tmp, "%d,%d", id, count);
            count++;
            return string(tmp);
        }
    } new_id;

    list_op_gen_pattern *pattern;
    list_log &list;
    list_type type;
    // TODO record_for_collision<string> add;

public:
    list_generator(list_type type, list_log &list, const char *p) : type(type), list(list)
    {
        // TODO add_record(add); start_maintaining_records();

        if (p == nullptr)
            pattern = &list_pt_dft;
        // TODO pattern?
    }

    void gen_and_exec(redis_client &c) override;
};

#endif //BENCH_LIST_GENERATOR_H
