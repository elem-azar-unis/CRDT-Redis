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
    // TODO conflicts?
    struct list_op_gen_pattern
    {
        double PR_ADD;
        double PR_UPD;
        double PR_REM;
    };

    class id_gen
    {
    private:
        int id, count = 0;

        static int index(thread::id tid);

    public:
        id_gen() { id = index(this_thread::get_id()); }

        string get()
        {
            ostringstream stream;
            stream << id << "," << count;
            count++;
            return stream.str();
        }
    } new_id;

    list_op_gen_pattern &pattern;
    list_log &list;
    const string &type;
    // TODO record_for_collision<string> add;

    static list_op_gen_pattern &get_pattern(const string &name);

public:
    list_generator(const string &type, list_log &list, const string &p)
        : type(type), list(list), pattern(get_pattern(p))
    {
        // TODO add_record(add); start_maintaining_records();
    }

    void gen_and_exec(redis_client &c) override;
};

#endif  // BENCH_LIST_GENERATOR_H
