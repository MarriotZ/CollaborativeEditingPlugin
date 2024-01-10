#include <QStandardItemModel>
#include <QKeyEvent>
#include <QMenu>
#include <QAction>
#include <QtnThinkHelper.h>

#include <TopSimMissionScheduler_Inter/Topic.h>
#include <TopSimMissionScheduler_Inter/Defined.h>

#include <MYUtil/MYEventBus.h>
#include <MYUtil/MYUtil.h>
#include <MYUICommon/MYAppManager.h>
#include <MYUICommon/MYAppCenter.h>
#include <MYUICommon/MYIIconManager.h>

#include <MYControls/MYModelBoxFilterWidget.h>
#include <MYControls/MYUserSortFilterProxyModel.h>

#include <MYEventDeclare/MYEventDeclare.h>
#include <MYUserAppSkeleton/MYUserMessageBox.h>
#include <MYScenarioLib/MYScenarioEventDecl.h>
#include <MYScenarioLib/MYScenarioCEHelper.h>
#include <MYScenarioLib/MYIScenarioDataHandler.h>
#include <MYScenarioLib/MYCollaborativeEditingPRC.h>
#include <MYScenarioLib/MYScenarioMgrCenterViewer.h>
#include <MYScenarioLib/MYMissionHandler.h>

#include "MYUpdateDataWidget.h"
#include "MYDataCompareDialog.h"
#include "MYCollaborativeEditingPlugin_Decl.h"

#include <MYForceModels_Inter/MYForceModelsDeclare.h>

#include <MYBaseLib/MYIForceOrganizationDataSource.h>
#include <MYUICommon/MYUICommonDeclare.h>
#include <MYPlan_Inter/Defined.h>
#include <Tools_Inter/Defined.h>

#include "ui_MYUpdateDataWidget.h"

Q_DECLARE_METATYPE(std::vector<MYCEDiffDataPtr>);
Q_DECLARE_METATYPE(std::vector<MYCEUpdateDataPtr>);

typedef Think::Simulation::Scheduler::GroupSchedParam::DataType MYGroupSchedParam;
typedef Think::Simulation::Scheduler::GroupSchedParam::DataTypePtr MYGroupSchedParamPtr;

void DeleteMissionSchedParam(const TSHANDLE& Handle, const UINT32& MissionId)
{
	TSDomainPtr Domain = GetScenarioDomain();
	std::map<UINT32, TSITopicContextPtr> MissionSchedParamCtxs;
	MYGroupSchedParamPtr GroupSchedParam;

	TSTopicFindSetType FindSet = Domain->CreateTopicFindSet(Handle, Think_Simulation_MMF_MissionSchedParam);

	while (TSITopicContextPtr Ctx = Domain->GetNextTopic(FindSet))
	{
		if (MYMissionSchedParamPtr MissionSched = Ctx->GetTopicT<MYMissionSchedParam>())
		{
			if (MissionSched->MissionId == MissionId)
			{
				if (Ctx->Is(Think_Simulation_Scheduler_GroupSchedParam))
				{
					GroupSchedParam = TS_CAST(MissionSched, MYGroupSchedParamPtr);
				}

				GetScenarioDomain()->DeleteTopic(Ctx, false);
			}

			MissionSchedParamCtxs[MissionSched->MissionId] = Ctx;
		}
	}

	if (GroupSchedParam)
	{
		for (size_t i = 0; i < GroupSchedParam->SchedGroups.size(); ++i)
		{
			for (size_t j = 0; j < GroupSchedParam->SchedGroups[i].SchedulerIds.size(); ++j)
			{
				std::map<UINT32, TSITopicContextPtr>::iterator It = MissionSchedParamCtxs.find(GroupSchedParam->SchedGroups[i].SchedulerIds[j]);

				if (It != MissionSchedParamCtxs.end())
				{
					GetScenarioDomain()->DeleteTopic(It->second, false);
				}
			}
		}
	}
}

void DeleteMissionParam(const TSHANDLE& Handle, const UINT32& MissionId)
{
	TSDomainPtr Domain = GetScenarioDomain();

	TSTopicFindSetType FindSet = Domain->CreateTopicFindSet(Handle, Think_Simulation_MMF_MissionParam);

	while (TSITopicContextPtr Ctx = Domain->GetNextTopic(FindSet))
	{
		if (MYMissionParamPtr MissionParam = Ctx->GetTopicT<MYMissionParam>())
		{
			if (MissionParam->MissionId == MissionId)
			{
				GetScenarioDomain()->DeleteTopic(Ctx, false);
			}
		}
	}
}

bool GetDiffIdentNames(MYIdentManagerPtr RemoteIdentMgr, std::map<TSString, TSString>& IdentNames)
{
	bool IsExistDeleteIdent = false;

	if (TSITopicContextPtr Ctx = GetScenarioDomain()->GetFirstTopicByHandle(Think_Simulation_Scenario_IdentMgr))
	{
		if (MYIdentManagerPtr IdentMgr = Ctx->GetTopicT<MYIdentManager>())
		{
			for (size_t i = 0; i < IdentMgr->Profiles.size(); ++i)
			{
				for (size_t j = 0; j < RemoteIdentMgr->Profiles.size(); ++j)
				{
					if ((IdentMgr->Profiles[i].IdentName == RemoteIdentMgr->Profiles[j].IdentName)
						&& (IdentNames.find(IdentMgr->Profiles[i].IdentName) == IdentNames.end()))
					{
						IdentNames[IdentMgr->Profiles[i].IdentName] = RemoteIdentMgr->Profiles[j].IdentName;

						break;
					}
				}

				for (size_t j = 0; j < RemoteIdentMgr->Profiles.size(); ++j)
				{
					if ((IdentMgr->Profiles[i].Color == RemoteIdentMgr->Profiles[j].Color)
						&& (IdentNames.find(IdentMgr->Profiles[i].IdentName) == IdentNames.end()))
					{
						IdentNames[IdentMgr->Profiles[i].IdentName] = RemoteIdentMgr->Profiles[j].IdentName;

						break;
					}
				}

				if (!IsExistDeleteIdent)
				{
					IsExistDeleteIdent = IdentNames[IdentMgr->Profiles[i].IdentName].empty();
				}
			}
		}
	}

	return IsExistDeleteIdent;
}

void UpdateEntityIdent(std::map<TSString, TSString>& IdentNames)
{
	if (TSDomainPtr Domain = GetScenarioDomain())
	{
		TSTopicFindSetType FindSet = Domain->CreateTopicFindSet(Think_Simulation_MMF_EntityInitInfo);

		while (TSITopicContextPtr EntityTopic = Domain->GetNextTopic(FindSet))
		{
			if (MYEntityInitInfoPtr EntityInfo = EntityTopic->GetTopicT<MYEntityInitInfo>())
			{
				std::map<TSString, TSString>::iterator It = IdentNames.find(EntityInfo->Identification);

				if (It != IdentNames.end() 
					&& !(It->second.empty())
					&& EntityInfo->Identification != It->second)
				{
					EntityInfo->Identification = It->second;

					GetScenarioDomain()->UpdateTopic(EntityTopic->GetDataTopicHandle(), EntityInfo.get());
				}
			}
		}

		FindSet = Domain->CreateTopicFindSet(Think_Simulation_MMF_DecisionEntityInitInfo);

		while (TSITopicContextPtr DecisionEntityTopic = Domain->GetNextTopic(FindSet))
		{
			if (MYDecisionEntityInitInfoPtr DecisionEntityInfo = DecisionEntityTopic->GetTopicT<MYDecisionEntityInitInfo>())
			{
				std::map<TSString, TSString>::iterator It = IdentNames.find(DecisionEntityInfo->Identification);

				if (It != IdentNames.end()
					&& !(It->second.empty())
					&& DecisionEntityInfo->Identification != It->second)
				{
					DecisionEntityInfo->Identification = It->second;
					GetScenarioDomain()->UpdateTopic(DecisionEntityTopic->GetDataTopicHandle(), DecisionEntityInfo.get());
				}
			}
		}
	}
}

