class nsISupportsWeakReference
{
public:
    virtual unsigned int GetWeakReference(void **_retval) = 0;
};

class __declspec(dllexport) nsSupportsWeakReference : public nsISupportsWeakReference
{
public:
    nsSupportsWeakReference() { }
    virtual unsigned int GetWeakReference(void **_retval);
};

unsigned int nsSupportsWeakReference::GetWeakReference( void** aInstancePtr )
{
    return 0;
}

class nsWeakReference
{
private:
#ifdef BREAK_IT
    friend class nsSupportsWeakReference;
#endif
};
