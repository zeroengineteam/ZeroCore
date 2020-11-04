///////////////////////////////////////////////////////////////////////////////
///
/// Authors: Trevor Sundberg
/// Copyright 2016, DigiPen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#include "Precompiled.hpp"



namespace Zero
{
Browser::Browser(const BrowserSetup& setup)
{

}

Browser::~Browser()
{

}


/// Returns true if this browser is implemented as a browser that floats on top of everything.
/// If the browser is floating, it may not invoke mOnPaint as it may be painted by the operating system.
bool Browser::IsFloatingOnTop()
{
  return false;
}

/// Returns if this browser is considered secure which means that it follows browsing standards
/// for security, such as not allowing cross-origin requests unless explicitly allowed by the server.
bool Browser::IsSecurityRestricted()
{
  return false;
}

Math::IntVec2 Browser::GetSize()
{
  return Math::IntVec2();
}

void Browser::SetSize(Math::IntVec2Param size)
{
}

/// This is only used when the browser 'IsFloatingOnTop'.
Math::IntVec2 Browser::GetClientPosition()
{
  return Math::IntVec2();
}

void Browser::SetClientPosition(Math::IntVec2Param clientPosition)
{

}

/// Sets the clipping rectangles (only used if 'IsFloatingOnTop').
/// These rectangles will be clipped out of the view, as if holes are being cut.
/// Note that this might not be supported by all platforms.
void Browser::SetClippingRectangles(const Array<IntRect>& rectangles)
{

}


/// This is only used when the browser 'IsFloatingOnTop'.
int Browser::GetZIndex()
{
  return 0;
}

void Browser::SetZIndex(int zindex)
{
}

bool Browser::GetCanGoForward()
{
  return false;
}

bool Browser::GetCanGoBackward()
{
  return false;
}

void Browser::GoForward() {}
void Browser::GoBackward() {}

bool Browser::GetIsLoading()
{
  return false;
}

void Browser::Reload(bool useCache) {}

void Browser::SetFocus(bool focus) {}
bool Browser::GetFocus()
{
  return false;
}

void Browser::SetVisible(bool visible)
{

}

bool Browser::GetVisible()
{
  return false;
}

Math::Vec4 Browser::GetBackgroundColor()
{
  return Math::Vec4();
}

void Browser::SetBackgroundColor(Math::Vec4Param color)
{

}

bool Browser::GetTransparent()
{
  return false;
}

void Browser::SetTransparent(bool transparent)
{

}

String Browser::GetStatus()
{
  return String();
}

String Browser::GetTitle()
{
  return String();
}

Math::Vec2 Browser::GetScrollSpeed()
{
  return Math::Vec2();
}

void Browser::SetScrollSpeed(Math::Vec2Param pixelsPerScroll)
{

}

void Browser::SetUrl(StringParam url) {}
String Browser::GetUrl()
{
  return String();
}

void Browser::ExecuteScriptFromLocation(StringParam script, StringParam url, int line) {}

void Browser::SimulateKey(int key, bool down, BrowserModifiers::Enum modifiers) {}
void Browser::SimulateTextTyped(int character, BrowserModifiers::Enum modifiers) {}
void Browser::SimulateMouseMove(Math::IntVec2Param localPosition, BrowserModifiers::Enum modifiers) {}
void Browser::SimulateMouseClick(Math::IntVec2Param localPosition, MouseButtons::Enum button, bool down, BrowserModifiers::Enum modifiers) {}
void Browser::SimulateMouseDoubleClick(Math::IntVec2Param localPosition, MouseButtons::Enum button, BrowserModifiers::Enum modifiers) {}
void Browser::SimulateMouseScroll(Math::IntVec2Param localPosition, Math::Vec2Param delta, BrowserModifiers::Enum modifiers) {}

// This must be called before any browsers are created
void Browser::PlatformCreate() {}
void Browser::PlatformDestroy() {}
void Browser::PlatformUpdate() {}

const bool BrowserSubProcess::IsRequired = true;

//class ZeroApp : public CefApp
//{
//public:
//  void OnBeforeCommandLineProcessing(const CefString& process, CefRefPtr<CefCommandLine> commands) override
//  {
//  }
//
//};

int BrowserSubProcess::Execute()
{
  return 0;
}

} // namespace Zero