bool IsEntityExistIdent(const TSString& Identification, std::map<TSString, TSString>& IdentNames)
{
	std::map<TSString, TSString>::iterator Itt = IdentNames.begin();

	for (; Itt != IdentNames.end(); ++Itt)
	{
		if (Itt->second == Identification)
		{
			return true;
		}
	}

	return false;
}

bool IsExistFormation(const TSHANDLE& Handle)
{
	TSTopicFindSetType FindSetType =
		GetScenarioDomain()->CreateTopicFindSet(Handle, Think_Simulation_Formation);

	while (TSITopicContextPtr Ctx = GetScenarioDomain()->GetNextTopic(FindSetType))
	{
		if (MYFormationInfoPtr FormationInfo = Ctx->GetTopicT<MYFormationInfo>())
		{
			return true;
		}
	}

	return false;
}

MYDeployLayerPtr GetDeployLayerByIndex(const UINT32& StartHandle, const UINT32& EndHandle)
{
	TSTopicFindSetType SetType = GetScenarioDomain()->CreateTopicFindSet(Think_Simulation_Scenario_DeployLayer);

	while (TSITopicContextPtr Ctx = GetScenarioDomain()->GetNextTopic(SetType))
	{
		if (MYDeployLayerPtr LocalLayer = Ctx->GetTopicT<MYDeployLayer>())
		{
			if ((LocalLayer->StartHandleIndex == StartHandle)
				&& (LocalLayer->EndHandleIndex == EndHandle))
			{
				return LocalLayer;
			}
		}
	}

	return MYDeployLayerPtr();
}

TSHANDLE GetMissionHandle(const UINT32& MissionId)
{
	TSTopicFindSetType SetType = GetScenarioDomain()->CreateTopicFindSet(Think_Simulation_MMF_MissionParam);

	while (TSITopicContextPtr Ctx = GetScenarioDomain()->GetNextTopic(SetType))
	{
		if (MYMissionParamPtr MissionParam = Ctx->GetTopicT<MYMissionParam>())
		{
			if (MissionParam->MissionId == MissionId)
			{
				return MissionParam->Handle;
			}
		}
	}

	return TSHANDLE();
}

struct MYUpdateDataWidgetPrivate
{
	MYUpdateDataWidgetPrivate()
		: _UseLocalAct(NULL)
		, _UseRemoteAct(NULL)
		, _CompareAct(NULL)
		, _Menu(NULL)
		, _ProxyModel(NULL)
		, _DataDelegate(NULL)
		, _PlanManagerRWItem(NULL)
		, _PlanManagerFXPDQKItem(NULL)
		, _PlanManagerDXZDJXItem(NULL)
		, _PlanManagerXDZDMLItem(NULL)
		, _PlanManagerQXFPItem(NULL)
		, _PlanManagerJDHFItem(NULL)
		, _DataModifierVecPtr(NULL)
	{

	}

	QAction* _UseLocalAct;
	QAction* _UseRemoteAct;
	QAction* _CompareAct;
	QMenu* _Menu;
	MYUserSortFilterProxyModel *_ProxyModel;
	QStandardItemModel* _ItemModel;

	MYCollaborativeDataDelegate* _DataDelegate;
	std::map<TSHANDLE, QStandardItem*> _EntityItems;
	std::map<std::string, QStandardItem*> _ForceItems;
	QStandardItem* _PlanManagerRWItem; //NC新增，筹划-理解任务
	QStandardItem* _PlanManagerFXPDQKItem; //NC新增，筹划-分析判断情况
	QStandardItem* _PlanManagerDXZDJXItem; //NC新增，筹划-定下战斗决心
	QStandardItem* _PlanManagerXDZDMLItem; //NC新增，筹划-下达战斗命令
	QStandardItem* _PlanManagerQXFPItem; //NC新增，筹划-席位权限
	QStandardItem* _PlanManagerJDHFItem; //NC新增，筹划-阶段划分
	boost::function<void(void) > _RefreshCommitCallback;

	std::map<TSString, TSString> _UpdateDataId;

	//用户修改记录
	Think::Simulation::MYCollaborativeData::CollaborativeDataModifierVec::DataTypePtr _DataModifierVecPtr;
	std::map<int, Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType> _DataModifierInfoMap;
};

MYUpdateDataWidget::MYUpdateDataWidget( QWidget *parent /*= 0*/)
	: QWidget(parent)
	, _p(new MYUpdateDataWidgetPrivate)
{
	ui = new Ui::MYUpdateDataWidget();
	ui->setupUi(this);

	ui->pushButton_Update->setEnabled(false);
	MYModelBoxFilterWidget * filterWidget = new MYModelBoxFilterWidget(this, MYModelBoxFilterWidget::LayoutAlignNone);

	filterWidget->setRefuseFocus(true);
	connect(filterWidget, SIGNAL(filterChanged(QString)), this, SLOT(filter(QString)));
	
	ui->horizontalLayout->addWidget(filterWidget);

	_p->_ItemModel = new QStandardItemModel();
	_p->_ProxyModel = new MYUserSortFilterProxyModel();
	_p->_ProxyModel->setShowChildern(true);
	_p->_ProxyModel->setFilterKeyColumn(-1);
	
	_p->_ProxyModel->setSourceModel(_p->_ItemModel);
	ui->treeView->setModel(_p->_ProxyModel);
	_p->_ItemModel->setHorizontalHeaderLabels(QStringList() << FromAscii("协同数据") << FromAscii("状态") << FromAscii("提交人"));
	ui->treeView->setColumnWidth(0, 250);
	_p->_Menu = new QMenu(this);
	_p->_UseLocalAct = new QAction(FromAscii("使用本地"), _p->_Menu);
	_p->_UseRemoteAct = new QAction(FromAscii("使用远端"), _p->_Menu);
	_p->_CompareAct = new QAction(FromAscii("对比"), _p->_Menu);

	connect(_p->_UseLocalAct, SIGNAL(triggered()), this, SLOT(OnUseLocalData()));
	connect(_p->_UseRemoteAct, SIGNAL(triggered()), this, SLOT(OnUseRemoteData()));
	connect(_p->_CompareAct, SIGNAL(triggered()), this, SLOT(OnCompareData()));
	connect(ui->pushButton_Update, SIGNAL(clicked()), this, SLOT(OnPushButtonUpdateData()));
	connect(ui->pushButton_Refresh, SIGNAL(clicked()), this, SLOT(OnPushButtonRefreshClicked()));

	_p->_Menu->addAction(_p->_UseLocalAct);
	_p->_Menu->addSeparator();
	_p->_Menu->addAction(_p->_UseRemoteAct);
	_p->_Menu->addSeparator();
	_p->_Menu->addSeparator();
	_p->_Menu->addAction(_p->_CompareAct);

	connect(ui->treeView, SIGNAL(customContextMenuRequested(const QPoint &)),this,
		SLOT(OnTreeWidgetCustomContextMenuRequested(const QPoint &)));

	_p->_DataDelegate = new MYCollaborativeDataDelegate(ui->treeView);
	_p->_DataDelegate->Init(_p->_ItemModel, _p->_ProxyModel);
	ui->treeView->setItemDelegate(_p->_DataDelegate);
}

MYUpdateDataWidget::~MYUpdateDataWidget()
{
	delete _p;
	delete ui;
}

bool MYUpdateDataWidget::Initialize()
{
	if (_p->_ItemModel->rowCount())
	{
		return true;
	}

	if ((MYScenarioCEHelper::Instance()->GetCERoleType() == MYScenarioCEHelper::CER_Entrants
		&& !MYCollaborativeEditingConstructor::Instance()->IsOnLine()))
	{
		return true;
	}

	UpdateCEData();

	return true;
}

