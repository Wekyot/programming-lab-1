#include <stdio.h>
#include <locale.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "args_work.h"
#include "readtable.h"
#include "utils.h"

int main(int argc, char* argv[]){
    setlocale(LC_ALL, ".UTF-8");

#ifdef _WIN32    
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    parse_args(argc, argv);

    return 0;
}