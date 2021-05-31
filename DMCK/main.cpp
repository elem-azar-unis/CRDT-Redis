#include "exp_runner.h"

int main()
{
    std::istream::sync_with_stdio(false);
    std::ostream::sync_with_stdio(false);

    redis_connect c1{"127.0.0.1", 6379, 3, 0};
    redis_connect c2{"127.0.0.1", 6380, 3, 1};
    redis_connect c3{"127.0.0.1", 6381, 3, 2};

    c1.exec("rzadd s a 1");
    c2.exec("rzadd s a 2");

    auto r{c1.exec("rzscore s a")};
    print_reply(r);
    r = c2.exec("rzscore s a");
    print_reply(r);

    r = c2.get_inner_msg();
    c1.pass_inner_msg(r);
    r = c1.exec("rzscore s a");
    print_reply(r);

    r = c3.exec("rzscore s a");
    print_reply(r);
    r = c1.get_inner_msg();
    print_reply(r);
    c3.pass_inner_msg(r);
    r = c3.exec("rzscore s a");
    print_reply(r);

    return 0;
}