bool MYUpdateDataWidget::Cleanup()
{
	_p->_PlanManagerRWItem = NULL;
	_p->_PlanManagerFXPDQKItem = NULL;
	_p->_PlanManagerDXZDJXItem = NULL;
	_p->_PlanManagerXDZDMLItem = NULL;
	_p->_PlanManagerQXFPItem = NULL;
	_p->_PlanManagerJDHFItem = NULL;
	_p->_ForceItems.clear();
	_p->_EntityItems.clear();
	_p->_ItemModel->clear();
	_p->_ProxyModel->clear();
	_p->_ItemModel->setHorizontalHeaderLabels(QStringList() << FromAscii("协同数据") << FromAscii("状态") << FromAscii("提交人"));
	ui->treeView->setColumnWidth(0, 250);
	_p->_DataModifierInfoMap.clear();
	if (_p->_DataModifierVecPtr)
	{
		_p->_DataModifierVecPtr->DataModifierVec.clear();
	}
	return true;
}

void MYUpdateDataWidget::keyPressEvent(QKeyEvent *event)
{
	QWidget::keyPressEvent(event);

	if (event->key() == Qt::Key_Control)
	{
		ui->treeView->setSelectionMode(QAbstractItemView::MultiSelection);
	}
	else if (event->key() == Qt::Key_Shift)
	{
		ui->treeView->setSelectionMode(QAbstractItemView::ContiguousSelection);
	}
}

void MYUpdateDataWidget::keyReleaseEvent(QKeyEvent *event)
{
	QWidget::keyReleaseEvent(event);
	ui->treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MYUpdateDataWidget::OnDiffDataCallback(const std::vector<MYCEDiffDataPtr> & Diffs)
{
	for (size_t i = 0; i < Diffs.size(); ++i)
	{
		if (MYCEDiffDataPtr DiffData = Diffs[i])
		{
			if (_p->_UpdateDataId.find(DiffData->Id) != _p->_UpdateDataId.end())
			{
				continue;
			}

			if (QStandardItem* LocalItem = GetOrCreateItem(DiffData->MimeIObj, 
				DiffData->ThierTopicHandle, MYCEDataState::DS_Update))
			{
				std::vector<MYCEDiffDataPtr> Datas = LocalItem->data(DiffDataRole).value<std::vector<MYCEDiffDataPtr> >();
				Datas.push_back(DiffData);
				LocalItem->setData(QVariant::fromValue(Datas), DiffDataRole);
				LocalItem->setData(QVariant::fromValue(SDT_Conflict), ScenarioDataTypeRole);
				LocalItem->setData(QVariant::fromValue(DDS_UseRemote), SolveWayRole);
				LocalItem->setData(QVariant::fromValue(false), IsSolveConflictRole);
			}
		}
	}
}

void MYUpdateDataWidget::OnUpdateDataCallback(const std::vector<MYCEUpdateDataPtr>& UpdateDatas)
{
	std::vector<MYCEUpdateDataPtr> EntityDatas;
	std::vector<MYCEUpdateDataPtr> ComDatas;
	std::vector<MYCEUpdateDataPtr> Virtuals;
	_p->_DataModifierInfoMap.clear();

	for (std::size_t i = 0; i < UpdateDatas.size(); ++i)
	{
		if (_p->_UpdateDataId.find(UpdateDatas[i]->Id) != _p->_UpdateDataId.end())
		{
			continue;
		}

		TSInterObjectPtr IObject = UpdateDatas[i]->IObjs;
		TSTOPICHANDLE TopicHandle = UpdateDatas[i]->TopicHandle;

		TSTopicHelperPtr TopicHelper = TSTopicTypeManager::Instance()->GetTopicHelperByTopic(TopicHandle);

		if (TopicHandle == Think_Simulation_Scenario_BasicInfo)
		{
			MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>();
			MYIScenarioDataSourcePtr DataSource = QueryModuleT<MYIScenarioDataSource>();

			MYScenarioBasicInfoPtr ScenarioInfo = TS_CAST(IObject, MYScenarioBasicInfoPtr);

			if (DataHandler && ScenarioInfo)
			{
				if (MYScenarioBasicInfoPtr TmpScenarioInfo = DataHandler->GetScenarioBasicInfo())
				{
					GetScenarioDomain()->DeleteTopic(Think_Simulation_Scenario_BasicInfo, TmpScenarioInfo.get(), false);
					GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_BasicInfo, ScenarioInfo.get());

					if (ScenarioInfo->Name != TmpScenarioInfo->Name)
					{
						if (MYStudyInfoPtr StudyInfo = DataSource->GetStudyInfo(DataSource->GetContextStudyId()))
						{
							std::vector<TSString>::iterator it = std::find(
								StudyInfo->Scenarios.begin(), StudyInfo->Scenarios.end(), TmpScenarioInfo->Name);

							if (it != StudyInfo->Scenarios.end())
							{
								StudyInfo->Scenarios.erase(it);
								StudyInfo->Scenarios.push_back(ScenarioInfo->Name);
							}
						}

						//MYScenarioMgrPluginViewer::Instance()->ScenarioChanged(TmpScenarioInfo->Name, ScenarioInfo->Name);
					}

					/*MYScenarioMgrPluginViewer::Instance()->SetCurrentScenario(
						DataSource->GetContextStudyId(), ScenarioInfo->Name);*/
				}
				else
				{
					GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_BasicInfo, ScenarioInfo.get());
				}
			}
		}
		else if (TopicHandle == Think_Simulation_Scenario_DeployLayer)
		{
			if (MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>())
			{
				if (MYDeployLayerPtr RemoteLayer = TS_CAST(IObject, MYDeployLayerPtr))
				{
					if (MYDeployLayerPtr LocalLayer =
						GetDeployLayerByIndex(RemoteLayer->StartHandleIndex, RemoteLayer->EndHandleIndex))
					{
						if (LocalLayer->Name != RemoteLayer->Name
							|| LocalLayer->Description != RemoteLayer->Description)
						{
							TSString OldLayerName = LocalLayer->Name;

							GetScenarioDomain()->DeleteTopic(Think_Simulation_Scenario_DeployLayer, LocalLayer.get(), false);
							LocalLayer->Name = RemoteLayer->Name;
							LocalLayer->Description = RemoteLayer->Description;
							GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_DeployLayer, LocalLayer.get());

							if (OldLayerName == DataHandler->GetCurrentDeployLayer())
							{
								DataHandler->SetCurrentDeployLayer(LocalLayer->Name);
							}

							MYEventBus::Instance()->PostEventBlocking(SE_UI_SCENARIO_DEPLOYLAYER_ADD);
						}
					}
					else
					{
						GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_DeployLayer, RemoteLayer.get());
						MYEventBus::Instance()->PostEventBlocking(SE_UI_SCENARIO_DEPLOYLAYER_ADD);
					}
				}
			}
		}
		else if (TopicHandle == Think_Simulation_Scenario_IdentMgr)
		{
			if (MYIdentManagerPtr RemoteIdentMgr = TS_CAST(IObject, MYIdentManagerPtr))
			{
				std::map<TSString, TSString> IdentNames;

				if (!GetDiffIdentNames(RemoteIdentMgr, IdentNames))
				{
					UpdateEntityIdent(IdentNames);

					GetScenarioDomain()->UpdateTopic(TopicHandle, IObject.get());
					MYEventBus::Instance()->PostEventBlocking(SE_UI_IDENT_CHANGED);

					MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>();
					MYIScenarioDataSourcePtr DataSource = QueryModuleT<MYIScenarioDataSource>();

					if (DataHandler && DataSource)
					{
						if (MYScenarioBasicInfoPtr ScenarioInfo = DataHandler->GetScenarioBasicInfo())
						{
							/*MYScenarioMgrPluginViewer::Instance()->SetCurrentScenario(
								DataSource->GetContextStudyId(), ScenarioInfo->Name);*/
						}
					}
				}
				else
				{
					QStandardItem* IdentItem = new QStandardItem(
						QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("XScenario/图片/标识管理.png")), 
						FromAscii("标识管理"));
					IdentItem->setEditable(false);
					_p->_ItemModel->appendRow(IdentItem);

					QString TipInfo;

					for (size_t m = 0; m < RemoteIdentMgr->Profiles.size(); ++m)
					{
						TipInfo += TSString2QString(RemoteIdentMgr->Profiles[m].IdentName);

						if (m + 1 < RemoteIdentMgr->Profiles.size())
						{
							TipInfo += "\n";
						}
					}

					IdentItem->setToolTip(TipInfo);

					QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
					StateItem->setEditable(false);
					_p->_ItemModel->setItem(IdentItem->row(), 1, StateItem);

					std::vector<MYCEUpdateDataPtr> Datas = IdentItem->data(UpdateDataRole).value<std::vector<MYCEUpdateDataPtr> >();
					Datas.push_back(UpdateDatas[i]);
					IdentItem->setData(QVariant::fromValue(Datas), UpdateDataRole);
				}
			}
		}
		else if (TopicHelper->CanConvert(Think_Simulation_MMF_LogicEntityInitInfo)
			|| TopicHelper->CanConvert(Think_Simulation_SimResource))
		{
			EntityDatas.push_back(UpdateDatas[i]);
		}
		//zjy
		else if (TopicHelper->CanConvert(Think_Simulation_MYForces_VirtualForce))
		{
			MYVirtualForcePtr virforce = TS_CAST(IObject, MYVirtualForcePtr);
			Virtuals.push_back(UpdateDatas[i]);
		}
		else
		{
			ComDatas.push_back(UpdateDatas[i]);
		}
	}

	for (std::size_t i = 0; i < Virtuals.size(); ++i)
	{
		TSInterObjectPtr IObject = Virtuals[i]->IObjs;
		TSTOPICHANDLE TopicHandle = Virtuals[i]->TopicHandle;

		if (QStandardItem* LocalItem = GetOrCreateItem(IObject, TopicHandle, Virtuals[i]->State))
		{
			std::vector<MYCEUpdateDataPtr> Datas = LocalItem->data(UpdateDataRole).value<std::vector<MYCEUpdateDataPtr> >();
			Datas.push_back(Virtuals[i]);
			LocalItem->setData(QVariant::fromValue(Datas), UpdateDataRole);

			if (QStandardItem* StateItem = _p->_ItemModel->item(LocalItem->row(), 1))
			{
				QString State = FromAscii("修改");

				switch (Virtuals[i]->State)
				{
				case MYCEDataState::DS_Delete:
					State = FromAscii("删除");
					break;
				case MYCEDataState::DS_Add:
					State = FromAscii("新增");
					break;
				case MYCEDataState::DS_Update:
					State = FromAscii("修改");
					break;
				default:
					break;
				}

				StateItem->setText(State);
			}
		}
	}

	for (std::size_t i = 0; i < EntityDatas.size(); ++i)
	{
		TSInterObjectPtr IObject = EntityDatas[i]->IObjs;
		TSTOPICHANDLE TopicHandle = EntityDatas[i]->TopicHandle;

		if (QStandardItem* LocalItem = GetOrCreateItem(IObject, TopicHandle, EntityDatas[i]->State))
		{
			std::vector<MYCEUpdateDataPtr> Datas = LocalItem->data(UpdateDataRole).value<std::vector<MYCEUpdateDataPtr> >();
			Datas.push_back(EntityDatas[i]);
			LocalItem->setData(QVariant::fromValue(Datas), UpdateDataRole);

			if (QStandardItem* StateItem = _p->_ItemModel->item(LocalItem->row(), 1))
			{
				QString State = FromAscii("修改");

				switch (EntityDatas[i]->State)
				{
				case MYCEDataState::DS_Delete:
					State = FromAscii("删除");
					break;
				case MYCEDataState::DS_Add:
					State = FromAscii("新增");
					break;
				case MYCEDataState::DS_Update:
					State = FromAscii("修改");
					break;
				default:
					break;
				}

				StateItem->setText(State);
			}
		}
	}

	for (std::size_t i = 0; i < ComDatas.size(); ++i)
	{
		TSInterObjectPtr IObject = ComDatas[i]->IObjs;
		TSTOPICHANDLE TopicHandle = ComDatas[i]->TopicHandle;
		if (QStandardItem* LocalItem = GetOrCreateItem(IObject, TopicHandle, ComDatas[i]->State))
		{
			QVariant value = LocalItem->data(UpdateDataRole);
			std::vector<MYCEUpdateDataPtr> Datas = LocalItem->data(UpdateDataRole).value<std::vector<MYCEUpdateDataPtr> >();
			Datas.push_back(ComDatas[i]);
			LocalItem->setData(QVariant::fromValue(Datas), UpdateDataRole);		
		}
	}

	//刷新用户
	int rowCount = _p->_ItemModel->rowCount();
	if (_p->_DataModifierInfoMap.size() == rowCount)
	{
		for (int i = 0; i < rowCount; ++i)
		{
			QStandardItem* ModiferItem = new QStandardItem(GetModiferName(_p->_DataModifierInfoMap[i]));
			ModiferItem->setEditable(false);
			_p->_ItemModel->setItem(i, 2, ModiferItem);
		}
	}
}

