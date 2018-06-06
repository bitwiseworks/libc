extern int ext1(void);
extern int ext2(void);
extern int ext3(void);
extern int ext4(void);
extern int ext5(void);
extern int lib6(void);
extern int lib7(void);

int main(void)
{
    int i = ext1()
          + ext2()
          + ext3()
          + ext4()
          + ext5()
          + lib6()
          + lib7();
    return (i != 7);
}
