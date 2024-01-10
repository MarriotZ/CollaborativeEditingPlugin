#ifndef __MYUPDATEDATAWIDGET_H__
#define __MYUPDATEDATAWIDGET_H__

#include <QWidget>
#include <TopSimRuntime/TopSimCommTypes.h>
#include <MYScenarioLib/MYCollaborativeEditingPRC.h>
#include <MYCollaborative_Inter/Defined.h>

namespace Ui { class MYUpdateDataWidget; };

class QStandardItem;

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYUpdateDataWidget
///	Author:		MaxTsang
///	Time:		
/// Summary:	上传数据
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MYUpdateDataWidgetPrivate;

class MYUpdateDataWidget : public QWidget
{
	Q_OBJECT

public:
	MYUpdateDataWidget(QWidget* parent = 0);
	~MYUpdateDataWidget();

	bool Initialize();
	bool Cleanup();

	bool IsUpdate();

	void SetRefreshCommitDataCallback(boost::function<void(void) > Callback);

signals:
	void SigShowLog(const QString& Log);

protected:
	virtual void keyPressEvent(QKeyEvent* event);
	virtual void keyReleaseEvent(QKeyEvent* event);

private slots:
	void OnPushButtonUpdateData();
	void OnCompareData();
	void OnUseLocalData();
	void OnUseRemoteData();

	void OnTreeWidgetCustomContextMenuRequested(const QPoint& Point);
	void filter(QString filterStr);
	void OnPushButtonRefreshClicked();

private:
	QStandardItem* GetOrCreateItem(TSInterObjectPtr IObject, const TSTOPICHANDLE& TopicHandle, const UINT32& State);
	QStandardItem* GetOrCreateEntityItem(TSInterObjectPtr LocalObj, const UINT32& State);
	QStandardItem* GetOrCreateResourceItem(TSInterObjectPtr LocalObj, const UINT32& State);
	QStandardItem* GetOrCreateForceItem(TSInterObjectPtr LocalObj, const UINT32& State);
	QStandardItem* GetOrCreatePlanItem(TSInterObjectPtr LocalObj, const UINT32& State);

	void OnDiffDataCallback(const std::vector<MYCEDiffDataPtr>& Diffs);
	void OnUpdateDataCallback(const std::vector<MYCEUpdateDataPtr>& UpdateDatas);

	void UpdateCEData();

	//获取修改人员信息
	QString GetModiferName(Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier);

private:
	Ui::MYUpdateDataWidget* ui;
	MYUpdateDataWidgetPrivate* _p;

};

#endif //__MYUPDATEDATAWIDGET_H__