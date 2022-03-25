#include "exp_runner.h"

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // run<rpq_op_script, rpq_oracle>("TLA/rwf_rpq/rwf_rpq_2_4.script");
    // run<rpq_op_script, rpq_oracle>("TLA/rwf_rpq/rwf_rpq_3_3.script");

    std::ofstream out_2_4("out_2_4.script");
    run<list_op_script, list_oracle>("TLA/rwf_list/rwf_list_2_4.script", out_2_4);
    std::ofstream out_3_3("out_3_3.script");
    run<list_op_script, list_oracle>("TLA/rwf_list/rwf_list_3_3.script", out_3_3);

    // run<list_op_script, list_oracle>("test.script");

    return 0;
}
