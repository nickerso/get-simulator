#include "common.hpp"

static int _debugLevel_ = 0;

void setDebugLevel(const int level)
{
    _debugLevel_ = level;
}

int debugLevel()
{
    return _debugLevel_;
}
