//
// Created by admin on 2020/1/8.
//

#include "util.h"

#include <cstring>

#include "errno.h"
#include "exp_env.h"
#include "exp_setting.h"

string exp_env::sudo_pwd;

const char *exp_setting::alg_type;
const char *exp_setting::rdt_type;
exp_setting::default_setting *exp_setting::default_p = nullptr;

int exp_setting::delay;
int exp_setting::delay_low;
int exp_setting::total_clusters;
int exp_setting::server_per_cluster;
int exp_setting::total_ops;
int exp_setting::op_per_sec;

bool exp_setting::compare = false;

exp_setting::exp_type exp_setting::type;
string exp_setting::pattern_name;
int exp_setting::round_num;

#define DEFINE_ACTION(_name) #_name,
const char *exp_setting::type_str[] = {EXP_TYPE_CODEC(DEFINE_ACTION)};
#undef DEFINE_ACTION

int intRand(int min, int max)
{
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_int_distribution<int> distribution(min, max);
    return distribution(*rand_gen);
}

string strRand(int max_len)
{
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    int len = intRand(1, max_len - 1);
    ostringstream stream;
    for (int i = 0; i < len; ++i)
        stream << charset[intRand(sizeof(charset))];
    return stream.str();
}

double doubleRand(double min, double max)
{
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_real_distribution<double> distribution(min, max);
    return distribution(*rand_gen);
}

void redis_client::add_pipeline_cmd(cmd *command)
{
    {
        lock_guard<mutex> lk(m);
        cmds.emplace_back(command);
    }
    cv.notify_all();
    if (!run)
    {
        run = true;
        pipeline = thread([this] {
            list<unique_ptr<cmd>> waiting;
            while (run)
            {
                {
                    unique_lock<mutex> lk(m);
                    cv.wait(lk, [this] { return !cmds.empty() || !run; });
                    while (!cmds.empty())
                    {
                        waiting.emplace_back(cmds.front());
                        cmds.pop_front();
                    }
                }
                for (auto &cw : waiting)
                    if (redisAppendCommand(c, cw->stream.str().c_str()) != REDIS_OK)
                    {
                        cout << "\nsomething wrong appending " << cw->stream.str() << " to host "
                             << c->tcp.host << ":" << c->tcp.port << endl;
                        exit(-1);
                    }
                while (!waiting.empty())
                {
                    auto reply = exec();
                    if (reply == nullptr) reply_error(waiting.front()->stream.str());
                    if (reply->type != REDIS_REPLY_ERROR)
                        waiting.front()->handle_redis_return(reply);
                    waiting.pop_front();
                }
            }
        });
    }
}

redisReply_ptr redis_client::exec(const cmd &cmd) { return exec(cmd.stream.str()); }

void redis_client::reply_error(const string &cmd)
{
    cout << "\nSomething wrong for host " << c->tcp.host << ":" << c->tcp.port << "to execute "
         << cmd << "\n";
    if (c->reader->err == REDIS_ERR_IO)
        cout << "IO error: " << strerror(errno) << endl;
    else
        cout << "errno: " << c->reader->err << ", err str: " << c->reader->errstr << endl;
    exit(-1);
}

redisReply_ptr redis_client::exec()
{
    void *r;
    redisGetReply(c, &r);
    return redisReply_ptr(static_cast<redisReply *>(r), freeReplyObject);
}
