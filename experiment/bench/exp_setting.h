//
// Created by admin on 2020/4/15.
//

#ifndef BENCH_EXP_SETTING_H
#define BENCH_EXP_SETTING_H

#include <cstdio>
#include <cassert>

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
    static enum class exp_type
    {
        EXP_TYPE_CODEC(DEFINE_ACTION)
    } type;
#undef DEFINE_ACTION

    static const char *type_str[];
    static const char *pattern_name;
    static int round_num;

    static inline void set_default(default_setting *p) { default_p = p; }

    static inline void print_settings()
    {
        printf("exp on ");
        if (type != exp_type::pattern)
        {
            switch (type)
            {
            case exp_type::speed:
                printf("speed: %dop/s", op_per_sec);
                break;
            case exp_type::replica:
                printf("replica: %dx%d", total_clusters, server_per_cluster);
                break;
            case exp_type::delay:
                printf("delay: (%dms,%dms)", delay, delay_low);
                break;
            case exp_type::pattern:
                break;
            }
            printf(", round %d", round_num);
        }
        else
            printf("pattern: %s", pattern_name);
        printf(", total ops %d\n", total_ops);
    }

    static inline void set_speed(int round, int speed)
    {
        apply_default();
        op_per_sec = speed;
        total_ops = total_sec / default_p->speed_e.times() *
                    (default_p->speed_e.start + default_p->speed_e.step * default_p->speed_e.times() / 10);
        round_num = round;
        type = exp_type::speed;
    }

    static inline void set_replica(int round, int cluster, int serverPCluster)
    {
        apply_default();
        total_clusters = cluster;
        server_per_cluster = serverPCluster;
        total_ops = total_sec / default_p->replica_e.times() * op_per_sec;
        round_num = round;
        type = exp_type::replica;
    }

    static inline void set_delay(int round, int hd, int ld)
    {
        apply_default();
        delay = hd;
        delay_low = ld;
        total_ops = total_sec / default_p->delay_e.times() * op_per_sec;
        round_num = round;
        type = exp_type::delay;
    }

    static inline void set_pattern(const char *name)
    {
        apply_default();
        total_ops = total_sec / 5 * op_per_sec;
        pattern_name = name;
        type = exp_type::pattern;
    }
};

#endif //BENCH_EXP_SETTING_H
