#include <stdlib.h>

int main()
{
    div_t lv = {0,0};
    lv = div(6,2);
    printf(" 6/2 -> q=%d r=%d\n", lv.quot, lv.rem);
    return 0;
}
