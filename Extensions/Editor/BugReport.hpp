///////////////////////////////////////////////////////////////////////////////
/// Authors: Trevor Sundberg
/// Copyright 2010-2011, DigiPen Institute of Technology
///////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Zero
{
class Editor;
class TextEditor;
class WebResponseEvent;
class UpdateEvent;
class TextButton;

class BugReporter : public Composite
{
public:
  typedef BugReporter ZilchSelf;

  BugReporter(Composite* parent);
  ~BugReporter();

  void Reset();

  // Internal
  void OnSend(Event* event);

  TextBox* mUserName;
  TextBox* mTitle;
  SelectorButton* mSelectorButton;
  TextButton* mSend;
  TextEditor* mExpected;
  TextEditor* mHappened;
  TextEditor* mRepro;
  
  bool mSent;
};

}//namespace Zero