void MYUpdateDataWidget::UpdateCEData()
{
	if (MYCollaborativeEditingConstructor::Instance()->IsLatestVersion())
	{
		return;
	}

	Cleanup();

	MYCollaborativeEditingConstructor::Instance()->Update(boost::bind(&MYUpdateDataWidget::OnDiffDataCallback, this, _1),
		boost::bind(&MYUpdateDataWidget::OnUpdateDataCallback, this, _1));
	ui->pushButton_Update->setEnabled((bool)_p->_ItemModel->rowCount());
}


QString MYUpdateDataWidget::GetModiferName(Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier)
{
	for (int i = 0; i < _p->_DataModifierVecPtr->DataModifierVec.size(); ++i)
	{
		Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType data = _p->_DataModifierVecPtr->DataModifierVec[i];
		if (data.TopicHandle == DataModifier.TopicHandle && data.Handle == DataModifier.Handle
			&& data.Keyword == DataModifier.Keyword)
		{
			return TSString2QString(data.Modifier);
		}
	}
	return "";
}

bool MYUpdateDataWidget::IsUpdate()
{
	if (_p->_ItemModel->rowCount())
	{
		MYCollaborativeEditingConstructor::Instance()->RevertVersion();
	}

	UpdateCEData();
	return !_p->_ItemModel->rowCount();
}

void MYUpdateDataWidget::SetRefreshCommitDataCallback(boost::function<void(void) > Callback)
{
	_p->_RefreshCommitCallback = Callback;
}

