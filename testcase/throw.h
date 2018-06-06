
#if 1
#define dfprintf(a) fprintf a
#else
#define dfprintf(a) do {} while(0)
#endif

class expt
{
private:
    int i;
    const char *psz;

public:
    expt(int i, const char *psz);
    int get() const;
};

class bar
{
    int     i;
public:
    bar(int i);
    bar() throw(int);
    int get() const;
    int getThrow() const throw(expt);
};

