#ifndef __MYCOLLABORATIVEUSERNAMEDIALOG_H__
#define __MYCOLLABORATIVEUSERNAMEDIALOG_H__

#include <MYUserAppSkeleton/MYUserDialog.h>

#include <MYScenarioLib/MYCollaborativeEditingPRC.h>

namespace Ui { class MYCollaborativeUserNameDialog; };

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYCollaborativeUserNameDialog
///	Author:		MaxTsang
///	Time:		
/// Summary:	
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MYCollaborativeUserNameDialogPrivate;

class MYCollaborativeUserNameDialog : public MYUserDialog
{
	Q_OBJECT

public:
	MYCollaborativeUserNameDialog(MYCERoomInfoExPtr CERoomInfo, QWidget* parent = 0);
	~MYCollaborativeUserNameDialog();

	QString GetLayerName();

private slots:
	void OnPushButtonOkClicked();
	void OnPushButtonCancelClicked();

private:
	bool Check(const QString& LoginName);

private:
	Ui::MYCollaborativeUserNameDialog* ui;
	MYCollaborativeUserNameDialogPrivate* _p;
};

#endif //__MYCOLLABORATIVEUSERNAMEDIALOG_H__