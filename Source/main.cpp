#include <iostream>
#include <Windows.h>

#include "UI/ui.h"


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