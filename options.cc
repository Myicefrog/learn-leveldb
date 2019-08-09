#include "options.h"
#include "env.h"
#include "comparator.h"

namespace leveldb {

Options::Options() : comparator(BytewiseComparator()), env(Env::Default()) {}

}
