/* conio.h,v 1.2 2004/09/14 22:27:32 bird Exp */
/** @file
 * EMX
 */

#ifndef _CONIO_H
#define _CONIO_H

#define getch()   _read_kbd (0, 1, 0)
#define getche()  _read_kbd (1, 1, 0)

#endif /* not _CONIO_H */
