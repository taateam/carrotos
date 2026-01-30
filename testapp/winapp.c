#include <windows.h>

int main(void)
{
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;

    WriteConsoleA(
        h,
        "HELLO FROM CONSOLE TEST\r\n",
        25,
        &written,
        NULL
    );

    return 0;
}
