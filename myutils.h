#include <iostream>
#include <termios.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <ctime>
#include <fstream>
#include <sys/stat.h>
#include <stack>
#include <unistd.h>
#include <grp.h>
#include <vector>
#include <string>
#include <sys/ioctl.h>
#include <string>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
using namespace std;

int crow = 0, ccol = 0;
int windowpos = 0;
vector<string> dirlist;
struct winsize terminal;
stack<string> backdirstack, frontdirstack;
string commandStr = "";
int numTermRows = 0, numTermCols = 0;
string root = "", curDir = "";
struct termios commconfig, normalconfig;
vector<string> commandVector;
void pressUpKey();
void pressDownKey();
void pressLeftKey();
void pressRightKey();
void pressEnterKey();
void pressHomeKey();
bool isFile(string path);
void pressBackSpaceKey();
void moveCursor();
void copyMany();
void moveMany();
void moveFile(string from, string to);
void printDirList();
void executeCommand();
bool isDirectory(string path);
void deleteFiles();
void copyDirectory(string from, string to);
void printDirInLine(string s);
string getAbsPath(string path);
void createDirs();
void deleteDirectory(string dir);
void moveDirectory(string from, string to);
void createFiles();
void comStrToComVec();
void copyFile(string from, string to);
void gotof();
string getParDir(string path);
void search();
void searchDir();
void enComMode();
void enNormMode();
void pressK();
void givemsg();
void setCurToCmd();
void pressL();
void fillDirList(string path);
void renamef();
void search();

void enComMode()
{
    while (true)
    {
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("\033[0K");
        printf("cmd>");
        fflush(0);
        ccol += 4;
        char ch[3] = {0};
        commandStr.clear();
        commandVector.clear();
        while (true)
        {
            if (read(STDIN_FILENO, ch, 3) == 0)
            {
                continue;
            }
            if (ch[0] == 033 && ch[1] == 0 && ch[2] == 0)
            {
                crow = ccol = 1;
                moveCursor();
                return;
            }
            else if (ch[0] == 033 && ch[1] == '[' && (ch[2] == 'A' || ch[2] == 'B' || ch[2] == 'C' || ch[2] == 'D'))
            {
                continue;
            }
            else if (ch[0] == 10)
            {
                commandStr.push_back('\n');
                comStrToComVec();
                executeCommand();
                crow = terminal.ws_row - 1;
                ccol = 1;
                moveCursor();
                printf("\033[0K");
                printf("cmd>");
                ccol += 4;
                commandStr.clear();
            }
            else if (ch[0] == 127)
            {
                if (ccol > 5)
                {
                    ccol--;
                    moveCursor();
                    printf("\033[0K");
                    commandStr.pop_back();
                }
            }
            else
            {
                printf("%c", ch[0]);
                ccol++;
                moveCursor();
                commandStr.push_back(ch[0]);
            }
            fflush(0);
            for (int i = 0; i < 3; i++)
                ch[i] = 0;
        }
    }
}

void printDirList()
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal);
    numTermRows = terminal.ws_row - 2;
    numTermCols = terminal.ws_col;
    printf("\033[2J");
    ccol = 1;
    moveCursor();
    int top, bottom;
    if (dirlist.size() <= numTermRows)
    {
        top = 0;
        bottom = dirlist.size() - 1;
    }
    else
    {
        top = windowpos;
        bottom = numTermRows + windowpos;
    }
    for (int i = top; i < bottom + 1; i++)
    {
        printDirInLine(dirlist[i]);
    }
}

void copyFile(string from, string to)
{
    ifstream fs;
    ofstream fd;
    fs.open(from);
    if (!fs)
    {
        perror("Error ");
        return;
    }
    fd.open(to);
    if (!fd)
    {
        perror("Error ");
        return;
    }
    char buffer;
    while (fs.eof() == 0)
    {
        fs >> buffer;
        fd << buffer;
    }
    fs.close();
    fd.close();
}

void deleteFiles()
{
    if (commandVector.size() != 2)
    {
        givemsg();
        printf("Give proper arguments delete file");
        setCurToCmd();
        return;
    }
    for (int i = 1; i < commandVector.size(); i++)
    {
        string filepath = getAbsPath(commandVector[i]);
        if (isFile(filepath))
        {
            if (remove(filepath.c_str()) != 0)
            {
                givemsg();
                printf("Problem removing file");
                setCurToCmd();
                return;
            }
        }
        else if (isDirectory(filepath))
        {
            deleteDirectory(filepath);
        }
        else
        {
            givemsg();
            printf("Check arguments delete file");
            setCurToCmd();
            return;
        }
    }
    fillDirList(curDir);
    printDirList();
}

