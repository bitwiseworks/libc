/* Check that obj is visible.
 * The virtual function table of the base class used to cause trouble.
 */

class ParentClass
{
public:
    virtual int get()
    {
        return 1;
    }
};

class ChildClass : public ParentClass
{
public:
    ChildClass()
    {
    }
};

int main(void)
{
    ChildClass obj;
    return 0;
}
