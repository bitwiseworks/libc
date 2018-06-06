#define NULL ((void*)0)

static struct {
    void *      pv;
    int         num;
} array[] = {
    {NULL,                          1},
    {NULL,                         -1},
    {NULL,                          1},
    {NULL,                         -1},
    {NULL,                          1},
    {NULL,                         -1},
    {NULL,                          1},
    {NULL,                         -1},
    {NULL,                          1},
    {NULL,                         -1},
    {NULL,                          1},
    {NULL,                         -1},
    {NULL,                          1},
    {NULL,                         -1},
};


int main()
{
    int rc = 0;
    int i;

    for (i = 0; i < sizeof(array) / sizeof(array[0]); i++)
        rc = array[i].num;

    return rc;
}
