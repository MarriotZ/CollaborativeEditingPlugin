#include <MYUtil/MYUtil.h>
#include <MYUICommon/MYAppCenter.h>

#include <MYScenarioLib/MYScenarioCEHelper.h>
#include <MYScenarioLib/MYIScenarioDataHandler.h>
#include <MYScenarioLib/MYIScenarioDataSource.h>

#include <MYUserAppSkeleton/MYUserDialog.h>

#include "MYCollaborativeEditWidget.h"
#include "MYCollaborativeEditingPlugin_Decl.h"
#include "MYCommitDataWidget.h"

#include "ui_MYCollaborativeEditWidget.h"

bool g_IsOpenNewScenario = true;

struct MYCollaborativeEditWidgetPrivate
{
	MYCollaborativeEditWidgetPrivate()
	{

	}
};

MYCollaborativeEditWidget::MYCollaborativeEditWidget(QWidget *parent /*= 0*/)
	: QWidget(parent)
	, _p(new MYCollaborativeEditWidgetPrivate())
{
	ui = new Ui::MYCollaborativeEditWidget();
	ui->setupUi(this);

	ui->splitter->setStretchFactor(0, 3);
	ui->splitter->setStretchFactor(1, 7);
	ui->widget_CEGroup->setEnabled(false);

	connect(ui->widget_Commit, SIGNAL(SigShowLog(const QString&)), this, SLOT(OnShowLog(const QString&)));
	connect(ui->widget_Update, SIGNAL(SigShowLog(const QString&)), this, SLOT(OnShowLog(const QString&)));

	MYScenarioCEHelper::Instance()->RegisterJoinCECallback(
		boost::bind(&MYCollaborativeEditWidget::OnJoinCollaborativeEdit, this));

	MYScenarioCEHelper::Instance()->RegisterExitCECallback(
		boost::bind(&MYCollaborativeEditWidget::OnExitCollaborativeEdit, this));

	ui->widget_Commit->SetIsUpdateCallback(boost::bind(&MYUpdateDataWidget::IsUpdate, ui->widget_Update));
	ui->widget_Update->SetRefreshCommitDataCallback(
		boost::bind(&MYCommitDataWidget::OnPushButtonRefreshClicked, ui->widget_Commit));
}

MYCollaborativeEditWidget::~MYCollaborativeEditWidget()
{
	delete _p;
	delete ui;
}

void MYCollaborativeEditWidget::Initialize()
{
	ui->widget_CERoom->Initialize();
}

void MYCollaborativeEditWidget::Cleanup()
{
	ui->widget_CERoom->Cleanup();
}

void MYCollaborativeEditWidget::UpdateRoom()
{
	if (g_IsOpenNewScenario)
	{
		ui->widget_CERoom->Cleanup();
	}

	g_IsOpenNewScenario = true;
}

void MYCollaborativeEditWidget::UpdateData()
{
	if (ui->widget_CEGroup->isEnabled())
	{
		MYAppCenter::ShowProgress(0, 0, true);
		MYAppCenter::SetProgress(FromAscii("正在刷新数据......"));

		ui->widget_Commit->Initialize();
		ui->widget_Update->Initialize();

		MYAppCenter::HideProgress();
	}
}

void MYCollaborativeEditWidget::UpdateWidget(const TSString &study, const TSString &scenario)
{
	
}

void MYCollaborativeEditWidget::OnApply()
{
	setEnabled(true);
}

void MYCollaborativeEditWidget::OnShowLog(const QString& Log)
{
	ui->label_Log->setText(Log);
}

void MYCollaborativeEditWidget::OnClose()
{
	void();
}

void MYCollaborativeEditWidget::OnSave()
{
	void();
}

void MYCollaborativeEditWidget::OnOpen()
{
	void();
}

void MYCollaborativeEditWidget::OnJoinCollaborativeEdit()
{
	ui->widget_CEGroup->setEnabled(true);
}

void MYCollaborativeEditWidget::OnExitCollaborativeEdit()
{
	ui->widget_CEGroup->setEnabled(false);
	ui->widget_Commit->Cleanup();
	ui->widget_Update->Cleanup();

	ui->label_Log->setText("");
}

////////////////////////////////////////////////////////

#include <MYScenarioLib/MYScenarioMgrCenterViewer.h>

struct MYCollaborativeEditRegisterPrivate
{
	MYCollaborativeEditRegisterPrivate()
		: _EditDlg(NULL)
		, _Register(NULL)
		, _UserDialog(NULL)
	{

	}