void MYUpdateDataWidget::OnPushButtonUpdateData()
{
	MYAppCenter::ShowProgress(0, 0, false);
	MYAppCenter::SetProgress(FromAscii("正在同步数据......"));

	MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>();
	MYIScenarioDataSourcePtr DataSource = QueryModuleT<MYIScenarioDataSource>();

	std::vector<TSHANDLE> UpdateHandles;
	std::vector<TSHANDLE> AddHandles;
	bool IsMissionUpdate = false;
	bool IsForceUpdate = false; 
	QStandardItem* IdentMgrItem = NULL;

	QList<QModelIndex> Indexs = ui->treeView->selectionModel()->selectedRows();

	if (!Indexs.size())
	{
		MYAppCenter::HideProgress();
		emit SigShowLog(FromAscii("请选择要更新的数据!"));
		return;
	}

	MYCollaborativeEditingConstructor::Instance()->StartUpdateData();

	std::vector<QStandardItem*> Items;

	for (int i = 0; i < Indexs.size(); ++i)
	{
		if (QStandardItem* Item = _p->_ItemModel->itemFromIndex(_p->_ProxyModel->mapToSource(Indexs[i])))
		{
			bool IsIdentMgrUpdate = false;

			std::vector<MYCEUpdateDataPtr> UpdateDatas = Item->data(UpdateDataRole).value<std::vector<MYCEUpdateDataPtr> >();

			for (int n = 0; n < UpdateDatas.size(); ++n)
			{
				TSInterObjectPtr IObject = UpdateDatas[n]->IObjs;
				TSTOPICHANDLE TopicHandle = UpdateDatas[n]->TopicHandle;

				if (TopicHandle == Think_Simulation_Scenario_DeployLayer
					|| TopicHandle == Think_Simulation_Scenario_BasicInfo)
				{
					continue;
				}

				if (UpdateDatas[n]->State == MYCEDataState::DS_Update
					|| UpdateDatas[n]->State == MYCEDataState::DS_Add)
				{
					if (MYHandleObjectPtr Object = TS_CAST(IObject, MYHandleObjectPtr))
					{
						if (MYCollaborativeCommon::Instance()->GetEntityInfo(Object->Handle, GetScenarioDomain())
							|| MYCollaborativeCommon::Instance()->GetSimResource(Object->Handle, GetScenarioDomain()))
						{
							if (std::find(UpdateHandles.begin(), UpdateHandles.end(), Object->Handle) == UpdateHandles.end())
							{
								UpdateHandles.push_back(Object->Handle);
							}
						}
						else
						{
							if (std::find(AddHandles.begin(), AddHandles.end(), Object->Handle) == AddHandles.end())
							{
								AddHandles.push_back(Object->Handle);
							}

							if (TS_CAST(IObject, MYLogicEntityInitInfoPtr) || TS_CAST(IObject, MYSimResourcePtr))
							{
								if (MYDeployLayerPtr Layer = MYCollaborativeCommon::Instance()->GetDeployLayerByHandle(Object->Handle))
								{
									Layer->LayerObjects.push_back(Object->Handle);

									GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_DeployLayer, Layer.get());

									DataHandler->AddHandleObject(Object, TopicHandle, false);
								}
							}
						}

						//如果是实体，更新Geo位置
						if (MYEntityInitInfoPtr EntityInitInfo = TS_CAST(Object, MYEntityInitInfoPtr))
						{
							MYEntityGeoStatePtr GeoState(new MYEntityGeoState);
							GeoState->Handle = Object->Handle;
							GeoState->GeoPos = EntityInitInfo->GeoPos;

							GetScenarioDomain()->UpdateTopic(Think_Simulation_Interaction_EntityGeoState, GeoState.get());
						}
					}

					if (MYMissionParamPtr MissionParam = TS_CAST(IObject, MYMissionParamPtr))
					{
						IsMissionUpdate = true;

						DeleteMissionParam(MissionParam->Handle, MissionParam->MissionId);
					}

					if (MYMissionSchedParamPtr MissionSched = TS_CAST(IObject, MYMissionSchedParamPtr))
					{
						IsMissionUpdate = true;

						DeleteMissionSchedParam(MissionSched->Handle, MissionSched->MissionId);
					}

					if (MYIdentManagerPtr TmpIdentMgr = TS_CAST(IObject, MYIdentManagerPtr))
					{
						IsIdentMgrUpdate = true;
						IdentMgrItem = Item;
						continue;
					}

					if (MYForceOrganizationPtr forceOrga = TS_CAST(IObject, MYForceOrganizationPtr))
					{
						if (MYIForceOrganizationDataSourcePtr forceDataSource = QueryModuleT<MYIForceOrganizationDataSource>())
						{
							forceDataSource->UpdateForceOrganization(forceOrga);
							IsForceUpdate = true;
						}
					}
					else if (MYVirtualForcePtr virforce = TS_CAST(IObject, MYVirtualForcePtr))
					{
						if (MYIForceOrganizationDataSourcePtr forceDataSource = QueryModuleT<MYIForceOrganizationDataSource>())
						{
							forceDataSource->AddVirtualForce(MYIForceOrganizationDataSource::Task_DataType, virforce);
							IsForceUpdate = true;
						}
					}

					GetScenarioDomain()->UpdateTopic(TopicHandle, IObject.get());
				}
				else if (UpdateDatas[n]->State == MYCEDataState::DS_Delete)
				{
					if (MYLogicEntityInitInfoPtr EntityInfo = TS_CAST(IObject, MYLogicEntityInitInfoPtr))
					{
						std::vector<TSHANDLE> HandlesForRemove;
						HandlesForRemove.push_back(EntityInfo->Handle);
						MYEventBus::Instance()->PostEventBlocking(
							SE_UI_HANDLE_OBJECT_BATCH_REMOVE, TSVariant::FromValue(HandlesForRemove));

						std::vector<TSHANDLE> HandlesForUpdate = DataHandler->RemoveHandleObject(EntityInfo->Handle, false);

						UpdateHandles.insert(UpdateHandles.end(), HandlesForUpdate.begin(), HandlesForUpdate.end());
					}
					else if (MYSimResourcePtr Resource = TS_CAST(IObject, MYSimResourcePtr))
					{
						std::vector<TSHANDLE> HandlesForRemove;
						HandlesForRemove.push_back(Resource->Handle);
						MYEventBus::Instance()->PostEventBlocking(
							SE_UI_HANDLE_OBJECT_BATCH_REMOVE, TSVariant::FromValue(HandlesForRemove));

						std::vector<TSHANDLE> HandlesForUpdate = DataHandler->RemoveHandleObject(Resource->Handle, false);
						UpdateHandles.insert(UpdateHandles.end(), HandlesForUpdate.begin(), HandlesForUpdate.end());
						
						if (!TS_INVALID_HANDLE_VALUE(Resource->MissionId))
						{
							TSHANDLE EntityHandle = GetMissionHandle(Resource->MissionId);

							if (std::find(UpdateHandles.begin(), UpdateHandles.end(), EntityHandle) == UpdateHandles.end())
							{
								UpdateHandles.push_back(EntityHandle);
							}
						}
					}
					else
					{
						if (MYMissionParamPtr MissionParam = TS_CAST(IObject, MYMissionParamPtr))
						{
							IsMissionUpdate = true;

							if (std::find(UpdateHandles.begin(), UpdateHandles.end(), MissionParam->Handle) == UpdateHandles.end())
							{
								UpdateHandles.push_back(MissionParam->Handle);
							}
						}
						// add
						else if (MYForceOrganizationPtr forceOrga = TS_CAST(IObject, MYForceOrganizationPtr))
						{
							//一般情况下，不存在删除NULL组的操作
						}
						else if (MYVirtualForcePtr virforce = TS_CAST(IObject, MYVirtualForcePtr))
						{
							//需要修改NULL组模块的删除接口，调用协同单例的DeleteTopic接口，才会进入这里
							if (MYIForceOrganizationDataSourcePtr forceDataSource = QueryModuleT<MYIForceOrganizationDataSource>())
							{
								forceDataSource->DeleteVirtualForce(MYIForceOrganizationDataSource::Task_DataType, virforce->DataUniquelyId);
								IsForceUpdate = true;
							}
						}

						GetScenarioDomain()->DeleteTopic(TopicHandle, IObject.get(), false);
					}
				}

				_p->_UpdateDataId[UpdateDatas[n]->Id] = UpdateDatas[n]->Id;

				MYCollaborativeEditingConstructor::Instance()->ClearCommitData(UpdateDatas[n]->IObjs, UpdateDatas[n]->TopicHandle);
			}

			DiffDataSolveWay SolveWay = Item->data(SolveWayRole).value<DiffDataSolveWay>();

			std::vector<MYCEDiffDataPtr> Datas = Item->data(DiffDataRole).value<std::vector<MYCEDiffDataPtr> >();

			for (int n = 0; n < Datas.size(); ++n)
			{
				if (SolveWay == DDS_UseRemote)
				{
					if (MYMissionParamPtr MissionParam = TS_CAST(Datas[n]->MimeIObj, MYMissionParamPtr))
					{
						DeleteMissionParam(MissionParam->Handle ,MissionParam->MissionId);
					}
					else if (MYMissionSchedParamPtr MissionSchedParam = TS_CAST(Datas[n]->MimeIObj, MYMissionSchedParamPtr))
					{
						DeleteMissionSchedParam(MissionSchedParam->Handle, MissionSchedParam->MissionId);
					}

					if (MYHandleObjectPtr Object = TS_CAST(Datas[n]->TheirIObj, MYHandleObjectPtr))
					{
						if (std::find(UpdateHandles.begin(), UpdateHandles.end(), Object->Handle) == UpdateHandles.end())
						{
							UpdateHandles.push_back(Object->Handle);
						}
					}

					GetScenarioDomain()->UpdateTopic(Datas[n]->ThierTopicHandle, Datas[n]->TheirIObj.get());
				}

				MYCollaborativeEditingConstructor::Instance()->ClearCommitData(Datas[n]->MimeIObj, Datas[n]->MimeTopicHandle);

				_p->_UpdateDataId[Datas[n]->Id] = Datas[n]->Id;
			}

			if (!IsIdentMgrUpdate)
			{
				Items.push_back(Item);
			}
		}
	}

	for (size_t i = 0; i < Items.size(); ++i)
	{
		_p->_ItemModel->removeRow(Items[i]->row());
	}

	if (AddHandles.size())
	{
		MYEventBus::Instance()->PostEventBlocking(SE_UI_HANDLE_OBJECT_BATCH_ADD, TSVariant::FromValue(AddHandles));
	}

	if (UpdateHandles.size())
	{
		MYEventBus::Instance()->PostEventBlocking(SE_UI_HANDLE_OBJECT_BATCH_UPDATE, TSVariant::FromValue(UpdateHandles));
	}

	MYCollaborativeEditingConstructor::Instance()->EndUpdateData();

	if (IsMissionUpdate && !UpdateHandles.size() && !AddHandles.size())
	{
		std::vector<TSHANDLE> Handles;
		MYMissionHandler::Instance()->UpdateAllEntityMissionParam(Handles);
	}

	if (IsForceUpdate)
	{
		//刷新编组树
		//MYEventBus::Instance()->Post(SE_UI_COLL_TASKORGA_REFRSEH);
		MYEventBus::Instance()->Post(SE_UI_TASKORGA_REFRSEH);
	}

	MYAppCenter::HideProgress();
	MYEventBus::Instance()->PostEventBlocking(SE_UI_VIEW_UPDATE);
	ui->pushButton_Update->setEnabled(_p->_ItemModel->rowCount());
	emit SigShowLog(FromAscii("协同组的想定数据同步成功!"));

	std::map<TSString, TSString> IdentNames;

	if (IdentMgrItem)
	{
		std::vector<MYCEUpdateDataPtr> UpdateDatas = IdentMgrItem->data(UpdateDataRole).value<std::vector<MYCEUpdateDataPtr> >();

		MYIdentManagerPtr RemoteIdentMgr;

		if (UpdateDatas.size())
		{
			RemoteIdentMgr = TS_CAST(UpdateDatas[0]->IObjs, MYIdentManagerPtr);
		}

		if (RemoteIdentMgr && GetDiffIdentNames(RemoteIdentMgr, IdentNames))
		{
			TSTopicFindSetType FindSet = GetScenarioDomain()->CreateTopicFindSet(Think_Simulation_MMF_LogicEntityInitInfo);

			while (TSITopicContextPtr EntityTopic = GetScenarioDomain()->GetNextTopic(FindSet))
			{
				if (MYEntityInitInfoPtr EntityInfo = EntityTopic->GetTopicT<MYEntityInitInfo>())
				{
					if (!IsEntityExistIdent(EntityInfo->Identification, IdentNames))
					{
						emit SigShowLog(FromAscii("协同组的标识管理缺少本地实体需要的标识方!"));

						return;
					}
				}
				else if (MYDecisionEntityInitInfoPtr DecisionEntityInfo = EntityTopic->GetTopicT<MYDecisionEntityInitInfo>())
				{
					if (!IsEntityExistIdent(DecisionEntityInfo->Identification, IdentNames))
					{
						emit SigShowLog(FromAscii("协同组的标识管理缺少本地实体需要的标识方!"));

						return;
					}
				}
			}
		}

		MYCollaborativeEditingConstructor::Instance()->StartUpdateData();

		GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_IdentMgr, RemoteIdentMgr.get());

		MYCollaborativeEditingConstructor::Instance()->EndUpdateData();
		UpdateEntityIdent(IdentNames);
		MYEventBus::Instance()->PostEventBlocking(SE_UI_IDENT_CHANGED);
		_p->_ItemModel->removeRow(IdentMgrItem->row());

		_p->_UpdateDataId[UpdateDatas[0]->Id] = UpdateDatas[0]->Id;
	}

	ui->pushButton_Update->setEnabled(_p->_ItemModel->rowCount());

	if (!_p->_ItemModel->rowCount())
	{
		_p->_UpdateDataId.clear();
	}

	if (_p->_RefreshCommitCallback)
	{
		_p->_RefreshCommitCallback();
	}
}

