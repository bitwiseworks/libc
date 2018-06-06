/* from 'samm' */
#include <locale.h>

int main(void)
{
    char *rc;
    printf("Setting locale to ru_ru.IBM-866\n");
    rc=setlocale(LC_CTYPE,"ru_ru.IBM-866");
    printf("return=%s\n",rc);
    printf("Setting locale to ab_cd\n");
    rc=setlocale(LC_CTYPE,"ab_cd"); /* must return error, not null! */
    printf("return=%s\n",rc);
    return 0;
}