	MYCollaborativeEditWidget* _EditDlg;
	MYUserDialog* _UserDialog;
	MYScenarioMgrRegisterPtr _Register;
};

MYCollaborativeEditRegister::MYCollaborativeEditRegister()
	: _p(new MYCollaborativeEditRegisterPrivate())
{

}

MYCollaborativeEditRegister::~MYCollaborativeEditRegister()
{
	delete _p;
}

bool MYCollaborativeEditRegister::Initialize(QWidget * parent, int Index)
{
	_p->_EditDlg = new MYCollaborativeEditWidget(parent);
	_p->_EditDlg->Initialize();
	
	_p->_UserDialog = new MYUserDialog(parent);
	_p->_UserDialog->SetContentWidget(_p->_EditDlg);
	_p->_UserDialog->SetMaxVisible(true);
	_p->_UserDialog->SetMinVisible(false);
	_p->_UserDialog->SetCloseVisible(true);
	_p->_UserDialog->setWindowTitle(FromAscii("协同编辑"));

	//不再注册到想定管理界面
	//_p->_Register.reset(new MYScenarioMgrRegister(_p->_EditDlg, toAsciiData("协同管理")));

	//_p->_Register->StudyViewCb = boost::bind(&MYCollaborativeEditRegister::OnStudyView, this, _1);
	//_p->_Register->ScenarioViewCb = boost::bind(&MYCollaborativeEditRegister::OnScenarioView, this, _1, _2);
	//_p->_Register->ContextChangedCb = boost::bind(&MYCollaborativeEditRegister::OnContextChanged, this, _1, _2);
	//_p->_Register->SetScenarioNewCallback(boost::bind(&MYCollaborativeEditRegister::OnScenarioNew, this, _1, _2));

	//return MYScenarioMgrPluginViewer::Instance()->Register(_p->_Register, Index);
	
	return true;
}

void MYCollaborativeEditRegister::Cleanup()
{
	_p->_EditDlg->Cleanup();
}

void MYCollaborativeEditRegister::SetIsCurrent()
{
	//MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>();
	//MYIScenarioDataSourcePtr DataSource = QueryModuleT<MYIScenarioDataSource>();

	//if (DataSource && DataHandler)
	//{
	//	MYScenarioBasicInfoPtr BasicInfo = DataHandler->GetScenarioBasicInfo();

	//	if (BasicInfo)
	//	{
	//		MYScenarioMgrPluginViewer::Instance()->SetCurrentScenario(DataSource->GetContextStudyId()
	//			, BasicInfo->Name);
	//	}
	//}

	//MYScenarioMgrRegisterPtr CurRegister = MYScenarioMgrPluginViewer::Instance()->GetCurrentRegister();

	//if (_p->_Register == CurRegister)
	//{
	//	MYScenarioMgrPluginViewer::Instance()->SetVisible(!MYScenarioMgrPluginViewer::Instance()->GetVisible());
	//}
	//else
	//{
	//	MYScenarioMgrPluginViewer::Instance()->SetVisible(true);
	//}

	if (_p->_EditDlg)
	{
		_p->_EditDlg->UpdateData();
	}

	_p->_UserDialog->setVisible(!_p->_UserDialog->isVisible());

	//MYScenarioMgrPluginViewer::Instance()->SetCurrentRegister(_p->_Register);
}

void MYCollaborativeEditRegister::UpdateRoom()
{
	_p->_EditDlg->UpdateRoom();
}

void MYCollaborativeEditRegister::OnContextChanged(const TSString & StudyId, const TSString & ScenarioName)
{
	//if (_p->_EditDlg)
	//{
	//	_p->_EditDlg->UpdateWidget(StudyId, ScenarioName);
	//}

	//MYScenarioMgrPluginViewer::Instance()->SetRegisterVisible(_p->_Register, true);
}

void MYCollaborativeEditRegister::OnScenarioView(const TSString & StudyId, const TSString & ScenarioName)
{
	//if (_p->_EditDlg)
	//{
	//	_p->_EditDlg->UpdateWidget(StudyId, ScenarioName);
	//}

	//MYScenarioMgrPluginViewer::Instance()->SetRegisterVisible(_p->_Register, true);
}

void MYCollaborativeEditRegister::OnStudyView(const TSString & StudyId)
{
	//MYScenarioMgrPluginViewer::Instance()->SetRegisterVisible(_p->_Register, false);
}

void MYCollaborativeEditRegister::OnScenarioNew(const TSString & StudyId, MYScenarioBasicInfoPtr BasicInfo)
{
	/*if (_p->_EditDlg)
	{
		_p->_EditDlg->setEnabled(false);
	}*/
}
