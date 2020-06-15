#include "stdafx.h"
#include "ListCtrlSelect.h"
// In some .cpp file...

namespace ListCtrlEx {

	void SetCurSel(CListCtrl& ctrl, int ixItem, bool ensureVisible)
	{
		if (ixItem >= 0)
		{
			POSITION pos = ctrl.GetFirstSelectedItemPosition();
			while (pos)
			{
				int i = ctrl.GetNextSelectedItem(pos);
				if (i != ixItem)
					ctrl.SetItemState(i, 0, LVIS_SELECTED | LVIS_FOCUSED);
			}
			int ixCurFocus = GetFocusItem(ctrl);
			if (ixCurFocus >= 0 && ixCurFocus != ixItem)
				SetFocusItem(ctrl, -1);
			ctrl.SetItemState(ixItem,
				LVIS_SELECTED | LVIS_FOCUSED,
				LVIS_SELECTED | LVIS_FOCUSED);
			if (ensureVisible)
				ctrl.EnsureVisible(ixItem, false);
		}
		else
		{
			POSITION pos = ctrl.GetFirstSelectedItemPosition();
			while (pos)
			{
				int i = ctrl.GetNextSelectedItem(pos);
				ctrl.SetItemState(i, 0, LVIS_SELECTED);
			}
		}

	}

	int GetCurSel(const CListCtrl& ctrl)
	{
		return (ctrl.GetSelectedCount() == 1)
			? ctrl.GetNextItem(-1, LVIS_SELECTED)
			: -1;

	}

	void SetFocusItem(CListCtrl& ctrl, int ixItem, bool ensureVisible)
	{
		int ixCurFocus = GetFocusItem(ctrl);
		if (ixItem >= 0)
		{
			if (ixCurFocus != ixItem)
			{
				if (ixCurFocus >= 0)
					ctrl.SetItemState(ixCurFocus, 0, LVIS_FOCUSED);
				ctrl.SetItemState(ixItem, LVIS_FOCUSED, LVIS_FOCUSED);
			}
			if (ensureVisible)
				ctrl.EnsureVisible(ixItem, false);
		}
		else if (ixCurFocus >= 0)
			ctrl.SetItemState(ixCurFocus, 0, LVIS_FOCUSED);

	}

	int GetFocusItem(const CListCtrl& ctrl)
	{
		return ctrl.GetNextItem(-1, LVIS_FOCUSED);

	}

} // namespace ListCtrlEx
