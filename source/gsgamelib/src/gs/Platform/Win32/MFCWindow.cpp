#include "MFCWindow.h"
#include <cassert>

void MFCWindow::Create(const char* /*title*/, int /*width*/, int /*height*/, int /*bpp*/, bool /*bFullScreen*/)
{
	assert(m_hWnd && m_hInstance && "Make sure to call SetHandles() before the window is created!");
}
