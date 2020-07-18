//
// Created by admin on 2020/1/13.
//

#ifndef BENCH_LIST_BASICS_H
#define BENCH_LIST_BASICS_H

// TODO conflicts?
struct list_op_gen_pattern
{
    double PR_ADD;
    double PR_UPD;
    double PR_REM;
};

// TODO pattern?
extern list_op_gen_pattern list_pt_dft;

constexpr int MAX_FONT_SIZE = 100;
constexpr int TOTAL_FONT_TYPE = 10;
constexpr int MAX_COLOR = 1u << 25u;

constexpr int BOLD = 1u << 0u;
constexpr int ITALIC = 1u << 1u;
constexpr int UNDERLINE = 1u << 2u;

#define LIST_TYPE_CODEC(ACTION) \
    ACTION(r)                   \
    ACTION(rwf)

#define DEFINE_ACTION(_name) _name,
enum class list_type
{
    LIST_TYPE_CODEC(DEFINE_ACTION)
};
#undef DEFINE_ACTION

extern const char *list_type_str[];

#endif  // BENCH_LIST_BASICS_H
