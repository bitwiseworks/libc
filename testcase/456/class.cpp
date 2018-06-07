
class ParentClass
{
private:
    int     iPrivate;
    static char *pszPrivateStatic;

protected:
    int     iProtected;
    static char *pszProtectedStatic;

public:
    static char *pszPublicStatic;
    static long  lPublicStatic;
    int      iPublic;

    ParentClass()
    {
        iPublic = 1;
        iProtected = 2;
        iPrivate = 3;
    }

    virtual ~ParentClass()
    {
        iPublic = -1;
        iProtected = -2;
        iPrivate = -3;
    }

    virtual int get() const
    {
        return iProtected + iPrivate;
    }

    virtual void set(int i)
    {
        iPublic = i;
        iProtected = i + 1;
        iPrivate = i + 2;
    }

    int getPrivate(void)
    {
        return iPrivate;
    }
};
char *ParentClass::pszPrivateStatic = "PrivateStatic";
char *ParentClass::pszProtectedStatic = "ProtectedStatic";
char *ParentClass::pszPublicStatic = "PublicStatic";
long  ParentClass::lPublicStatic = 42;

class ChildClass : public ParentClass
{
public:
    ChildClass(int &ri) :
        ParentClass()
    {
        ri = getPrivate();
    }
};

int main(void)
{
    int i;
    ChildClass *pObj = new ChildClass(i);
    pObj->set(i+1);
    delete pObj;
    return 0;
}
