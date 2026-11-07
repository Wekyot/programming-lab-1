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
    setlocale(LC_NUMERIC, "C");

#ifdef _WIN32    
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    int reason = parse_args(argc, argv);

    switch (reason)
    {
    case FUNC_RETURN_SUCCESS:
        return 0;
    case FUNC_RETURN_RUNTIME_ERROR:
        return 1;
    case FUNC_RETURN_ARGS_ERROR:
        return 2;
    }
}