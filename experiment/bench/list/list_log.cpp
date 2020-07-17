//
// Created by admin on 2020/1/13.
//

#include "list_log.h"

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
    list<element *> doc_read;
    {
        lock_guard<mutex> lk(mtx);
        for (auto &e_p : document)
            doc_read.emplace_back(e_p.get());
    }

    int len = doc_read.size();
    int r_len = r->elements;
    // Levenshtein distance
    vector<vector<double> > dp(len + 1, vector<double>(r_len + 1, 0));
    for (int i = 1; i <= len; i++) dp[i][0] = i;
    for (int j = 1; j <= r_len; j++) dp[0][j] = j;
    auto iter = doc_read.begin();
    for (int i = 1; i <= len; i++)
    {
        for (int j = 1; j <= r_len; j++)
            dp[i][j] = min(dp[i - 1][j - 1] + diff(**iter, r->element[j - 1]),
                           min(dp[i][j - 1] + 1, dp[i - 1][j] + 1));
        iter++;
    }
    double distance = dp[len][r_len];

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

void list_log::write_file()
{
    char f[64];

    sprintf(f, "%s/ovhd.csv", dir);
    FILE *ovhd = fopen(f, "w");
    for (auto &o:overhead_log)
        fprintf(ovhd, "%d,%d\n", get<0>(o), get<1>(o));
    fflush(ovhd);
    fclose(ovhd);

    sprintf(f, "%s/distance.csv", dir);
    FILE *distance = fopen(f, "w");
    for (auto &o:distance_log)
        fprintf(distance, "%d,%f\n", get<0>(o), get<1>(o));
    fflush(distance);
    fclose(distance);
}

void list_log::insert(string &prev, string &name, string &content,
                      int font, int size, int color, bool bold, bool italic, bool underline)
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.find(prev) == ele_map.end() || ele_map.find(name) != ele_map.end()) return;
    auto it_next = prev.empty() ? document.begin() : ele_map[prev];
    if (it_next != document.begin())it_next++;
    unique_ptr<element> e(new element(content, font, size, color, bold, italic, underline));
    document.insert(it_next, move(e));
    it_next--;
    ele_map[name] = it_next;
}

void list_log::update(string &name, const char *upd_type, int value)
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.find(name) != ele_map.end())
    {
        auto &e = *ele_map[name];
        if (strcmp(upd_type, "font") == 0)
            e->font = value;
        else if (strcmp(upd_type, "size") == 0)
            e->size = value;
        else if (strcmp(upd_type, "color") == 0)
            e->color = value;
        else if (strcmp(upd_type, "bold") == 0)
            e->bold = value;
        else if (strcmp(upd_type, "italic") == 0)
            e->italic = value;
        else if (strcmp(upd_type, "underline") == 0)
            e->underline = value;
    }
}

void list_log::remove(string &name)
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.find(name) != ele_map.end())
    {
        auto &e = ele_map[name];
        document.erase(e);
        ele_map.erase(name);
    }
}

string list_log::random_get()
{
    lock_guard<mutex> lk(mtx);
    if (ele_map.empty())return string();
    auto random_it = next(begin(ele_map), intRand(ele_map.size()));
    return random_it->first;
}
