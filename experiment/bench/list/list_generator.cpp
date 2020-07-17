//
// Created by admin on 2020/7/16.
//

#include "list_generator.h"

#define DEFINE_ACTION(_name) #_name,
const char *list_type_str[] = {LIST_TYPE_CODEC(DEFINE_ACTION)};
#undef DEFINE_ACTION