void deleteDirectory(string dir)
{
    string dirabspath = getAbsPath(dir);
    if (!isDirectory(dirabspath))
    {
        printf("Wrong path");
        return;
    }
    DIR *dd;
    dd = opendir(dirabspath.c_str());
    if (dd == NULL)
    {
        printf("cannot open directory");
        return;
    }

    struct dirent *direntry;

    while (direntry = readdir(dd))
    {
        string entryname = direntry->d_name;
        if (entryname == "." || entryname == "..")
        {
            continue;
        }
        else
        {
            string fpath = dirabspath + "/" + entryname;
            if (isFile(fpath))
            {
                if (remove(fpath.c_str()) != 0)
                {
                    printf("Problem with removing file");
                    return;
                }
            }
            else if (isDirectory(fpath))
            {
                deleteDirectory(fpath);
            }
            else
            {
                printf("Some problem");
                return;
            }
        }
    }
    rmdir(dirabspath.c_str());
    closedir(dd);
}

void executeCommand()
{
    string command = commandVector[0];
    if (command == "copy")
    {
        copyMany();
    }
    else if (command == "rename")
    {
        renamef();
    }
    else if (command == "move")
    {
        moveMany();
    }
    else if (command == "create_file")
    {
        createFiles();
    }
    else if (command == "create_dir")
    {
        createDirs();
    }
    else if (command == "delete_dir" || command == "delete_file")
    {
        deleteFiles();
    }
    else if (command == "search")
    {
        search();
        getchar();
        givemsg();
        setCurToCmd();
    }
    else if (command == "goto")
    {
        gotof();
    }
    else
    {
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("\033[0K");
        printf("command not found\n");
    }
}

void gotof()
{
    if (commandVector.size() != 2)
    {
        givemsg();
        printf("Give correct arguments");
        setCurToCmd();
        return;
    }
    string destpath = getAbsPath(commandVector[1]);
    if (!isDirectory(destpath))
    {
        givemsg();
        printf("Give directory");
        setCurToCmd();
        return;
    }
    curDir = destpath;
    windowpos = 0;
    fillDirList(curDir);
    printDirList();
    backdirstack.push(curDir);
    crow = terminal.ws_row - 1;
    ccol = 1;
    moveCursor();
    printf("\033[0K");
    printf("cmd>");
    ccol += 4;
}

void moveCursor()
{
    printf("\033[%d;%dH", crow, ccol);
}

string getAbsPath(string path)
{
    if (*(path.end() - 1) == '/')
    {
        path = path.substr(0, path.size() - 1);
    }
    if (path == "~")
        return root;
    else if (path[0] == '~')
        return root + path.substr(1);
    else if (path[0] == '/')
        return path;
    else if (path == ".")
        return curDir;
    else if (path.substr(0, 2) == "./")
        return curDir + path.substr(1);
    else
        return curDir + "/" + path;
}

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

void fillDirList(string path)
{
    string abspath = getAbsPath(path);
    DIR *dirptr;
    dirptr = opendir(abspath.c_str());
    if (!dirptr)
    {
        perror("cannot open dir");
        return;
    }
    dirlist.clear();

    struct dirent *direntry;
    while (direntry = readdir(dirptr))
    {
        dirlist.push_back(direntry->d_name);
    }
    if (abspath == root)
    {
        for (int i = 0; i < dirlist.size(); i++)
        {
            if (dirlist[i] == "..")
            {
                dirlist.erase(dirlist.begin() + i);
                break;
            }
        }
    }
    crow = 1;
    ccol = 1;
    moveCursor();
    closedir(dirptr);
}

