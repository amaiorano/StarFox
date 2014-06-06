#include "gs/Rendering/GraphicsEngine.h"
#include "Win32Headers.h"

class Win32Window;

class Win32GLGraphicsEngine : public GraphicsEngine
{
private:
	friend class GraphicsEngine;
	Win32GLGraphicsEngine();

public:
	virtual void Initialize(const char* title, int width, int height, int bpp, ScreenMode::Type screenMode, VertSync::Type vertSync);
	virtual void Shutdown();
	virtual void Update(bool& bQuit, bool bIsPaused = false);
	virtual void SetTitle(const char* title);
	virtual bool HasFocus() const;

protected:
	virtual void DoSetActiveViewport(const Viewport& viewport);

private:
	friend void OnWindowResized(float32 newWidth, float32 newHeight);	

	Win32Window* m_pWindow;
	HDC m_hDC;
	HGLRC m_hRC;
};
