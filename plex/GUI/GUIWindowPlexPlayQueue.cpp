#include "GUIWindowPlexPlayQueue.h"
#include "PlexApplication.h"
#include "PlexPlayQueueManager.h"
#include "music/tags/MusicInfoTag.h"
#include "GUIUserMessages.h"
#include "Application.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnSelect(int iItem)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (!item)
    return false;

  if (item->HasMusicInfoTag())
  {
    g_plexApplication.playQueueManager->playCurrentId(item->GetMusicInfoTag()->GetDatabaseId());
    return true;
  }

  return false;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::isItemPlaying(CFileItemPtr item)
{
  int playingID = -1;

  if (PlexUtils::IsPlayingPlaylist() && g_application.CurrentFileItemPtr())
  {
    if (g_application.CurrentFileItemPtr()->HasMusicInfoTag())
      playingID = g_application.CurrentFileItemPtr()->GetMusicInfoTag()->GetDatabaseId();

    if (item->HasMusicInfoTag())
      if ((playingID > 0) && (playingID == item->GetMusicInfoTag()->GetDatabaseId()))
        return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPlayQueue::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  if (PlexUtils::IsPlayingPlaylist())
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 13350);
  buttons.Add(CONTEXT_BUTTON_REMOVE_SOURCE, 1210);
  buttons.Add(CONTEXT_BUTTON_CLEAR, 192);

  if (!g_application.IsPlaying())
    buttons.Add(CONTEXT_BUTTON_EDIT, 52608);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::Update(const CStdString& strDirectory, bool updateFilterPath)
{
  CStdString dirPath = strDirectory;
  if (strDirectory.empty())
    dirPath = "plexserver://playqueue/";

  CStdString plexEditMode = m_vecItems->GetProperty("PlexEditMode").asString();
  int selectedID = m_viewControl.GetSelectedItem();

  if (CGUIPlexMediaWindow::Update(dirPath, updateFilterPath))
  {
    if (m_vecItems->Size() == 0)
    {
      OnBack(ACTION_NAV_BACK);
      return true;
    }

    // restore EditMode now that we have updated the list
    m_vecItems->SetProperty("PlexEditMode", plexEditMode);

    // restore selection if any
    if (selectedID >= 0)
      m_viewControl.SetSelectedItem(selectedID);
    else if (PlexUtils::IsPlayingPlaylist() && g_application.CurrentFileItemPtr())
      m_viewControl.SetSelectedItem(g_application.CurrentFileItemPtr()->GetPath());

    // since we call to plexserver://playqueue we need to rewrite that to the real
    // current path.
    m_startDirectory = CURL(m_vecItems->GetPath()).GetUrlWithoutOptions();
    return true;
  }
  return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_PLEX_PLAYQUEUE_UPDATED:
    {
      Update("plexserver://playqueue/", false);
      return true;
    }

    case GUI_MSG_WINDOW_INIT:
    case GUI_MSG_WINDOW_DEINIT:
      m_vecItems->SetProperty("PlexEditMode", "");
      break;
  }

  return CGUIPlexMediaWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return false;

  switch (button)
  {
    case CONTEXT_BUTTON_NOW_PLAYING:
    {
      CPlexNavigationHelper::navigateToNowPlaying();
      break;
    }
    case CONTEXT_BUTTON_REMOVE_SOURCE:
    {
      g_plexApplication.playQueueManager->removeItem(item);
      break;
    }
    case CONTEXT_BUTTON_CLEAR:
    {
      g_plexApplication.playQueueManager->clear();
      OnBack(ACTION_NAV_BACK);
      break;
    }
    case CONTEXT_BUTTON_EDIT:
    {
      // toggle edit mode
       if (!g_application.IsPlaying())
        m_vecItems->SetProperty("PlexEditMode", m_vecItems->GetProperty("PlexEditMode").asString() == "1" ? "" : "1");
      break;
    }
    default:
      break;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPlayQueue::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_DELETE_ITEM)
  {
    int i = m_viewControl.GetSelectedItem();
    CFileItemPtr item = m_vecItems->Get(i);
    if (item)
      g_plexApplication.playQueueManager->removeItem(item);
    return true;
  }

  // Long OK press, we wanna handle PQ EditMode
  if (action.GetID() == ACTION_SHOW_GUI)
  {
    OnContextButton(0, CONTEXT_BUTTON_EDIT);
    return true;
  }

  // move directly PQ items without requiring editmode
  if ((action.GetID() == ACTION_MOVE_ITEM_UP) || (action.GetID() == ACTION_MOVE_ITEM_DOWN))
  {
    int iSelected = m_viewControl.GetSelectedItem();
    if (iSelected >= 0 && iSelected < (int)m_vecItems->Size())
    {
      g_plexApplication.playQueueManager->moveItem(m_vecItems->Get(iSelected), action.GetID() == ACTION_MOVE_ITEM_UP ? 1 : -1);
      return true;
    }
  }

  // record selected item before processing
  int oldSelectedID = m_viewControl.GetSelectedItem();

  bool ret = CGUIPlexMediaWindow::OnAction(action);

  // handle cursor move if we are in editmode for PQ
  if (!m_vecItems->GetProperty("PlexEditMode").asString().empty())
  {
    switch (action.GetID())
    {
      case ACTION_MOVE_LEFT:
      case ACTION_MOVE_RIGHT:
      case ACTION_MOVE_UP:
      case ACTION_MOVE_DOWN:
        // Move the PQ item to the new selection position
        int newSelectedID= m_viewControl.GetSelectedItem();
        CFileItemPtr selectedItem = m_vecItems->Get(oldSelectedID);
        m_viewControl.SetSelectedItem(newSelectedID);

        if (oldSelectedID != newSelectedID)
          g_plexApplication.playQueueManager->moveItem(selectedItem, newSelectedID - oldSelectedID);
        break;
    }

  }

  return ret;
}