void copyDirectory(string from, string to)
{
    string srcdirpath = getAbsPath(from);
    string destdirpath = getAbsPath(to);
    if (!isDirectory(srcdirpath))
    {
        givemsg();
        printf("copy dir Source path does not exist");
        setCurToCmd();
        return;
    }
    DIR *dd;
    dd = opendir(srcdirpath.c_str());
    if (!dd)
    {
        givemsg();
        printf("copy directory open directory problem");
        setCurToCmd();
        return;
    }

    struct dirent *direntry;
    while (direntry = readdir(dd))
    {
        string entryname = string(direntry->d_name);
        if (entryname == "." || entryname == "..")
        {
            continue;
        }
        else
        {
            string srcpath = srcdirpath + "/" + entryname;
            string destpath = destdirpath + "/" + entryname;
            if (isFile(srcpath))
            {
                copyFile(srcpath, destpath);
            }
            else if (isDirectory(srcpath))
            {
                if (mkdir(destpath.c_str(), 0777) == 0)
                {
                    copyDirectory(srcpath, destpath);
                }
                else
                {
                    perror("");
                    return;
                }
            }
            else
            {
                givemsg();
                printf("copy dir problem with path");
                setCurToCmd();
                return;
            }
        }
    }
    closedir(dd);
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
        //moveCursor();
    }
}

void comStrToComVec()
{
    commandVector.clear();
    string token = "";
    for (int i = 0; i < commandStr.size(); i++)
    {
        if (commandStr[i] == ' ' || commandStr[i] == '\n')
        {
            if (token.size() > 0)
            {
                commandVector.push_back(token);
            }
            token = "";
        }
        else
        {
            token += commandStr[i];
        }
    }
}

bool isFile(string path)
{
    string abspath = getAbsPath(path);
    ifstream f;
    f.open(abspath);
    if (!f)
        return false;
    f.close();
    return true;
}

void moveDirectory(string from, string to)
{
    string srcdirpath = getAbsPath(from);
    string destdirpath = getAbsPath(to);
    if (!isDirectory(srcdirpath))
    {
        givemsg();
        printf("Source path does not exist move dir");
        setCurToCmd();
        return;
    }
    DIR *dd;
    dd = opendir(srcdirpath.c_str());
    if (!dd)
    {
        givemsg();
        printf("open directory move dir");
        setCurToCmd();
        return;
    }

    struct dirent *direntry;
    while (direntry = readdir(dd))
    {
        string entryname = string(direntry->d_name);
        if (entryname == "." || entryname == "..")
        {
            continue;
        }
        else
        {
            string srcpath = srcdirpath + "/" + entryname;
            string destpath = destdirpath + "/" + entryname;
            if (isFile(srcpath))
            {
                moveFile(srcpath, destpath);
            }
            else if (isDirectory(srcpath))
            {
                if (mkdir(destpath.c_str(), 0777) == 0)
                {
                    moveDirectory(srcpath, destpath);
                }
                else
                {
                    givemsg();
                    printf("Cannot create directory move dir");
                    setCurToCmd();
                    return;
                }
            }
            else
            {
                givemsg();
                printf("problem with path move dir");
                setCurToCmd();
                return;
            }
        }
    }
    closedir(dd);
    rmdir(from.c_str());
}

void createDirs()
{
    if (commandVector.size() < 3)
    {
        givemsg();
        printf("Give proper arguments create dir");
        setCurToCmd();
        return;
    }
    string destdir = getAbsPath(commandVector.back());
    if (!isDirectory(destdir))
    {
        givemsg();
        printf("Not dest dir in create dir");
        setCurToCmd();
        return;
    }
    for (int i = 1; i < commandVector.size() - 1; i++)
    {
        string destpath = destdir + "/" + commandVector[i];
        if (mkdir(destpath.c_str(), 0777) != 0)
        {
            givemsg();
            printf("cannot create dir");
            setCurToCmd();
            return;
        }
    }
    fillDirList(curDir);
    printDirList();
}

void moveMany()
{
    if (commandVector.size() < 3)
    {
        givemsg();
        printf("give proper arguments move command");
        setCurToCmd();
    }
    else
    {
        string destfolder = getAbsPath(commandVector.back());
        if (!isDirectory(destfolder))
        {
            givemsg();
            printf("Destination is not a folder move comm");
            setCurToCmd();
            return;
        }
        for (int i = 1; i < commandVector.size() - 1; i++)
        {
            string sourcepath = getAbsPath(commandVector[i]);
            int fnstart = sourcepath.find_last_of("/");
            string destpath = destfolder + sourcepath.substr(fnstart);
            if (isFile(sourcepath))
            {
                moveFile(sourcepath, destpath);
            }
            else if (isDirectory(sourcepath))
            {
                //to do
                if (mkdir(destpath.c_str(), 0777) == 0)
                    moveDirectory(sourcepath, destpath);
            }
            else
            {
                givemsg();
                printf("Problem in given path");
                setCurToCmd();
            }
        }
    }
    fillDirList(curDir);
    printDirList();
}

