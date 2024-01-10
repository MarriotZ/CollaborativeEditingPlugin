#ifndef __MYCOLLABORATIVEEDITWIDGET_H__
#define __MYCOLLABORATIVEEDITWIDGET_H__

#include <QWidget>
#include <MYScenarioLib/MYScenarioEventDecl.h>

namespace Ui { class MYCollaborativeEditWidget; };

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYCollaborativeEditWidget
///	Author:		MaxTsang
///	Time:		
/// Summary:	协同房间
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MYCollaborativeEditWidgetPrivate;

class MYCollaborativeEditWidget : public QWidget
{
	Q_OBJECT

public:
	MYCollaborativeEditWidget(QWidget* parent = 0);
	~MYCollaborativeEditWidget();

	void Initialize();
	void Cleanup();

	void UpdateRoom();
	void UpdateData();

	void UpdateWidget(const TSString& study, const TSString& scenario);

private slots:
	void OnApply();
	void OnShowLog(const QString& Log);
	void OnClose();
	void OnSave();
	void OnOpen();

private:
	void OnJoinCollaborativeEdit();
	void OnExitCollaborativeEdit();

private:
	Ui::MYCollaborativeEditWidget* ui;
	MYCollaborativeEditWidgetPrivate* _p;
};

struct MYCollaborativeEditRegisterPrivate;

class MYCollaborativeEditRegister
{
public:
	MYCollaborativeEditRegister();
	~MYCollaborativeEditRegister();

public:
	bool Initialize(QWidget* parent, int Index);
	void Cleanup();
	void SetIsCurrent();
	void UpdateRoom();

private:
	void OnContextChanged(const TSString& StudyId, const TSString& ScenarioName);
	void OnScenarioView(const TSString& StudyId, const TSString& ScenarioName);
	void OnStudyView(const TSString& StudyId);
	void OnScenarioNew(const TSString& StudyId, MYScenarioBasicInfoPtr BasicInfo);

private:
	MYCollaborativeEditRegisterPrivate* _p;
};

typedef boost::shared_ptr<MYCollaborativeEditRegister> MYCollaborativeEditRegisterPtr;

#endif //__MYCOLLABORATIVEEDITWIDGET_H__