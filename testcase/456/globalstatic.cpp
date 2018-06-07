class foo
{
public:
    virtual long GetJavaWrapper(void)
    {
        static const char *pszFunctionMethod = __FUNCTION__;
        return *pszFunctionMethod;
    }
};


int main()
{
    static const char *pszFunctionMain = __FUNCTION__;
    foo obj;
    int rc = obj.GetJavaWrapper() + *pszFunctionMain;
    return rc;
}


