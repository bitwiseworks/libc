/* verify that one can step thru all the code and that the this and obj variables make sense. */
#define USE_INLINE
class foo
{
    char ch;
public:
#ifdef USE_INLINE
    foo()
    {
        ch = 'k';
        int ch2 = 1;
        ch += ch2;
        int ch3 = 1;
        ch += ch3;
        #if 1
        if (ch == 123)
        {
            int ch4 = 23;
            ch -= ch4;
        }
        #endif
    }

    virtual ~foo()
    {
        ch = 0;
    }

    virtual void set(int ch)
    {
        this->ch = ch;
    }

#else
    foo();
    virtual ~foo();
    virtual void set(int ch);

#endif
};

#ifndef USE_INLINE
foo::foo()
{
    ch = 'k';
    int ch2 = 1;
    ch += ch2;
    int ch3 = 1;
    ch += ch3;
    #if 0
    if (ch == 123)
    {
        int ch4 = 23;
        ch -= ch4;
    }
    #endif
}

foo::~foo()
{
    ch = 0;
}

void foo::set(int ch)
{
    this->ch = ch;
}
#endif


int main(int argc, char **argv)
{
    foo obj;
    obj.set('k');
    return 0;
}
