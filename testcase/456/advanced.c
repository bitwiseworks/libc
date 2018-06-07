/* Minimal testcase for union in struct.
 * In C++ mode the following result:
 *      Works in idebug (VAC365 fp2)
 *      Works not in icsdebug (VAC308)
 *      Works not in idbug (JIT)
 *
 * The pNext pointers used to be a problem too.
 */
union u
{
    char *  pch;
    int     i;
    union u *pNext;
};

struct s
{
    int tag;
    union u d;
    struct s * pNext;
};

int main(void)
{
    union  u  un;
    struct s  st;
    un.pch = "";
    un.i = 1;
    st.tag = 2;
    st.d = un;
    return 0;
}

