#include <math.h>
#include <stdio.h>
int main()
{
    double r = nan("");
    float rd = nanf("");
    long double lrd = nanl("");
    return r == r
        || rd == rd
        || lrd == lrd;
}
