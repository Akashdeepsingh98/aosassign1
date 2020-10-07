
#include "myutils.h"
using namespace std;

int main()
{
    char tmp[1024];
    getcwd(tmp, sizeof(tmp));
    root = tmp;
    root = getAbsPath(root);
    curDir = root;
    cout << curDir << endl;
    getchar();
    backdirstack.push(curDir);
    printf("\033[?1049h");
    fillDirList(curDir);
    printDirList();
    enNormMode();
}