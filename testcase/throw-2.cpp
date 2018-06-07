// exceptions: multiple catch blocks
#include <iostream>
using namespace std;

int main ()
{
    cout << "Hello! This has to throw an exception." << endl;
    try
    {
        char * mystring;
        mystring = new char [10];
        if (mystring == NULL)
            throw "Allocation failure";
        for (int n = 0; n <= 100; n++)
        {
            if (n > 9)
                throw n;
            mystring[n] = 'z';
        }
    }
    catch (int i)
    {
        cout << "Exception: ";
        cout << "index " << i << " is out of range" << endl;
    }
    catch (const char * str)
    {
        cout << "Exception: " << str << endl;
    }
    return 0;
}

