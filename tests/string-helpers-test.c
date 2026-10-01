#include <assert.h>

#include "string-helpers.h"

int main(void)
{
	assert(matchesFilter("Half-Life 2", "") == 1);
	assert(matchesFilter("Half-Life 2", "half") == 1);
	assert(matchesFilter("Half-Life 2", "LIFE 2") == 1);
	assert(matchesFilter("Half-Life 2", "Portal") == 0);
	assert(matchesFilter("", "game") == 0);

	return 0;
}
