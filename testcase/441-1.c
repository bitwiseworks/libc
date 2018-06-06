extern int uninitialized;
extern int initialized;

int nonexternalized;

int foo1(void)
{
    return initialized + 1 + uninitialized + nonexternalized;
}