void MYUpdateDataWidget::OnCompareData()
{
	QList<QModelIndex> Indexs = ui->treeView->selectionModel()->selectedRows();

	if (Indexs.size() == 1)
	{
		if (QStandardItem *Item = _p->_ItemModel->itemFromIndex(_p->_ProxyModel->mapToSource(Indexs[0])))
		{
			std::vector<MYCEDiffDataPtr> Datas = Item->data(DiffDataRole).value<std::vector<MYCEDiffDataPtr> >();
			std::vector<MYCEDiffDataPtr> TmpDatas;

			for (size_t j = 0; j < Datas.size(); ++j)
			{
				if (Datas[j]->ThierTopicHandle == Think_Simulation_Interaction_EntityGeoState)
				{
					continue;
				}

				TmpDatas.push_back(Datas[j]);
			}

			if (TmpDatas.size())
			{
				MYDataCompareDialog Dlg(this);

				if (TmpDatas.size() == 1)
				{
					Dlg.SetCompareData(TmpDatas[0]);
				}
				else
				{
					Dlg.SetCompareData(TmpDatas);
				}

				Dlg.exec();
			}
		}
	}
}

void MYUpdateDataWidget::OnUseLocalData()
{
	QList<QModelIndex> Indexs = ui->treeView->selectionModel()->selectedRows();

	for (int i = 0; i < Indexs.size(); ++i)
	{
		if (QStandardItem *Item = _p->_ItemModel->itemFromIndex(_p->_ProxyModel->mapToSource(Indexs[i])))
		{
			Item->setData(QVariant::fromValue(DDS_UseLocal), SolveWayRole);
			Item->setData(QVariant::fromValue(true), IsSolveConflictRole);
		}
	}
}

void MYUpdateDataWidget::OnUseRemoteData()
{
	QList<QModelIndex> Indexs = ui->treeView->selectionModel()->selectedRows();

	for(int i = 0; i < Indexs.size(); ++i)
	{
		if (QStandardItem *Item = _p->_ItemModel->itemFromIndex(_p->_ProxyModel->mapToSource(Indexs[i])))
		{
			Item->setData(QVariant::fromValue(DDS_UseRemote), SolveWayRole);
			Item->setData(QVariant::fromValue(true), IsSolveConflictRole);
		}
	}
}

void MYUpdateDataWidget::OnTreeWidgetCustomContextMenuRequested(const QPoint & Point)
{
	QModelIndex Index = ui->treeView->indexAt(Point);

	if (Index != QModelIndex())
	{
		if (QStandardItem *Item = _p->_ItemModel->itemFromIndex(_p->_ProxyModel->mapToSource(Index)))
		{
			if (Item->data(ScenarioDataTypeRole).value<ScenarioDataType>() == SDT_Conflict
				&& !Item->data(IsSolveConflictRole).value<bool>())
			{
				_p->_Menu->exec(QCursor::pos());
			}
		}
	}
}

