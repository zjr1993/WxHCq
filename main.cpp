#include <iostream>
#include <fcntl.h>
#include <io.h>

int main() {

    _setmode(_fileno(stdout), _O_U16TEXT);
    std::wcout << L"三从一纵"<< std::endl;

    return 0;
}