bool isDirectory(string path)
{
    struct stat statbuffer;
    if (stat(path.c_str(), &statbuffer) != 0)
    {
        givemsg();
        printf("Cannot check if directory");
        setCurToCmd();
        return false;
    }
    if (S_ISDIR(statbuffer.st_mode))
        return true;
    return false;
}

void printDirInLine(string s)
{
    ccol = 0;
    struct stat statbuffer;
    string abspath = getAbsPath(s);
    stat(abspath.c_str(), &statbuffer);
    switch (statbuffer.st_mode & S_IFMT)
    {
    case S_IFBLK:
        printf("b");
        break;
    case S_IFCHR:
        printf("c");
        break;
    case S_IFDIR:
        printf("d");
        break;
    case S_IFIFO:
        printf("p");
        break;
    case S_IFLNK:
        printf("l");
        break;
    case S_IFREG:
        printf("-");
        break;
    case S_IFSOCK:
        printf("s");
        break;
    default:
        printf("-");
        break;
    }
    printf((statbuffer.st_mode & S_IRUSR) ? "r" : "-");
    printf((statbuffer.st_mode & S_IWUSR) ? "w" : "-");
    printf((statbuffer.st_mode & S_IXUSR) ? "x" : "-");
    printf((statbuffer.st_mode & S_IRGRP) ? "r" : "-");
    printf((statbuffer.st_mode & S_IWGRP) ? "w" : "-");
    printf((statbuffer.st_mode & S_IXGRP) ? "x" : "-");
    printf((statbuffer.st_mode & S_IROTH) ? "r" : "-");
    printf((statbuffer.st_mode & S_IWOTH) ? "w" : "-");
    printf((statbuffer.st_mode & S_IXOTH) ? "x" : "-");
    ccol += 10;

    struct passwd *username;
    username = getpwuid(statbuffer.st_uid);
    printf(" %12s ", username->pw_name);
    ccol += 14;

    struct group *groupname;
    groupname = getgrgid(statbuffer.st_uid);
    printf(" %12s ", groupname->gr_name);
    ccol += 14;

    unsigned long long int size = statbuffer.st_size;
    printf(" %15llu ", size);
    ccol += 17;

    string changetime = string(ctime(&statbuffer.st_mtime)).substr(4, 12);
    printf(" %10s ", changetime.c_str());
    ccol += 12;

    printf(" %-20s ", s.c_str());
    printf("\n");
}

void pressRightKey()
{
    ccol = crow = 1;
    moveCursor();
    if (frontdirstack.size() > 0)
    {
        string cur = frontdirstack.top();
        frontdirstack.pop();
        curDir = cur;
        backdirstack.push(cur);
        fillDirList(curDir);
        printDirList();
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("Status: Normal Mode");
        crow = 1;
        moveCursor();
    }
}

void createFiles()
{
    if (commandVector.size() < 3)
    {
        givemsg();
        printf("Give proper arguments create files");
        setCurToCmd();
    }
    else
    {
        string destdir = getAbsPath(commandVector.back());
        if (!isDirectory(destdir))
        {
            givemsg();
            printf("Give proper dest directory");
            setCurToCmd();
            return;
        }
        for (int i = 1; i < commandVector.size() - 1; i++)
        {
            string destpath = destdir + "/" + commandVector[i];
            fstream file;
            file.open(destpath, ios::out);
            if (!file)
            {
                givemsg();
                printf("problem creating file");
                setCurToCmd();
                return;
            }
        }
    }
    fillDirList(curDir);
    printDirList();
}

void pressLeftKey()
{
    ccol = crow = 1;
    moveCursor();
    if (backdirstack.size() > 1)
    {
        string cur = backdirstack.top();
        backdirstack.pop();
        frontdirstack.push(cur);
        cur = backdirstack.top();
        curDir = cur;
        fillDirList(curDir);
        printDirList();
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("Status: Normal Mode");
        crow = 1;
        moveCursor();
    }
}

void givemsg()
{
    crow = terminal.ws_row;
    ccol = 1;
    moveCursor();
    printf("\033[0K");
}

