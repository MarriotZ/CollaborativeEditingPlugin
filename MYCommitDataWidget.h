#ifndef __MYCOMMITDATAWIDGET_H__
#define __MYCOMMITDATAWIDGET_H__

#include <QWidget>
#include <TopSimRuntime/TopSimCommTypes.h>
#include <MYScenarioLib/MYCollaborativeEditingPRC.h>
#include <MYCollaborative_Inter/Defined.h>

namespace Ui { class MYCommitDataWidget; };

class QStandardItem;

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYCommitDataWidget
///	Author:		MaxTsang
///	Time:		
/// Summary:	协同数据	
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MYCommitDataWidgetPrivate;

class MYCommitDataWidget : public QWidget
{
	Q_OBJECT

public:
	MYCommitDataWidget(QWidget* parent = 0);
	~MYCommitDataWidget();

	bool Initialize();
	bool Cleanup();

	void SetIsUpdateCallback(boost::function<bool(void) > Callback);

signals:
	void SigShowLog(const QString& Log);

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);

public slots:
	void OnCommitData();
	void filter(QString filterStr);
	void OnPushButtonRefreshClicked();
	void OnCheckBoxAllSelectStateChanged(int state);
	void OnItemModelItemChanged(QStandardItem* Item);

private:
	QStandardItem* GetOrCreateEntityItem(TSITopicContextPtr LocalCtx);
	QStandardItem* GetOrCreateResourceItem(TSITopicContextPtr LocalCtx);
	QStandardItem* GetOrCreateForceItem(TSITopicContextPtr LocalCtx);
	QStandardItem* GetOrCreatePlanItem(TSITopicContextPtr LocalCtx);
	void UpdateCommitData();
	void AddDataModifierVec(Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier);

private:
	Ui::MYCommitDataWidget* ui;
	MYCommitDataWidgetPrivate* _p;
};

#endif //__MYCOMMITDATAWIDGET_H__