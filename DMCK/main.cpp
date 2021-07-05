#include "exp_runner.h"

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // exp_env env{3};

    // auto& c = env.get();

    // c[0].exec("rwfzrem s a 1");
    // c[0].exec("rwfzadd s a 1");
    // c[1].exec("rwfzadd s a 2");

    // auto r = c[0].exec("rwfzscore s a");
    // r.print();
    // r = c[1].exec("rwfzscore s a");
    // r.print();

    // r = c[1].get_inner_msg();
    // c[0].pass_inner_msg(r);
    // r = c[0].exec("rwfzscore s a");
    // r.print();

    // r = c[2].exec("rwfzscore s a");
    // r.print();
    // r = c[0].get_inner_msg();
    // r.print();
    // c[2].pass_inner_msg(r);
    // r = c[2].exec("rwfzscore s a");
    // r.print();

    // run<rpq_op_script, rpq_oracle>("TLA/rwf_rpq/rwf_rpq_2_4.script");
    // run<rpq_op_script, rpq_oracle>("TLA/rwf_rpq/rwf_rpq_3_3.script");
    run<list_op_script, list_oracle>("test.script");

    return 0;
}
