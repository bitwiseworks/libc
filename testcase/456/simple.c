/* step this code and check that everything is visible and step ordre is ok. */

/* Check that both these are viewable in the 'program monitor'. */
static int      fileglobal = 2;
int             realglobal = 1;

/* Enum test: check that these are ok and the value is stringified */
enum ENM { enmA, enmB, enmC};
static enum ENM enmfileglobal = enmA;
enum ENM        enmrealglobal = enmB;

/* Enum test: check that these are ok and the value is stringified */
enum { enmAnon, enmBnon, enmCnon}
                enmAnonymous;


int foo(int i, int j)
{
    static int myVar = 0;
    static int myVar2;

    if (myVar++)
    {
        int scope;

        return scope + myVar + myVar2;
    }
    myVar2++;
    return 0;
}

static int bar(int i, int j)
{
    return i+j;
}

void stub(void)
{
}

int main(int argc, char **argv)
{
    int i;
    i = foo(1, 2);
    i += bar(1, 2);
    stub();
    return i;
}
