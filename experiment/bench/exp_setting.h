//
// Created by admin on 2020/4/15.
//

#ifndef BENCH_EXP_SETTING_H
#define BENCH_EXP_SETTING_H

#include <cstdio>

class exp_setting
{
private:
    static inline void set_default()
    {
        delay = 50;
        delay_low = 10;
        total_clusters = 3;
        server_per_cluster = 3;
        total_ops = 27;
        op_per_sec = 10000;
    }

public:
    static int delay;
    static int delay_low;
    static int total_clusters;
    static int server_per_cluster;
    static int total_ops;
    static int op_per_sec;

/*
#define EXP_TYPE_CODEC(ACTION)      \
    ACTION(speed)                   \
    ACTION(replica)                 \
    ACTION(delay)                   \
    ACTION(pattern)

#define DEFINE_ACTION(_name) e##_name,
    enum exp_type { EXP_TYPE_CODEC(DEFINE_ACTION) };
#undef DEFINE_ACTION

#define DEFINE_ACTION(_name) #_name,
    static const char* type_str[] = { EXP_TYPE_CODEC(DEFINE_ACTION) };
#undef DEFINE_ACTION
*/

    static enum class exp_type
    {
        speed = 0, replica = 1, delay = 2, pattern
    } type;
    static const char *type_str[3];
    static const char *pattern_name;
    static int round_num;

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
            printf(", round %d\n", round_num);
        }
        else printf("pattern: %s\n", pattern_name);
    }

    static inline void set_speed(int round, int speed)
    {
        set_default();
        op_per_sec = speed;
        total_ops = 200000;
        round_num = round;
        type = exp_type::speed;
    }

    static inline void set_replica(int round, int cluster, int serverPCluster)
    {
        set_default();
        total_clusters = cluster;
        server_per_cluster = serverPCluster;
        total_ops = 2000;
        round_num = round;
        type = exp_type::replica;
    }

    static inline void set_delay(int round, int hd, int ld)
    {
        set_default();
        delay = hd;
        delay_low = ld;
        total_ops = 1000;
        round_num = round;
        type = exp_type::delay;
    }

    static inline void set_pattern(const char *name)
    {
        set_default();
        pattern_name = name;
        type = exp_type::pattern;
    }
};

#endif //BENCH_EXP_SETTING_H
