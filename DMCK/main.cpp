#include "exp_runner.h"
#include "op_script.h"
#include "oracle.h"

int main()
{
    std::istream::sync_with_stdio(false);
    std::ostream::sync_with_stdio(false);

    exp_env env{3};

    auto& c = env.get();

    c[0].exec("rwfzrem s a 1");
    c[0].exec("rwfzadd s a 1");
    c[1].exec("rwfzadd s a 2");

    auto r = c[0].exec("rwfzscore s a");
    print_reply(r);
    r = c[1].exec("rwfzscore s a");
    print_reply(r);

    r = c[1].get_inner_msg();
    c[0].pass_inner_msg(r);
    r = c[0].exec("rwfzscore s a");
    print_reply(r);

    r = c[2].exec("rwfzscore s a");
    print_reply(r);
    r = c[0].get_inner_msg();
    print_reply(r);
    c[2].pass_inner_msg(r);
    r = c[2].exec("rwfzscore s a");
    print_reply(r);

    // run<rpq_op_script, rpq_oracle>("TLA/rwf_rpq/rwf_rpq.script");

    return 0;
}
