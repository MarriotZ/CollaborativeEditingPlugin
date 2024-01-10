#ifndef __MYDATACONFLICTHINTDIALOG_H__
#define __MYDATACONFLICTHINTDIALOG_H__

#include <MYUserAppSkeleton/MYUserDialog.h>

namespace Ui { class MYDataConflictHintDialog; };

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYDataConflictHintDialog
///	Author:		MaxTsang
///	Time:		
/// Summary:	≤Ó“Ï≈–∂œ
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MYDataConflictHintDialogPrivate;

class MYDataConflictHintDialog : public MYUserDialog
{
	Q_OBJECT

public:
	MYDataConflictHintDialog(QWidget* parent = 0);
	~MYDataConflictHintDialog();

	void Initialize(const QString& Name, const UINT32& Type);
	void Cleanup();

	void GetConflictWay(bool& IsShowDlg, UINT32& Solution);

private slots:
	void OnRadioButtonClicked();
	void OnPushButtonOkClicked();
	void OnPushButtonCancelClicked();

private:
	Ui::MYDataConflictHintDialog* ui;
	MYDataConflictHintDialogPrivate* _p;
};

#endif //__MYDATACONFLICTHINTDIALOG_H__