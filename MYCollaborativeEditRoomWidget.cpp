#include <QTimer>

#include <MYUtil/MYUtil.h>
#include <MYUtil/MYEventBus.h>
#include <MYUICommon/MYAppCenter.h>
#include <MYUICommon/MYAppConfig.h>
#include <MYUICommon/MYIAppSkeleton.h>
#include <MYUserAppSkeleton/MYUserMessageBox.h>

#include <CollaborativeEditing_xidl/cpp/generic/Think.CollaborativeEditing.Server/Proxy.h>
#include <MYScenarioLib/MYScenarioEventDecl.h>
#include <MYScenarioLib/MYCollaborativeEditingPRC.h>
#include <MYScenarioLib/MYIScenarioDataHandler.h>
#include <MYScenarioLib/MYScenarioCEHelper.h>
#include <MYScenarioLib/MYScenarioMgrCenterViewer.h>

#include <MYUICommon/MYUICommonDeclare.h>

#include "MYCollaborativeUserNameDialog.h"
#include "MYCollaborativeEditingPlugin_Decl.h"
#include "MYCollaborativeEditRoomWidget.h"

#include "ui_MYCollaborativeEditRoomWidget.h"

Q_DECLARE_METATYPE(MYCERoomInfoExPtr);

extern bool g_IsScenarioEditing;
extern bool g_IsOpenNewScenario;

bool GetLayerName(QString& LayerName, MYCERoomInfoExPtr CERoomInfo)
{
	MYCollaborativeUserNameDialog Dlg(CERoomInfo,MYAppManager::Instance()->GetAppSkeleton());

	if (Dlg.exec() == QDialog::Accepted)
	{
		LayerName = Dlg.GetLayerName();

		return true;
	}

	return false;
}

bool ScenarioIsExist(const TSString& ScenarioName)
{
	if (MYIScenarioDataSourcePtr DataSource = QueryModuleT<MYIScenarioDataSource>())
	{
		if (MYStudyInfoPtr StudyInfo = DataSource->GetStudyInfo(DataSource->GetContextStudyId()))
		{
			std::vector<TSString>::iterator it = std::find(
				StudyInfo->Scenarios.begin(), StudyInfo->Scenarios.end(), ScenarioName);

			return it != StudyInfo->Scenarios.end();
		}
	}

	return false;
}

struct MYCollaborativeRoomWidgetPrivate
{
	MYCollaborativeRoomWidgetPrivate()
	{
	}
};

MYCollaborativeEditRoomWidget::MYCollaborativeEditRoomWidget(QWidget *parent /*= 0*/)
	: QWidget(parent)
	, _p(new MYCollaborativeRoomWidgetPrivate())
{
	ui = new Ui::MYCollaborativeEditRoomWidget();
	ui->setupUi(this);

	//MYScenarioMgrPluginViewer::Instance()->SetChangedCallback(
	//	boost::bind(&MYCollaborativeEditRoomWidget::OnChanged, this, _1, _2, _3));

	connect(ui->pushButton_CreateRoom, SIGNAL(clicked()), this, SLOT(OnPushButtonCreateRoomClicked()));
	connect(ui->pushButton_JoinRoom, SIGNAL(clicked()), this, SLOT(OnPushButtonJoinRoomClicked()));
	connect(ui->pushButton_Refresh, SIGNAL(clicked()), this, SLOT(OnPushButtonRefreshClicked()));

	connect(ui->pushButton_DestroyRoom, SIGNAL(clicked()), this, SLOT(OnPushButtonDestroyRoomClicked()));
	connect(ui->pushButton_ExitRoom, SIGNAL(clicked()), this, SLOT(OnPushButtonExitRoomClicked()));

	ui->pushButton_DestroyRoom->setVisible(false);
	ui->pushButton_ExitRoom->setVisible(false);
}

MYCollaborativeEditRoomWidget::~MYCollaborativeEditRoomWidget()
{
	delete _p;
	delete ui;
}

void MYCollaborativeEditRoomWidget::Initialize()
{
	
}

void MYCollaborativeEditRoomWidget::Cleanup()
{
	if (MYScenarioCEHelper::Instance()->GetCERoleType() == MYScenarioCEHelper::CER_Creator)
	{
		if (CEServer())
		{
			CEServer()->DestroyRoom();
		}
	}
	else if (MYScenarioCEHelper::Instance()->GetCERoleType() == MYScenarioCEHelper::CER_Entrants)
	{
		if (CEProxy())
		{
			try
			{
				CEProxy()->Leave();
			}
			catch (TSException& e)
			{
				DEF_LOG_ERROR(toAsciiData("错误码:%1")) << e.what();
			}
		}
	}
}

