//
// Created by admin on 2020/1/13.
//

#include "list_log.h"

constexpr int BOLD = 1u << 0u;
constexpr int ITALIC = 1u << 1u;
constexpr int UNDERLINE = 1u << 2u;

double list_log::diff(const list_log::element &e, const redisReply *r)
{
    if (e.content != r->element[1]->str) return 1;
    if (e.font != atoi(r->element[2]->str)) return 0.5; // NOLINT
    if (e.size != atoi(r->element[3]->str)) return 0.5; // NOLINT
    if (e.color != atoi(r->element[4]->str)) return 0.5; // NOLINT
    int p = atoi(r->element[5]->str); // NOLINT
    if (e.bold != (bool) (p & BOLD)) return 0.5; // NOLINT
    if (e.italic != (bool) (p & ITALIC)) return 0.5; // NOLINT
    if (e.underline != (bool) (p & UNDERLINE)) return 0.5; // NOLINT
    return 0;
}

void list_log::read_list(redisReply_ptr &r)
{
    int len;
    double distance;

    {
        lock_guard<mutex> lk(mtx);
        len = document.size();
        int r_len = r->elements;

        // Levenshtein distance
        vector<vector<double> > dp(len + 1, vector<double>(r_len + 1, 0));
        for (int i = 1; i <= len; i++) dp[i][0] = i;
        for (int j = 1; j <= r_len; j++) dp[0][j] = j;
        auto iter = document.begin();
        for (int i = 1; i <= len; i++)
        {
            for (int j = 1; j <= r_len; j++)
                dp[i][j] = min(dp[i - 1][j - 1] + diff(**iter, r->element[j - 1]),
                               min(dp[i][j - 1] + 1, dp[i - 1][j] + 1));
            iter++;
        }
        distance = dp[len][r_len];
    }

    {
        lock_guard<mutex> lk(dis_mtx);
        distance_log.emplace_back(len, distance);
    }
}

void list_log::overhead(int o)
{
    int num;
    {
        lock_guard<mutex> lk(mtx);
        num = document.size();
    }
    lock_guard<mutex> lk(ovhd_mtx);
    overhead_log.emplace_back(num, o);
}

