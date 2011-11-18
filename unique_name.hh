#ifndef UNIQUE_NAME_HH
#define UNIQUE_NAME_HH

#include "concat.hh"

#define UNIQUE_NAME(prefix) CONCAT(prefix, __COUNTER__)

#endif