void MYCollaborativeEditRoomWidget::ReBuildTreeWidget()
{
	ui->treeWidget->clear();

	try
	{
		TSString CurrentRoomName = MYCollaborativeEditingConstructor::Instance()->GetCurrentRoomName();
		TSString CurrentServerName = MYCollaborativeEditingConstructor::Instance()->GetCurrentServerName();

		std::vector<MYCERoomInfoExPtr> CERoomInfos = CEProxy()->GetRoomInfos();

		TSString ActivityCode = GetAppConfig<TSString>("Root.NC.ScenarioISN");
		TSString SeatSchemeId = GetAppConfig<TSString>("Root.NC.SeatSchemeId");

		for (size_t i = 0; i < CERoomInfos.size(); ++i)
		{
			QStringList ServerNameList = TSString2QString(CERoomInfos[i]->ServerName).split("@@").last().split("@");
			if (ServerNameList.size() == 2 && GetAppConfig<bool>("Root.Scenario.OnLine", true))
			{
				if (ActivityCode != QString2TSString(ServerNameList[0])
					|| SeatSchemeId != QString2TSString(ServerNameList[1]))
				{
					continue;
				}
			}

			QString SeatName = TSString2QString(CERoomInfos[i]->ServerName).split("#").last().split("@@").first();


			QTreeWidgetItem* Item = new QTreeWidgetItem;
			Item->setText(0, TSString2QString(CERoomInfos[i]->RoomInfo->Name) + " " + SeatName);
			Item->setData(0, Qt::UserRole + 1, QVariant::fromValue(CERoomInfos[i]));
			Item->setToolTip(0, TSString2QString(CERoomInfos[i]->RoomInfo->Name) + " " + SeatName);


			if ((CERoomInfos[i]->RoomInfo->Name == CurrentRoomName)
				&& (CERoomInfos[i]->ServerName == CurrentServerName))
			{
				QFont Font = Item->font(0);
				Font.setBold(true);
				Item->setFont(0, Font);
				Item->setFont(1, Font);
			}

			ui->treeWidget->addTopLevelItem(Item);
		}
	}
	catch (const TSException& e)
	{
		DEF_LOG_ERROR(toAsciiData("错误码:%1")) << e.what();
	}
}

void MYCollaborativeEditRoomWidget::OnChanged(const TSString& OldName, const TSString& NewName, bool IsStudy)
{
	if (!IsStudy)
	{
		MYCollaborativeEditingConstructor::Instance()->ResetRoomName(NewName);
		ReBuildTreeWidget();
	}
}

void MYCollaborativeEditRoomWidget::OnPushButtonCreateRoomClicked()
{
	g_IsOpenNewScenario = false;
	
	if (CEServer())
	{
		if (MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>())
		{
			QString LayerName;

			if (GetLayerName(LayerName, NULL))
			{
				//MYScenarioMgrPluginViewer::Instance()->OpenScenario();

				if (MYCERoleInfoPtr	RoleInfo = CEServer()->CreateRoom(DataHandler->GetScenarioBasicInfo()->Name,
					QString2TSString(LayerName)))
				{
					ui->pushButton_CreateRoom->setVisible(false);
					ui->pushButton_JoinRoom->setVisible(false);
					ui->pushButton_DestroyRoom->setVisible(true);
					ui->pushButton_ExitRoom->setVisible(false);
					MYScenarioCEHelper::Instance()->JoinCE();
					ReBuildTreeWidget();
				}
			}

			g_IsOpenNewScenario = true;
		}
	}
}