void moveFile(string from, string to)
{
    string srcpath = getAbsPath(from);
    string destpath = getAbsPath(to);
    if (!isFile(srcpath))
    {
        givemsg();
        printf("move file problem at source");
        setCurToCmd();
        return;
    }
    if (rename(srcpath.c_str(), destpath.c_str()) != 0)
    {
        givemsg();
        printf("move file problem moving file");
        setCurToCmd();
    }
}

void copyMany()
{
    if (commandVector.size() < 3)
    {
        givemsg();
        printf("give proper arguments\n");
        setCurToCmd();
    }
    else
    {
        string destfolder = getAbsPath(commandVector.back());
        if (!isDirectory(destfolder))
        {
            givemsg();
            printf("Destination is not a folder");
            setCurToCmd();
            return;
        }
        for (int i = 1; i < commandVector.size() - 1; i++)
        {
            string sourcepath = getAbsPath(commandVector[i]);
            int fnstart = sourcepath.find_last_of("/");
            string destpath = destfolder + sourcepath.substr(fnstart);
            if (isFile(sourcepath))
            {
                copyFile(sourcepath, destpath);
            }
            else if (isDirectory(sourcepath))
            {
                if (mkdir(destpath.c_str(), 0777) != 0)
                {
                    givemsg();
                    printf("could not copy directory %d", i);
                    setCurToCmd();
                    return;
                }
                else
                {
                    copyDirectory(sourcepath, destpath);
                }
            }
            else
            {
                givemsg();
                printf("problem in source path %d", i);
                setCurToCmd();
                return;
            }
        }
    }
}

void pressHomeKey()
{
    crow = ccol = 1;
    moveCursor();
    if (curDir != root)
        backdirstack.push(curDir);
    while (!frontdirstack.empty())
        frontdirstack.pop();
    curDir = root;
    backdirstack.push(curDir);
    fillDirList(curDir);
    printDirList();
    crow = terminal.ws_row - 1;
    ccol = 1;
    moveCursor();
    printf("Status: Normal Mode");
    crow = 1;
    moveCursor();
}

void pressBackSpaceKey()
{
    crow = ccol = 1;
    moveCursor();
    if (curDir != root)
    {
        string pardir = getParDir(curDir);
        while (frontdirstack.size() > 0)
        {
            frontdirstack.pop();
        }
        fillDirList(pardir);
        printDirList();
        curDir = pardir;
        backdirstack.push(curDir);
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("Status: Normal Mode");
        crow = 1;
        moveCursor();
    }
}

string getParDir(string path)
{
    string abspath = getAbsPath(path);
    int lastslash = abspath.find_last_of("/");
    string pardir = abspath.substr(0, lastslash);
    return pardir;
}

void pressEnterKey()
{
    if (dirlist[crow + windowpos - 1] == ".")
    {
        getchar();
        fillDirList(curDir);
        printDirList();
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("Status: Normal Mode");
        crow = 1;
        moveCursor();
    }
    else if (dirlist[crow + windowpos - 1] == "..")
    {
        string parent = getParDir(curDir);
        curDir = parent;
        backdirstack.push(curDir);
        while (frontdirstack.size() > 0)
        {
            frontdirstack.pop();
        }
        fillDirList(curDir);
        printDirList();
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("Status: Normal Mode");
        crow = 1;
        moveCursor();
    }
    else
    {
        string abspath = getAbsPath(curDir + "/" + dirlist[crow + windowpos - 1]);
        if (isDirectory(abspath))
        {
            while (frontdirstack.size() > 0)
            {
                frontdirstack.pop();
            }
            curDir = abspath;
            backdirstack.push(curDir);
            fillDirList(curDir);
            printDirList();
            crow = terminal.ws_row - 1;
            ccol = 1;
            moveCursor();
            printf("Status: Normal Mode");
            crow = 1;
            moveCursor();
        }
        else if (isFile(abspath))
        {
            int childpid = fork();

            if (childpid)
            {
                int retstat;
                waitpid(childpid, &retstat, 0);
            }
            else
            {

                char *const argv[5] = {"xterm", "-e", "vim", (char *)abspath.c_str(), NULL};
                execvp("xterm", argv);
            }
        }
        else
        {
            printf("Problem with selected file");
            return;
        }
    }
    ccol = crow = 1;
    moveCursor();
}

