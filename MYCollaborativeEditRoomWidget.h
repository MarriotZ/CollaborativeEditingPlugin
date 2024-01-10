#ifndef __MYCOLLABORATIVEEDITROOMWIDGET_H__
#define __MYCOLLABORATIVEEDITROOMWIDGET_H__

#include <QWidget>
#include <TopSimRuntime/TopSimCommTypes.h>
#include <MYEventDeclare/MYEventDeclare.h>

namespace Ui {class MYCollaborativeEditRoomWidget;};

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYCollaborativeEditRoomWidget
///	Author:		MaxTsang
///	Time:		
/// Summary:	·¿¼ä²Ù×÷
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MYCollaborativeRoomWidgetPrivate;

class MYCollaborativeEditRoomWidget : public QWidget
{
	Q_OBJECT

public:
	MYCollaborativeEditRoomWidget(QWidget *parent = 0);
	~MYCollaborativeEditRoomWidget();

	void Initialize();
	void Cleanup();

private:
	void ReBuildTreeWidget();
	void OnChanged(const TSString& OldName, const TSString& NewName, bool IsStudy);

private slots:
	void OnPushButtonCreateRoomClicked();
	void OnPushButtonJoinRoomClicked();
	void OnPushButtonRefreshClicked();
	void OnPushButtonExitRoomClicked();
	void OnPushButtonDestroyRoomClicked();

private:
	Ui::MYCollaborativeEditRoomWidget    * ui;
	MYCollaborativeRoomWidgetPrivate *_p;
};

#endif //__MYCOLLABORATIVEEDITROOMWIDGET_H__
