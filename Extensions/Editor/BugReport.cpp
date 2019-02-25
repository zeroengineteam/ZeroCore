///////////////////////////////////////////////////////////////////////////////
/// Authors: Trevor Sundberg, Joshua Davis
/// Copyright 2010-2016, DigiPen Institute of Technology
///////////////////////////////////////////////////////////////////////////////
#include "Precompiled.hpp"

namespace Zero
{

BugReporter::BugReporter(Composite* parent) :
  Composite(parent)
{
  mSent = false;
  this->SetLayout(CreateStackLayout());
  mMinSize = Vec2(500, 600);

  new Label(this, cText, "ZeroHub Username:");
  mUserName = new TextBox(this);
  mUserName->SetEditable(true);

  mSelectorButton = new SelectorButton(this);
  mSelectorButton->CreateButton("Bug Report");
  mSelectorButton->CreateButton("Feature Request");
  mSelectorButton->SetSelectedItem(0, false);

  new Label(this, cText, "Title:");
  mTitle = new TextBox(this);
  mTitle->SetEditable(true);

  new Label(this, cText, "Reproduction steps:");
  mRepro = new TextEditor(this);
  mRepro->SetMinSize(Vec2(100, 60));
  mRepro->SetSizing(SizeAxis::Y, SizePolicy::Flex, 20);

  new Label(this, cText, "What's expected:");
  mExpected = new TextEditor(this);
  mExpected->SetMinSize(Vec2(100, 60));
  mExpected->SetSizing(SizeAxis::Y, SizePolicy::Flex, 20);
  mExpected->Append("\n\n");

  new Label(this, cText, "What happened:");
  mHappened = new TextEditor(this);
  mHappened->SetMinSize(Vec2(100, 60));
  mHappened->SetSizing(SizeAxis::Y, SizePolicy::Flex, 20);
  mHappened->Append("\n\n");

  mRepro->SetWordWrap(true);
  mRepro->DisableScrollBar(0);

  mSend = new TextButton(this);
  mSend->SetText("Send");

  ConnectThisTo(mSend, Events::ButtonPressed, OnSend);
}

BugReporter::~BugReporter()
{
}

void BugReporter::Reset()
{
  mSent = false;
  mUserName->SetText(String());
  mTitle->SetText(String());
  mExpected->SetAllText(String());
  mHappened->SetAllText(String());
  mRepro->SetAllText(String());
  mUserName->TakeFocus();

  // Set focus on username field if it's empty.
  if (mUserName->GetText().Empty())
	  mUserName->TakeFocus();
  else
    mTitle->TakeFocus();
}

void BugReporter::OnSend(Event* event)
{
  // Verify that the user entered a title
  if (mTitle->GetText().Empty())
  {
    DoNotifyWarning("Bug Reporter", "You must give the bug a title");
    return;
  }

  // Prevent multiple clicks
  if (mSent)
    return;

  // URL encoded characters
  // %2A = \n
  // %3A = :

  StringBuilder bugReportUrl;
  bugReportUrl.Append("https://github.com/zeroengineteam/ZeroCore/issues/new?");
  bugReportUrl.Append("title=");
  bugReportUrl.Append(mTitle->GetText());
  bugReportUrl.Append("&body=");
  bugReportUrl.Append("%2AUserName%3A ");
  bugReportUrl.Append(mUserName->GetText());
  bugReportUrl.Append("%0AReportType%3A ");
  bugReportUrl.Append(mSelectorButton->mButtons[mSelectorButton->GetSelectedItem()]->mButtonText->GetText());
  bugReportUrl.Append("%0AExpected%3A ");
  bugReportUrl.Append(mExpected->GetAllText());
  bugReportUrl.Append("%0AHappened%3A ");
  bugReportUrl.Append(mHappened->GetAllText());
  bugReportUrl.Append("%0ARepro%3A ");
  bugReportUrl.Append(mRepro->GetAllText());
  bugReportUrl.Append("%0ARevision%3A ");
  bugReportUrl.Append(GetRevisionNumberString());
  bugReportUrl.Append("%0AChangeSet%3A ");
  bugReportUrl.Append(GetChangeSetString());
  bugReportUrl.Append("%0APlatform%3A ");
  bugReportUrl.Append(GetPlatformString());
  bugReportUrl.Append("%0ABuildVersiovision%3A ");
  bugReportUrl.Append(GetBuildVersionName());

  
  Z::gEditor->ShowBrowser(bugReportUrl.ToString(), "Bug Report Form");
  CloseTabContaining(this);
}

}//namespace Zero