void MYCollaborativeEditRoomWidget::OnPushButtonJoinRoomClicked()
{
	//加入协同编辑的Room
	QList<QTreeWidgetItem*> Items = ui->treeWidget->selectedItems();

	if (Items.size() == 1)
	{
		if (MYCERoomInfoExPtr CERoomInfo = Items[0]->data(0, Qt::UserRole + 1).value<MYCERoomInfoExPtr>())
		{
			/*if (!ScenarioIsExist(CERoomInfo->RoomInfo->Name))
			{
				if (g_IsScenarioEditing)
				{
					int Ret = MYUserMessageBox::question(this, FromAscii("系统提示"), FromAscii("是否将更改保存到想定?"), FromAscii("是"), FromAscii("否"), FromAscii("取消"));

					if (Ret == 0)
					{
						MYEventBus::Instance()->PostEventBlocking(SE_UI_SCENARIO_SAVE_REQUEST);
					}
					else if ((Ret == -1) || (Ret == 2))
					{
						return;
					}
				}
			}
			else
			{
				int Ret = MYUserMessageBox::question(this, FromAscii("系统提示"),
					FromAscii("协同组的想定已经存在当前课题下，是否继续加入协同组?"),
					FromAscii("是"), FromAscii("否"));

				if (Ret != 0)
				{
					return;
				}
			}*/

			int Ret = MYUserMessageBox::question(this, FromAscii("系统提示"),
				FromAscii("加入将清除现有数据，是否继续加入协同组?"),
				FromAscii("是"), FromAscii("否"));

			if (Ret != 0)
			{
				return;
			}


			QString LayerName;

			if (GetLayerName(LayerName, CERoomInfo))
			{
				MYAppCenter::ShowProgress(0, 0, false);
				MYAppCenter::SetProgress(FromAscii("正在加入组..."));

				try
				{
					if (MYCERoleInfoPtr	RoleInfo = CEProxy()->Join(CERoomInfo, QString2TSString(LayerName)))
					{
						g_IsOpenNewScenario = false;
						ui->pushButton_CreateRoom->setVisible(false);
						ui->pushButton_JoinRoom->setVisible(false);

						ui->pushButton_DestroyRoom->setVisible(false);
						ui->pushButton_ExitRoom->setVisible(true);

						MYScenarioBasicInfoPtr  BasicInfo(new MYScenarioBasicInfo(RoleInfo->BasicInfo));
						MYDeployLayerPtr Layer(new MYDeployLayer(RoleInfo->DeployLayer));
						MYIdentManagerPtr IdentMgr(new MYIdentManager(RoleInfo->IdentMgr));

						if (MYCollaborativeCommon::Instance()->UpdateScenarioDataToLocal(BasicInfo, Layer, IdentMgr))
						{
							/*MYScenarioMgrPluginViewer::Instance()->SetFocusScenario(
								QueryModuleT<MYIScenarioDataSource>()->GetContextStudyId(),
								BasicInfo->Name);*/

							MYScenarioCEHelper::Instance()->JoinCE();
						}
					}
					else
					{
						MYAppCenter::HideProgress();
						MYUserMessageBox::warning(this, FromAscii("系统提示"),
							FromAscii("加入失败,请检查加入的组是否有效!"));
						return;
					}
				}
				catch (const TSException& e)
				{
					MYAppCenter::HideProgress();

					MYUserMessageBox::warning(this, FromAscii("系统提示"),
						FromAscii("加入失败,请检查加入的组是否有效"));
					return;
				}
				//加入成功
				MYEventBus::Instance()->Post(SE_UI_COLLABORATIVE_JOINSTATE, TSVariant::FromValue(true));
				MYAppCenter::HideProgress();
			}
			else
			{
				return;
			}
		}

		QFont Font = Items[0]->font(0);

		Font.setBold(true);
		Items[0]->setFont(0, Font);
		Items[0]->setFont(0, Font);
	}
	else
	{
		MYUserMessageBox::warning(this, FromAscii("系统提示"), FromAscii("请选择要加入的协同想定！"));
	}
}

void MYCollaborativeEditRoomWidget::OnPushButtonRefreshClicked()
{
	MYAppCenter::ShowProgress(0, 0, false);
	MYAppCenter::SetProgress(FromAscii("正在获取组列表..."));

	ReBuildTreeWidget();

	MYAppCenter::HideProgress();
}

void MYCollaborativeEditRoomWidget::OnPushButtonExitRoomClicked()
{
	if (MYUserMessageBox::question(this, FromAscii("系统提示"),
		FromAscii("是否退出协同组?"), FromAscii("是"), FromAscii("否")) != 0)
	{
		return;
	}

	MYAppCenter::ShowProgress(0, 0, false);
	MYAppCenter::SetProgress(FromAscii("正在退出组..."));

	if (CEProxy())
	{
		try
		{
			MYCollaborativeEditingConstructor::Instance()->CEClear();
			CEProxy()->Leave();
		}
		catch (TSException& e)
		{
			DEF_LOG_ERROR(toAsciiData("错误码:%1")) << e.what();
		}

		for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
		{
			if (QTreeWidgetItem* Item = ui->treeWidget->topLevelItem(i))
			{
				QFont Font = Item->font(0);
				Font.setBold(false);
				Item->setFont(0, Font);
				Item->setFont(1, Font);
			}
		}
		
		MYScenarioCEHelper::Instance()->ExitCE();
		//MYScenarioMgrPluginViewer::Instance()->SetFocusScenario(QueryModuleT<MYIScenarioDataSource>()->GetContextStudyId(),
		//	QueryModuleT<MYIScenarioDataHandler>()->GetScenarioBasicInfo()->Name);
		
		ui->pushButton_ExitRoom->setVisible(false);
		ui->pushButton_DestroyRoom->setVisible(false);
		ui->pushButton_CreateRoom->setVisible(true);
		ui->pushButton_JoinRoom->setVisible(true);
	}
	MYEventBus::Instance()->Post(SE_UI_COLLABORATIVE_JOINSTATE, TSVariant::FromValue(false));

	MYAppCenter::HideProgress();
}

void MYCollaborativeEditRoomWidget::OnPushButtonDestroyRoomClicked()
{
	if (MYUserMessageBox::question(this, FromAscii("系统提示"), 
		FromAscii("是否销毁协同组?"), FromAscii("是"), FromAscii("否")) != 0)
	{
		return;
	}

	if (CEServer())
	{
		CEServer()->DestroyRoom();
	}

	ui->pushButton_ExitRoom->setVisible(false);
	ui->pushButton_DestroyRoom->setVisible(false);
	ui->pushButton_CreateRoom->setVisible(true);
	ui->pushButton_JoinRoom->setVisible(true);

	ReBuildTreeWidget();

	MYScenarioCEHelper::Instance()->ExitCE();
}
