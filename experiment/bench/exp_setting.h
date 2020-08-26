//
// Created by admin on 2020/4/15.
//

#ifndef BENCH_EXP_SETTING_H
#define BENCH_EXP_SETTING_H

#include <cassert>
#include <iostream>

#define TOTAL_SERVERS (exp_setting::total_clusters * exp_setting::server_per_cluster)

using namespace std;

class exp_setting
{
public:
    struct default_setting
    {
        int delay;
        int delay_low;
        int total_clusters;
        int server_per_cluster;
        int op_per_sec;

        struct
        {
            int start, end, step;

            inline int times() const { return (end - start) / step + 1; }
        } speed_e, replica_e, delay_e;
    };

private:
    static const char *alg_type;
    static const char *rdt_type;
    static constexpr int total_sec = 10000;
    static default_setting *default_p;

    static inline void apply_default()
    {
        assert(default_p != nullptr);
        delay = default_p->delay;
        delay_low = default_p->delay_low;
        total_clusters = default_p->total_clusters;
        server_per_cluster = default_p->server_per_cluster;
        op_per_sec = default_p->op_per_sec;
    }

public:
    static int delay;
    static int delay_low;
    static int total_clusters;
    static int server_per_cluster;
    static int total_ops;
    static int op_per_sec;

#define EXP_TYPE_CODEC(ACTION) \
    ACTION(speed)              \
    ACTION(replica)            \
    ACTION(delay)              \
    ACTION(pattern)

#define DEFINE_ACTION(_name) _name,
    static enum class exp_type { EXP_TYPE_CODEC(DEFINE_ACTION) } type;
#undef DEFINE_ACTION

    static const char *type_str[];
    static string pattern_name;
    static int round_num;

    static inline void set_default(default_setting *p) { default_p = p; }

    static inline void set_exp_subject(const char *alg_name, const char *rdt_name)
    {
        alg_type = alg_name;
        rdt_type = rdt_name;
    }

    static inline void print_settings()
    {
        cout << "exp subject: [" << alg_type << "-" << rdt_type << "]\n";
        cout << "exp on ";
        if (type != exp_type::pattern)
        {
            switch (type)
            {
                case exp_type::speed:
                    cout << "speed: " << op_per_sec << "op/s";
                    break;
                case exp_type::replica:
                    cout << "replica: " << total_clusters << "x" << server_per_cluster;
                    break;
                case exp_type::delay:
                    cout << "delay: (" << delay << "ms," << delay_low << "ms)";
                    break;
                case exp_type::pattern:
                    break;
            }
            cout << ", round " << round_num;
        }
        else
            cout << "pattern: " << pattern_name;
        cout << ", total ops " << total_ops << endl;
    }

    static inline void set_speed(int round, int speed)
    {
        apply_default();
        op_per_sec = speed;
        total_ops =
            total_sec
            * (default_p->speed_e.start + default_p->speed_e.step * default_p->speed_e.times() / 10)
            / default_p->speed_e.times();
        round_num = round;
        type = exp_type::speed;
    }

    static inline void set_replica(int round, int cluster, int serverPCluster)
    {
        apply_default();
        total_clusters = cluster;
        server_per_cluster = serverPCluster;
        total_ops = total_sec * op_per_sec / default_p->replica_e.times();
        round_num = round;
        type = exp_type::replica;
    }

    static inline void set_delay(int round, int hd, int ld)
    {
        apply_default();
        delay = hd;
        delay_low = ld;
        total_ops = total_sec * op_per_sec / default_p->delay_e.times();
        round_num = round;
        type = exp_type::delay;
    }

    static inline void set_pattern(const string &name)
    {
        apply_default();
        total_ops = total_sec * op_per_sec / 5;
        pattern_name = name;
        type = exp_type::pattern;
    }
};

#endif  // BENCH_EXP_SETTING_H
