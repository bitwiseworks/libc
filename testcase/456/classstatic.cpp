
class clazz
{
public:
    static char *pszPublicStatic;
};

char sz[120] = {0};
char *clazz::pszPublicStatic = "PublicStatic";

int main(void)
{
    clazz   obj;

    return obj.pszPublicStatic[0] == 'P' ? 0 : 1;
}
