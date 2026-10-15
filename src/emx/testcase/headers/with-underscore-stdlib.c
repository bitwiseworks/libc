/*
 * stdlib.h must provide the types needed by its underscore declarations when
 * _WITH_UNDERSCORE is enabled in strict C mode. Include no other header first.
 */
#define _WITH_UNDERSCORE 1

#include <stdlib.h>

int main(void)
{
    return 0;
}