void MYUpdateDataWidget::OnPushButtonRefreshClicked()
{
	if (MYScenarioCEHelper::Instance()->GetCERoleType() == MYScenarioCEHelper::CER_Entrants
		&& !MYCollaborativeEditingConstructor::Instance()->IsOnLine())
	{
		MYUserMessageBox::warning(this, FromAscii("系统提示"), FromAscii("连接服务失败!"));

		return;
	}

	if (_p->_ItemModel->rowCount())
	{
		MYCollaborativeEditingConstructor::Instance()->RevertVersion();
	}

	MYAppCenter::ShowProgress(0, 0, true);
	MYAppCenter::SetProgress(FromAscii("正在刷新数据......"));

	UpdateCEData();

	MYAppCenter::HideProgress();
}

void MYUpdateDataWidget::filter(QString filterStr)
{
	QRegExp regExp = QRegExp(filterStr, Qt::CaseInsensitive);

	_p->_ProxyModel->setFilterRegExp(regExp);
}

QStandardItem* MYUpdateDataWidget::GetOrCreateItem(TSInterObjectPtr IObject, const TSTOPICHANDLE& TopicHandle, const UINT32& State)
{
	QStandardItem* LocalItem = NULL;

	TSTopicHelperPtr TopicHelper = TSTopicTypeManager::Instance()->GetTopicHelperByTopic(TopicHandle);

	if (TopicHelper->CanConvert(Think_Simulation_MMF_LogicEntityInitInfo))
	{
		LocalItem = GetOrCreateEntityItem(IObject, State);
	}
	else if (TopicHelper->CanConvert(Think_Simulation_MMF_ComponentInitInfo)
		|| TopicHelper->CanConvert(Think_Simulation_MMF_MissionParam)
		|| TopicHelper->CanConvert(Think_Simulation_MMF_MissionSchedParam)
		|| TopicHelper->CanConvert(Think_Simulation_Models_MotionPlan)
		|| TopicHelper->CanConvert(Think_Simulation_Interaction_EntityGeoState)
		|| TopicHelper->CanConvert(Think_Simulation_Formation))
	{
		if (MYHandleObjectPtr Object = TS_CAST(IObject, MYHandleObjectPtr))
		{
			std::map<TSHANDLE, QStandardItem*>::iterator It = _p->_EntityItems.find(Object->Handle);

			if (It != _p->_EntityItems.end())
			{
				LocalItem = It->second;
			}
			else
			{
				MYLogicEntityInitInfoPtr EntityInfo = MYCollaborativeCommon::Instance()->GetEntityInfo(Object->Handle, GetScenarioDomain());
				LocalItem = GetOrCreateEntityItem(EntityInfo,State);
			}

			if (LocalItem)
			{
				QString TipInfo = LocalItem->toolTip();

				if (!TipInfo.isEmpty())
				{
					TipInfo += "\n";
				}

				if (MYMissionParamPtr MissionParam = TS_CAST(Object, MYMissionParamPtr))
				{
					LocalItem->setToolTip(TipInfo + MYCollaborativeCommon::Instance()->GetMissionName(TSString2QString(MissionParam->MissionName)));
				}
				else if (MYMissionSchedParamPtr SchedParam = TS_CAST(Object, MYMissionSchedParamPtr))
				{
					LocalItem->setToolTip(TipInfo + MYMissionHandler::Instance()->GetSchedNameByTopicHandle(TopicHandle));
				}
				else if (MYComponentInitInfoPtr ComponentInfo = TS_CAST(Object, MYComponentInitInfoPtr))
				{
					LocalItem->setToolTip(TipInfo + TSString2QString(ComponentInfo->Name));
				}
				else if (MYMotionPlanPtr Motion = TS_CAST(Object, MYMotionPlanPtr))
				{
					LocalItem->setToolTip(TipInfo + FromAscii("机动计划"));
				}
				else if (MYFormationInfoPtr FormationInfo = TS_CAST(Object, MYFormationInfoPtr))
				{
					LocalItem->setToolTip(TipInfo
						+ TSString2QString(FormationInfo->FormationName) + "-" + LocalItem->text());
				}
			}
		}
	}
	else if (TopicHelper->CanConvert(Think_Simulation_SimResource))
	{
		LocalItem = GetOrCreateResourceItem(IObject, State);
	}
	else if(TopicHelper->CanConvert(Think_Simulation_MYForces_TaskOrganization)
		|| TopicHelper->CanConvert(Think_Simulation_MYForces_OperOrganization)
		|| TopicHelper->CanConvert(Think_Simulation_MYForces_VirtualForce))
	{
		LocalItem = GetOrCreateForceItem(IObject, State);
	}
	else if (TopicHelper->CanConvert(Think_Simulation_MYPlanData_ComprehendMission)
		|| TopicHelper->CanConvert(Think_Simulation_MYPlanData_SituationConclusion)
		|| TopicHelper->CanConvert(Think_Simulation_MYPlanData_FightPlan)
		|| TopicHelper->CanConvert(Think_Simulation_MYPlanData_CombatOrder)
		|| TopicHelper->CanConvert(GLTZ_Control_SetFunInfo)
		|| TopicHelper->CanConvert(Think_Simulation_MYPlanData_CombatStageTime))
	{
		LocalItem = GetOrCreatePlanItem(IObject, State);
	}
	else if (TopicHelper->CanConvert(Think_Simulation_MYCollaborativeData_CollaborativeDataModifierVec))
	{
		_p->_DataModifierVecPtr = TS_CAST(IObject, Think::Simulation::MYCollaborativeData::CollaborativeDataModifierVec::DataTypePtr);
	}

	return LocalItem;
}

