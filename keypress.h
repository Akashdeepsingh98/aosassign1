#ifndef KEYPRESS_H
#define KEYPRESS_H
#endif

void pressUpKey();
void pressDownKey();
void pressLeftKey();
void pressRightKey();
void pressEnterKey();
void pressHomeKey();
void pressL();
void pressK();
void pressBackSpaceKey();

void pressUpKey()
{
    if (crow > 1)
    {
        crow--;
        moveCursor();
    }
    else if (crow + windowpos > 1)
    {
        windowpos--;
        printDirList();
        moveCursor();
    }
}

void pressDownKey()
{
    if (crow <= numTermRows && crow < dirlist.size())
    {
        crow++;
        moveCursor();
    }
    else if (crow > numTermRows && crow + windowpos < dirlist.size())
    {
        windowpos++;
        printDirList();
    }
}