void enNormMode()
{
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal);
    numTermRows = terminal.ws_row - 2;
    numTermCols = terminal.ws_col;
    tcgetattr(STDIN_FILENO, &commconfig);
    normalconfig = commconfig;
    normalconfig.c_lflag &= ~(ICANON | ECHO | IEXTEN | ISIG);
    normalconfig.c_iflag &= ~(BRKINT);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &normalconfig) != 0)
    {
        printf("Problem with attribute set normal mode\n");
        return;
    }

    windowpos = 0;

    crow = terminal.ws_row - 1;
    ccol = 1;
    moveCursor();
    printf("Status: Normal Mode");
    crow = 1;
    moveCursor();
    char ch[3] = {0};
    while (true)
    {
        fflush(0);
        if (read(STDIN_FILENO, ch, 3) == 0)
        {
            continue;
        }
        else if (ch[0] == 033 && ch[1] == '[' && ch[2] == 'A')
        {
            pressUpKey();
        }
        else if (ch[0] == 033 && ch[1] == '[' && ch[2] == 'B')
        {
            pressDownKey();
        }
        else if (ch[0] == 033 && ch[1] == '[' && ch[2] == 'C')
        {
            pressRightKey();
        }
        else if (ch[0] == 033 && ch[1] == '[' && ch[2] == 'D')
        {
            pressLeftKey();
        }
        else if (ch[0] == 'h' || ch[0] == 'H')
        {
            pressHomeKey();
        }
        else if (ch[0] == 127)
        {
            pressBackSpaceKey();
        }
        else if (ch[0] == 10)
        {
            pressEnterKey();
        }
        else if (ch[0] == ':')
        {
            enComMode();
        }
        else if (ch[0] == 'q')
        {
            printf("\033[2J");
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &commconfig);
            crow = ccol = 1;
            moveCursor();
            exit(0);
        }
        fflush(0);
        for (int i = 0; i < 3; i++)
            ch[i] = 0;
    }
}

void pressK()
{
    if (windowpos > 0)
    {
        windowpos--;
        printDirList();
    }
}

void pressL()
{
    numTermRows = terminal.ws_row - 2;
    int extrafiles = dirlist.size() - numTermRows;
    if (windowpos < extrafiles)
    {
        windowpos++;
        printDirList();
    }
}

void searchDir(string path, string filename, bool &found)
{
    DIR *dirptr;
    dirptr = opendir(path.c_str());
    if (!dirptr)
    {
        givemsg();
        printf("cannot open directory");
        setCurToCmd();
        getchar();
        return;
    }
    struct dirent *direntry;
    while (direntry = readdir(dirptr))
    {
        string fn = path + "/" + string(direntry->d_name);
        if (string(direntry->d_name)[0] == '.' || string(direntry->d_name) == "..")
        {
            continue;
        }
        if(string(direntry->d_name) == filename)
        {
            found=true;
            closedir(dirptr);
            return;
        }
        if(isDirectory(fn))
        {
            searchDir(fn, filename, found);
        }
    }
    closedir(dirptr);
}

void search()
{
    if (commandVector.size() != 2)
    {
        givemsg();
        printf("Search arguments incorrect");
        setCurToCmd();
        return;
    }
    bool found = false;
    searchDir(curDir, commandVector.back(), found);
    getchar();
    if (found)
    {
        crow = terminal.ws_row;
        ccol = 1;
        moveCursor();
        printf("\033[0K");
        printf("True");
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("cmd>");
        ccol += 4;
    }
    else
    {
        crow = terminal.ws_row;
        ccol = 1;
        moveCursor();
        printf("\033[0K");
        printf("False");
        crow = terminal.ws_row - 1;
        ccol = 1;
        moveCursor();
        printf("cmd>");
        ccol += 4;
    }
}

void renamef()
{
    if (commandVector.size() != 3)
    {
        givemsg();
        printf("Give proper arguments to rename\n");
        setCurToCmd();
        return;
    }
    string oldname = getAbsPath(commandVector[1]);
    string pardir = getParDir(oldname);
    string newname = pardir + "/" + commandVector[2];
    if (rename(oldname.c_str(), newname.c_str()) != 0)
    {
        givemsg();
        printf("problem renaming");
        setCurToCmd();
        return;
    }
    fillDirList(curDir);
    printDirList();
    crow = terminal.ws_row - 1;
    ccol = 1;
    moveCursor();
    printf("cmd>");
    ccol += 4;
}

void setCurToCmd()
{
    crow = terminal.ws_row - 1;
    ccol = 1;
    printf("\033[0K");
    moveCursor();
    printf("cmd>");
    ccol += 4;
}