QStandardItem* MYUpdateDataWidget::GetOrCreateEntityItem(TSInterObjectPtr LocalObj, const UINT32& State)
{
	QStandardItem* LocalItem = NULL;

	MYLogicEntityInitInfoPtr LocalEntity = TS_CAST(LocalObj, MYLogicEntityInitInfoPtr);

	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier;
	if (LocalEntity)
	{
		if (MYCollaborativeCommon::Instance()->GetEntityInfo(LocalEntity->Handle, GetScenarioDomain())
			&& State == MYCEDataState::DS_Add)
		{
			return LocalItem;
		}

		std::map<TSHANDLE, QStandardItem*>::iterator It = _p->_EntityItems.find(LocalEntity->Handle);

		if (It == _p->_EntityItems.end())
		{
			LocalItem = new QStandardItem(TSString2QString(LocalEntity->Name));
			LocalItem->setEditable(false);

			if (TS_CAST(LocalEntity, MYNetworkInitInfoPtr))
			{
				LocalItem->setIcon(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(
					FromAscii("XScenario/图片/网路链路.png")));
			}
			else
			{
				if (MYIIdentificationManagerPtr IdentMgr = QueryModuleT<MYIIdentificationManager>())
				{
					QColor Color = TSRGBA2QCOLOR(IdentMgr->GetIdentColor(LocalEntity->Identification));

					if (MYIIconManagerPtr IconMgr = QueryModuleT<MYIIconManager>())
					{
						if (MYIVectorIconPtr IconVector = IconMgr->GetIcon(
							TSString2QString(MYScenarioUtils::GetIcon2DName(LocalEntity->ModelName))))
						{
							LocalItem->setIcon(IconVector->CreateIcon(ICONSIZE, Color));
						}
					}
				}
			}

			_p->_EntityItems[LocalEntity->Handle] = LocalItem;
			_p->_ItemModel->appendRow(LocalItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(LocalItem->row(), 1, StateItem);
		}
		else
		{
			LocalItem = It->second;
		}
		DataModifier.TopicHandle = "Think_Simulation_MMF_LogicEntityInitInfo";
		DataModifier.Handle = LocalEntity->Handle;
	}

	_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;
	return LocalItem;
}

QStandardItem* MYUpdateDataWidget::GetOrCreateResourceItem(TSInterObjectPtr LocalObj, const UINT32& State)
{
	QStandardItem* LocalItem = NULL;

	MYSimResourcePtr LocalResource = TS_CAST(LocalObj, MYSimResourcePtr);

	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier;
	if (LocalResource)
	{
		if (MYCollaborativeCommon::Instance()->GetSimResource(LocalResource->Handle, GetScenarioDomain())
			&& State == MYCEDataState::DS_Add)
		{
			return LocalItem;
		}

		std::map<TSHANDLE, QStandardItem*>::iterator It = _p->_EntityItems.find(LocalResource->Handle);

		if (It == _p->_EntityItems.end())
		{
			LocalItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				TSString2QString(LocalResource->Name));
			LocalItem->setEditable(false);
			_p->_EntityItems[LocalResource->Handle] = LocalItem;
			_p->_ItemModel->appendRow(LocalItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(LocalItem->row(), 1, StateItem);
		}
		else
		{
			LocalItem = It->second;
		}
		DataModifier.TopicHandle = "Think_Simulation_SimResource";
		DataModifier.Handle = LocalResource->EntityHandle;
	}

	_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;
	return LocalItem;
}

QStandardItem* MYUpdateDataWidget::GetOrCreateForceItem(TSInterObjectPtr LocalObj, const UINT32& State)
{
	QStandardItem* LocalItem = NULL;

	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier;
	if (MYVirtualForcePtr virForce = TS_CAST(LocalObj, MYVirtualForcePtr))
	{
		std::map<std::string, QStandardItem*>::iterator It = _p->_ForceItems.find(virForce->DataUniquelyId);
		
		//在这里没有判断新增，是因为虚拟NULL与NULL组都不会显示新增，即使是新增显示的也是修改

		if (It == _p->_ForceItems.end())
		{
			LocalItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("编组数据_") + TSString2QString(virForce->Name));

			LocalItem->setEditable(false);
			_p->_ForceItems[virForce->DataUniquelyId] = LocalItem;

			_p->_ItemModel->appendRow(LocalItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(LocalItem->row(), 1, StateItem);
		}
		else
		{
			LocalItem = It->second;
		}
		DataModifier.TopicHandle = "Think_Simulation_MYForces_VirtualForce";
		DataModifier.Handle = virForce->BSEHandle;
		DataModifier.Keyword = virForce->DataUniquelyId;
	}
	else if (MYForceOrganizationPtr ForceOrganization = TS_CAST(LocalObj, MYForceOrganizationPtr))
	{
		std::map<std::string, QStandardItem*>::iterator It = _p->_ForceItems.find(ForceOrganization->Name);

		if (It == _p->_ForceItems.end())
		{
			LocalItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("编组数据_") + TSString2QString(ForceOrganization->Name));

			LocalItem->setEditable(false);
			_p->_ForceItems[ForceOrganization->Name] = LocalItem;

			_p->_ItemModel->appendRow(LocalItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(LocalItem->row(), 1, StateItem);
		}
		else
		{
			LocalItem = It->second;
		}
		DataModifier.TopicHandle = "Think_Simulation_MYForces_ForceOrganization";
		DataModifier.Keyword = ForceOrganization->Name;
	}

	_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;
	return LocalItem;
}

QStandardItem* MYUpdateDataWidget::GetOrCreatePlanItem(TSInterObjectPtr LocalObj, const UINT32& State)
{
	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier;
	if (Think::Simulation::MYPlanData::ComprehendMission::DataTypePtr PlanComprehendMissionData =
		TS_CAST(LocalObj, Think::Simulation::MYPlanData::ComprehendMission::DataTypePtr))
	{
		if (!_p->_PlanManagerRWItem)
		{
			_p->_PlanManagerRWItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-理解任务"));
			_p->_PlanManagerRWItem->setEditable(false);
			DataModifier.TopicHandle = "Think_Simulation_MYPlanData_ComprehendMission";
			_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;

			_p->_ItemModel->appendRow(_p->_PlanManagerRWItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerRWItem->row(), 1, StateItem);
		}
		return _p->_PlanManagerRWItem;
	}
	else if (Think::Simulation::MYPlanData::SituationConclusion::DataTypePtr PlanSituationConclusionData =
		TS_CAST(LocalObj, Think::Simulation::MYPlanData::SituationConclusion::DataTypePtr))
	{
		if (!_p->_PlanManagerFXPDQKItem)
		{
			_p->_PlanManagerFXPDQKItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-分析判断情况"));
			_p->_PlanManagerFXPDQKItem->setEditable(false);
			DataModifier.TopicHandle = "Think_Simulation_MYPlanData_SituationConclusion";
			_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;

			_p->_ItemModel->appendRow(_p->_PlanManagerFXPDQKItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerFXPDQKItem->row(), 1, StateItem);
		}
		return _p->_PlanManagerFXPDQKItem;
	}
	else if (Think::Simulation::MYPlanData::FightPlan::DataTypePtr PlanFightPlanData =
		TS_CAST(LocalObj, Think::Simulation::MYPlanData::FightPlan::DataTypePtr))
	{
		if (!_p->_PlanManagerDXZDJXItem)
		{
			_p->_PlanManagerDXZDJXItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-定下战斗决心"));
			_p->_PlanManagerDXZDJXItem->setEditable(false);
			DataModifier.TopicHandle = "Think_Simulation_MYPlanData_FightPlan";
			_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;

			_p->_ItemModel->appendRow(_p->_PlanManagerDXZDJXItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerDXZDJXItem->row(), 1, StateItem);
		}
		return _p->_PlanManagerDXZDJXItem;
	}
	else if (Think::Simulation::MYPlanData::CombatOrder::DataTypePtr PlanCombatOrderData =
		TS_CAST(LocalObj, Think::Simulation::MYPlanData::CombatOrder::DataTypePtr))
	{
		if (!_p->_PlanManagerXDZDMLItem)
		{
			_p->_PlanManagerXDZDMLItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-下达战斗命令"));
			_p->_PlanManagerXDZDMLItem->setEditable(false);
			DataModifier.TopicHandle = "Think_Simulation_MYPlanData_CombatOrder";
			_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;

			_p->_ItemModel->appendRow(_p->_PlanManagerXDZDMLItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerXDZDMLItem->row(), 1, StateItem);
		}
		return _p->_PlanManagerXDZDMLItem; 
	}
	else if (GLTZ::Control::SetFunInfo::DataTypePtr PlanCombatOrderData =
		TS_CAST(LocalObj, GLTZ::Control::SetFunInfo::DataTypePtr))
	{
		if (!_p->_PlanManagerQXFPItem)
		{
			_p->_PlanManagerQXFPItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-席位编辑权限"));
			_p->_PlanManagerQXFPItem->setEditable(false);
			DataModifier.TopicHandle = "GLTZ_Control_SetFunInfo";
			_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;

			_p->_ItemModel->appendRow(_p->_PlanManagerQXFPItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerQXFPItem->row(), 1, StateItem);
		}
		return _p->_PlanManagerQXFPItem;
	}
	else if (Think::Simulation::MYPlanData::CombatStageTime::DataTypePtr PlanCombatOrderData =
		TS_CAST(LocalObj, Think::Simulation::MYPlanData::CombatStageTime::DataTypePtr))
	{
		if (!_p->_PlanManagerJDHFItem)
		{
			_p->_PlanManagerJDHFItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-阶段划分"));
			_p->_PlanManagerJDHFItem->setEditable(false);
			DataModifier.TopicHandle = "Think_Simulation_MYPlanData_CombatStageTime";
			_p->_DataModifierInfoMap[_p->_DataModifierInfoMap.size()] = DataModifier;

			_p->_ItemModel->appendRow(_p->_PlanManagerJDHFItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerJDHFItem->row(), 1, StateItem);
		}
		return _p->_PlanManagerJDHFItem;
	}
	return NULL;
}
