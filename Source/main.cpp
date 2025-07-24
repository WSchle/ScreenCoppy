#include <iostream>
#include <Windows.h>

#include "UI/ui.h"

int main()
{
	std::cout << "Hello, World!\n";
	return 0;
}

int  WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{
	UI->drawSelection();
	return 0;
}