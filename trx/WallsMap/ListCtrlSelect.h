namespace ListCtrlEx {

void SetCurSel(
      CListCtrl& ctrl,
      int ixItem,
      bool ensureVisible = true);

int GetCurSel(const CListCtrl& ctrl);

void SetFocusItem(
      CListCtrl& ctrl,
      int ixItem,
      bool ensureVisible = true);

int GetFocusItem(const CListCtrl& ctrl);

} // namespace ListCtrlEx
