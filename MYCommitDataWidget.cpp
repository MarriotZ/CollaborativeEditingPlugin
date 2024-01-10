#include <QStandardItemModel>
#include <QKeyEvent>
#include <QtnThinkHelper.h>

#include <MYUtil/MYUtil.h>
#include <MYUICommon/MYAppCenter.h>
#include <MYUICommon/MYIIconManager.h>
#include <MYUICommon/MYAppConfig.h>
#include <MYUICommon/MYAppManager.h>

#include <MYControls/MYModelBoxFilterWidget.h>
#include <MYControls/MYUserSortFilterProxyModel.h>

#include <MYEventDeclare/MYEventDeclare.h>
#include <MYUserAppSkeleton/MYUserMessageBox.h>
#include <MYScenarioLib/MYScenarioCEHelper.h>
#include <MYScenarioLib/MYIScenarioDataHandler.h>
#include <MYScenarioLib/MYCollaborativeEditingPRC.h>
#include <MYScenarioLib/MYMissionHandler.h>

#include <MYForceModels_Inter/MYForceModelsDeclare.h>

#include <MYBaseLib/MYIForceOrganizationDataSource.h>
#include <MYPlan_Inter/Defined.h>
#include <Tools_Inter/Defined.h>

#include "MYCommitDataWidget.h"
#include "MYDataCompareDialog.h"
#include "MYCollaborativeEditingPlugin_Decl.h"


#include "ui_MYCommitDataWidget.h"

Q_DECLARE_METATYPE(std::vector<MYCECommitDataPtr>);

struct MYCommitDataWidgetPrivate
{
	MYCommitDataWidgetPrivate()
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
		, _ModifierCommitDataPtr(NULL)
	{

	}

	QAction* _UseLocalAct;
	QAction* _UseRemoteAct;
	QAction* _CompareAct;
	QMenu* _Menu;
	MYUserSortFilterProxyModel *_ProxyModel;
	QStandardItemModel* _ItemModel;
	UINT32 _OperatorType;

	MYCollaborativeDataDelegate* _DataDelegate;
	std::map<TSHANDLE, QStandardItem*> _EntityItems;
	std::map<std::string, QStandardItem*> _ForceItems; //新增，针对NULL组的处理
	QStandardItem* _PlanManagerRWItem; //NC新增，筹划-理解任务
	QStandardItem* _PlanManagerFXPDQKItem; //NC新增，筹划-分析判断情况
	QStandardItem* _PlanManagerDXZDJXItem; //NC新增，筹划-定下战斗决心
	QStandardItem* _PlanManagerXDZDMLItem; //NC新增，筹划-下达战斗命令
	QStandardItem* _PlanManagerQXFPItem; //NC新增，筹划-席位权限
	QStandardItem* _PlanManagerJDHFItem; //NC新增，筹划-阶段划分

	boost::function<bool(void) > _IsUpdateCallback;

	//用户修改记录
	Think::Simulation::MYCollaborativeData::CollaborativeDataModifierVec::DataTypePtr _DataModifierVecPtr;
	MYCECommitDataPtr  _ModifierCommitDataPtr;
};

MYCommitDataWidget::MYCommitDataWidget( QWidget *parent /*= 0*/)
	: QWidget(parent)
	, _p(new MYCommitDataWidgetPrivate)
{
	ui = new Ui::MYCommitDataWidget();
	ui->setupUi(this);
	ui->pushButton_Commit->setEnabled(false);

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

	connect(ui->pushButton_Commit, SIGNAL(clicked()), this, SLOT(OnCommitData()));
	connect(ui->pushButton_Refresh, SIGNAL(clicked()), this, SLOT(OnPushButtonRefreshClicked()));

	_p->_DataDelegate = new MYCollaborativeDataDelegate(ui->treeView);
	_p->_DataDelegate->Init(_p->_ItemModel, _p->_ProxyModel);
	ui->treeView->setItemDelegate(_p->_DataDelegate);

	_p->_ItemModel->setHorizontalHeaderLabels(QStringList() << FromAscii("可提交数据") << FromAscii("状态"));
	ui->treeView->setColumnWidth(0, 250);
}

MYCommitDataWidget::~MYCommitDataWidget()
{
	delete _p;
	delete ui;
}

bool MYCommitDataWidget::Initialize()
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

	UpdateCommitData();

	return true;
}

bool MYCommitDataWidget::Cleanup()
{
	_p->_PlanManagerRWItem = NULL;
	_p->_PlanManagerFXPDQKItem = NULL;
	_p->_PlanManagerDXZDJXItem = NULL;
	_p->_PlanManagerXDZDMLItem = NULL;
	_p->_PlanManagerQXFPItem = NULL;
	_p->_PlanManagerJDHFItem = NULL;

	if (_p->_DataModifierVecPtr)
	{
		_p->_DataModifierVecPtr->DataModifierVec.clear();
	}
	_p->_EntityItems.clear();
	_p->_ForceItems.clear();
	_p->_ItemModel->clear();
	_p->_ProxyModel->clear();
	_p->_ItemModel->setHorizontalHeaderLabels(QStringList() << FromAscii("可提交数据") << FromAscii("状态"));
	ui->treeView->setColumnWidth(0, 250);
	return true;
}

void MYCommitDataWidget::SetIsUpdateCallback(boost::function<bool(void) > Callback)
{
	_p->_IsUpdateCallback = Callback;
}

void MYCommitDataWidget::keyPressEvent(QKeyEvent *event)
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

void MYCommitDataWidget::keyReleaseEvent(QKeyEvent *event)
{
	QWidget::keyReleaseEvent(event);
	ui->treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MYCommitDataWidget::OnCommitData()
{
	if (MYScenarioCEHelper::Instance()->GetCERoleType() == MYScenarioCEHelper::CER_Entrants
		&& !MYCollaborativeEditingConstructor::Instance()->IsOnLine())
	{
		MYUserMessageBox::warning(this, FromAscii("系统提示"), FromAscii("连接服务失败!"));

		return;
	}

	if (!_p->_IsUpdateCallback() || !MYCollaborativeEditingConstructor::Instance()->IsLatestVersion())
	{
		MYUserMessageBox::warning(this, FromAscii("系统提示"),
			FromAscii("请先同步更新数据!"));

		return;
	}

	MYAppCenter::ShowProgress(0, 0, false);
	MYAppCenter::SetProgress(FromAscii("正在提交数据......"));

	std::vector<MYCEDataInfo> Datas;

	//提交用户修改记录数据
	if (_p->_DataModifierVecPtr)
	{
		TSITopicContextPtr Ctx = GetScenarioDomain()->UpdateTopic(
			Think_Simulation_MYCollaborativeData_CollaborativeDataModifierVec, _p->_DataModifierVecPtr.get());
		if (Ctx)
		{
			Datas.push_back(MYCollaborativeEditingConstructor::Instance()->ToCEDataInfo(Ctx));
		}
	}

	QList<QModelIndex> Indexs = ui->treeView->selectionModel()->selectedRows();
	
	if (!Indexs.size())
	{
		MYAppCenter::HideProgress();
		emit SigShowLog(FromAscii("请选择要提交的数据!"));
		return;
	}

	std::vector<QStandardItem*> Items;

	for (int i = 0; i < Indexs.size(); ++i)
	{
		if (QStandardItem* Item = _p->_ItemModel->itemFromIndex(_p->_ProxyModel->mapToSource(Indexs[i])))
		{
			std::vector<MYCECommitDataPtr> CommitDatas = Item->data(CommitDataRole).value<std::vector<MYCECommitDataPtr> >();

			for (int j = 0; j < CommitDatas.size(); ++j)
			{
				Datas.push_back(CommitDatas[j]->DataInfo);
			}

			Items.push_back(Item);
		}
	}

	for (size_t i = 0; i < Items.size(); ++i)
	{
		_p->_ItemModel->removeRow(Items[i]->row());
	}

	if (Datas.size())
	{
		MYCollaborativeEditingConstructor::Instance()->Commit(Datas);
	}

	MYAppCenter::HideProgress();
	emit SigShowLog(FromAscii("我的想定数据提交成功!"));
	ui->pushButton_Commit->setEnabled(_p->_ItemModel->rowCount());
}

void MYCommitDataWidget::OnPushButtonRefreshClicked()
{
	if (MYScenarioCEHelper::Instance()->GetCERoleType() == MYScenarioCEHelper::CER_Entrants
		&& !MYCollaborativeEditingConstructor::Instance()->IsOnLine())
	{
		MYUserMessageBox::warning(this, FromAscii("系统提示"), FromAscii("连接服务失败!"));

		return;
	}

	MYAppCenter::ShowProgress(0, 0, false);
	MYAppCenter::SetProgress(FromAscii("正在获取数据..."));

	UpdateCommitData();

	MYAppCenter::HideProgress();
}

void MYCommitDataWidget::OnCheckBoxAllSelectStateChanged(int state)
{
	disconnect(_p->_ItemModel, SIGNAL(itemChanged(QStandardItem *)),this,
		SLOT(OnItemModelItemChanged(QStandardItem *)));

	for (int i = 0; i < _p->_ItemModel->rowCount(); ++i)
	{
		QStandardItem* Item = _p->_ItemModel->item(i);
		Item->setCheckState((Qt::CheckState)state);

		for (int j = 0; j < Item->rowCount(); ++j)
		{
			Item->child(j)->setCheckState((Qt::CheckState)state);
		}
	}

	connect(_p->_ItemModel, SIGNAL(itemChanged(QStandardItem *)),this,
		SLOT(OnItemModelItemChanged(QStandardItem *)));
}

void MYCommitDataWidget::OnItemModelItemChanged(QStandardItem* Item)
{
	disconnect(_p->_ItemModel, SIGNAL(itemChanged(QStandardItem *)), this,
		SLOT(OnItemModelItemChanged(QStandardItem *)));

	if (Item->hasChildren())
	{
		for (int i = 0; i < Item->rowCount(); ++i)
		{
			Item->child(i)->setCheckState(Item->checkState());
		}
	}

	QList<QModelIndex> Indexs = ui->treeView->selectionModel()->selectedRows();

	if (Indexs.size() > 1)
	{
		for (int i = 0; i < Indexs.size(); ++i)
		{
			if (QStandardItem* SelectedItem = _p->_ItemModel->itemFromIndex(_p->_ProxyModel->mapToSource(Indexs[i])))
			{
				SelectedItem->setCheckState(Item->checkState());
			}
		}
	}

	connect(_p->_ItemModel, SIGNAL(itemChanged(QStandardItem *)), this,
		SLOT(OnItemModelItemChanged(QStandardItem *)));
}

void MYCommitDataWidget::filter(QString filterStr)
{
	QRegExp regExp = QRegExp(filterStr, Qt::CaseInsensitive);

	_p->_ProxyModel->setFilterRegExp(regExp);
}

QStandardItem* MYCommitDataWidget::GetOrCreateEntityItem(TSITopicContextPtr LocalCtx)
{
	QStandardItem* LocalItem = NULL;

	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType CollaborativeData;
	if (LocalCtx)
	{
		if (MYLogicEntityInitInfoPtr EntityInfo = LocalCtx->GetTopicT<MYLogicEntityInitInfo>())
		{
			std::map<TSHANDLE, QStandardItem*>::iterator It = _p->_EntityItems.find(EntityInfo->Handle);

			if (It == _p->_EntityItems.end())
			{
				LocalItem = new QStandardItem(TSString2QString(EntityInfo->Name));
				LocalItem->setEditable(false);

				if (LocalCtx->Is(Think_Simulation_Models_NetworkInitInfo))
				{
					LocalItem->setIcon(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(
							FromAscii("XScenario/图片/网路链路.png")));
				}
				else
				{
					if (MYIIdentificationManagerPtr IdentMgr = QueryModuleT<MYIIdentificationManager>())
					{
						QColor Color = TSRGBA2QCOLOR(IdentMgr->GetIdentColor(EntityInfo->Identification));

						if (MYIIconManagerPtr IconMgr = QueryModuleT<MYIIconManager>())
						{
							if (MYIVectorIconPtr IconVector = IconMgr->GetIcon(TSString2QString(MYScenarioUtils::GetIcon2DName(EntityInfo->ModelName))))
							{
								LocalItem->setIcon(IconVector->CreateIcon(ICONSIZE, Color));
							}
						}
					}
				}
				
				_p->_EntityItems[EntityInfo->Handle] = LocalItem;
				_p->_ItemModel->appendRow(LocalItem);

				QStandardItem* StateItem  = new QStandardItem(FromAscii("修改"));
				StateItem->setEditable(false);
				_p->_ItemModel->setItem(LocalItem->row(), 1, StateItem);

			}
			else
			{
				LocalItem = It->second;
			}
			CollaborativeData.TopicHandle = "Think_Simulation_MMF_LogicEntityInitInfo";
			CollaborativeData.Handle = EntityInfo->Handle;
		}
	}
	AddDataModifierVec(CollaborativeData);
	return LocalItem;
}

QStandardItem* MYCommitDataWidget::GetOrCreateResourceItem(TSITopicContextPtr LocalCtx)
{
	QStandardItem* LocalItem = NULL;

	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType CollaborativeData;
	if (MYSimResourcePtr Resource = LocalCtx->GetTopicT<MYSimResource>())
	{
		std::map<TSHANDLE, QStandardItem*>::iterator It = _p->_EntityItems.find(Resource->Handle);

		if (It == _p->_EntityItems.end())
		{
			LocalItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				TSString2QString(Resource->Name));

			LocalItem->setEditable(false);
			_p->_EntityItems[Resource->Handle] = LocalItem;

			_p->_ItemModel->appendRow(LocalItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(LocalItem->row(), 1, StateItem);
		}
		else
		{
			LocalItem = It->second;
		}
		CollaborativeData.TopicHandle = "Think_Simulation_SimResource";
		CollaborativeData.Handle = Resource->EntityHandle;
	}

	AddDataModifierVec(CollaborativeData);
	return LocalItem;
}

QStandardItem* MYCommitDataWidget::GetOrCreateForceItem(TSITopicContextPtr LocalCtx)
{
	QStandardItem* LocalItem = NULL;

	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType CollaborativeData;
	if (MYVirtualForcePtr virForce = LocalCtx->GetTopicT<MYVirtualForce>())
	{
		//判断是不是编组数据，如果是编组数据，才可以提交，避免提交了编成的虚拟NULL
		MYIForceOrganizationDataSourcePtr dataSource = QueryModuleT<MYIForceOrganizationDataSource>();
		if (dataSource)
		{
			std::vector<MYForceOrganizationPtr> orgaVec = dataSource->GetAllForceOrganization(MYIForceOrganizationDataSource::Task_DataType);
			bool isTaskVirforce = false;
			for (auto & iter : orgaVec)
			{
				if (iter)
				{
					if (iter->Name == virForce->OrganizationName)
					{
						isTaskVirforce = true;
					}
				}
			}
			if (!isTaskVirforce)
			{
				return NULL;
			}
		}
		std::map<std::string, QStandardItem*>::iterator It = _p->_ForceItems.find(virForce->DataUniquelyId);

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

		CollaborativeData.TopicHandle = "Think_Simulation_MYForces_VirtualForce";
		CollaborativeData.Handle = virForce->BSEHandle;
		CollaborativeData.Keyword = virForce->DataUniquelyId;
	}
	else if (MYForceOrganizationPtr ForceOrganization = LocalCtx->GetTopicT<MYForceOrganization>())
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
		CollaborativeData.TopicHandle = "Think_Simulation_MYForces_ForceOrganization";
		CollaborativeData.Keyword = ForceOrganization->Name;
	}

	AddDataModifierVec(CollaborativeData);
	return LocalItem;
}

QStandardItem* MYCommitDataWidget::GetOrCreatePlanItem(TSITopicContextPtr LocalCtx)
{
	Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType CollaborativeData;
	if (Think::Simulation::MYPlanData::ComprehendMission::DataTypePtr PlanComprehendMissionData =
		LocalCtx->GetTopicT<Think::Simulation::MYPlanData::ComprehendMission::DataType>())
	{
		if (!_p->_PlanManagerRWItem)
		{
			_p->_PlanManagerRWItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-理解任务"));
			_p->_PlanManagerRWItem->setEditable(false);

			_p->_ItemModel->appendRow(_p->_PlanManagerRWItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerRWItem->row(), 1, StateItem);
		}
		CollaborativeData.TopicHandle = "Think_Simulation_MYPlanData_ComprehendMission";
		AddDataModifierVec(CollaborativeData);
		return _p->_PlanManagerRWItem;
	}
	else if (Think::Simulation::MYPlanData::SituationConclusion::DataTypePtr PlanSituationConclusionData =
		LocalCtx->GetTopicT<Think::Simulation::MYPlanData::SituationConclusion::DataType>())
	{
		if (!_p->_PlanManagerFXPDQKItem)
		{
			_p->_PlanManagerFXPDQKItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-分析判断情况"));
			_p->_PlanManagerFXPDQKItem->setEditable(false);

			_p->_ItemModel->appendRow(_p->_PlanManagerFXPDQKItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerFXPDQKItem->row(), 1, StateItem);
		}
		CollaborativeData.TopicHandle = "Think_Simulation_MYPlanData_SituationConclusion";
		AddDataModifierVec(CollaborativeData);
		return _p->_PlanManagerFXPDQKItem;
	}
	else if (Think::Simulation::MYPlanData::FightPlan::DataTypePtr PlanFightPlanData =
		LocalCtx->GetTopicT<Think::Simulation::MYPlanData::FightPlan::DataType>())
	{
		if (!_p->_PlanManagerDXZDJXItem)
		{
			_p->_PlanManagerDXZDJXItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-定下战斗决心"));
			_p->_PlanManagerDXZDJXItem->setEditable(false);

			_p->_ItemModel->appendRow(_p->_PlanManagerDXZDJXItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerDXZDJXItem->row(), 1, StateItem);
		}
		CollaborativeData.TopicHandle = "Think_Simulation_MYPlanData_FightPlan";
		AddDataModifierVec(CollaborativeData);
		return _p->_PlanManagerDXZDJXItem;
	}
	else if (Think::Simulation::MYPlanData::CombatOrder::DataTypePtr PlanCombatOrderData =
		LocalCtx->GetTopicT<Think::Simulation::MYPlanData::CombatOrder::DataType>())
	{
		if (!_p->_PlanManagerXDZDMLItem)
		{
			_p->_PlanManagerXDZDMLItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-下达战斗命令"));
			_p->_PlanManagerXDZDMLItem->setEditable(false);

			_p->_ItemModel->appendRow(_p->_PlanManagerXDZDMLItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerXDZDMLItem->row(), 1, StateItem);
		}
		CollaborativeData.TopicHandle = "Think_Simulation_MYPlanData_CombatOrder";
		AddDataModifierVec(CollaborativeData);
		return _p->_PlanManagerXDZDMLItem;
	}
	else if (GLTZ::Control::SetFunInfo::DataTypePtr PlanCombatOrderData =
		LocalCtx->GetTopicT<GLTZ::Control::SetFunInfo::DataType>())
	{
		if (!_p->_PlanManagerQXFPItem)
		{
			_p->_PlanManagerQXFPItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-席位编辑权限"));
			_p->_PlanManagerQXFPItem->setEditable(false);

			_p->_ItemModel->appendRow(_p->_PlanManagerQXFPItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerQXFPItem->row(), 1, StateItem);
		}
		CollaborativeData.TopicHandle = "GLTZ_Control_SetFunInfo";
		AddDataModifierVec(CollaborativeData);
		return _p->_PlanManagerQXFPItem;
	}
	else if (Think::Simulation::MYPlanData::CombatStageTime::DataTypePtr PlanCombatOrderData =
		LocalCtx->GetTopicT<Think::Simulation::MYPlanData::CombatStageTime::DataType>())
	{
		if (!_p->_PlanManagerJDHFItem)
		{
			_p->_PlanManagerJDHFItem = new QStandardItem(QtnThinkThemeServices::Instance()->GetThemeResourceIcon(FromAscii("资源.png")),
				FromAscii("筹划-阶段划分"));
			_p->_PlanManagerJDHFItem->setEditable(false);

			_p->_ItemModel->appendRow(_p->_PlanManagerJDHFItem);

			QStandardItem* StateItem = new QStandardItem(FromAscii("修改"));
			StateItem->setEditable(false);
			_p->_ItemModel->setItem(_p->_PlanManagerJDHFItem->row(), 1, StateItem);
		}
		CollaborativeData.TopicHandle = "Think_Simulation_MYPlanData_CombatStageTime";
		AddDataModifierVec(CollaborativeData);
		return _p->_PlanManagerJDHFItem;
	}
	return NULL;
}

void MYCommitDataWidget::UpdateCommitData()
{
	Cleanup();

	if (MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>())
	{
		std::vector<MYCECommitDataPtr> Datas = MYCollaborativeEditingConstructor::Instance()->GetCommitData();

		for (size_t i = 0; i < Datas.size(); ++i)
		{
			if (TSTopicContextPtr Ctx = Datas[i]->Ctx)
			{
				QStandardItem* LocalItem = NULL;

				QString StateText = FromAscii("修改");

				switch (Datas[i]->DataInfo.State)
				{
				case MYCEDataState::DS_Delete:
					StateText = FromAscii("删除");
					break;
				case MYCEDataState::DS_Add:
					StateText = FromAscii("新增");
					break;
				case MYCEDataState::DS_Update:
					StateText = FromAscii("修改");
					break;
				default:
					break;
				}
				
				//目前只能协同编辑固定的主题,如果要能协同编辑其他主题，要改这里

				if (Ctx->Is(Think_Simulation_MMF_LogicEntityInitInfo))
				{
					if (LocalItem = GetOrCreateEntityItem(Ctx))
					{
						if (QStandardItem* StateItem = _p->_ItemModel->item(LocalItem->row(), 1))
						{
							StateItem->setText(StateText);
						}
					}
				}
				else if (Ctx->Is(Think_Simulation_MMF_ComponentInitInfo)
					|| Ctx->Is(Think_Simulation_MMF_MissionParam)
					|| Ctx->Is(Think_Simulation_MMF_MissionSchedParam)
					|| Ctx->Is(Think_Simulation_Models_MotionPlan)
					|| Ctx->Is(Think_Simulation_Interaction_EntityGeoState)
					|| Ctx->Is(Think_Simulation_Formation))
				{
					if (MYHandleObjectPtr Object = Ctx->GetTopicT<MYHandleObject>())
					{
						if (LocalItem = GetOrCreateEntityItem(DataHandler->GetLogicObjectContext(Object->Handle)))
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
								LocalItem->setToolTip(TipInfo + MYMissionHandler::Instance()->GetSchedNameByTopicHandle(Ctx->GetDataTopicHandle()));
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
				else if (Ctx->Is(Think_Simulation_SimResource))
				{
					if (LocalItem = GetOrCreateResourceItem(Ctx))
					{
						if (QStandardItem* StateItem = _p->_ItemModel->item(LocalItem->row(), 1))
						{
							StateItem->setText(StateText);
						}
					}
				}
				else if (Ctx->Is(Think_Simulation_MYPlanData_ComprehendMission)
				|| Ctx->Is(Think_Simulation_MYPlanData_SituationConclusion)
				|| Ctx->Is(Think_Simulation_MYPlanData_FightPlan)
				|| Ctx->Is(Think_Simulation_MYPlanData_CombatOrder)
				|| Ctx->Is(GLTZ_Control_SetFunInfo)
				|| Ctx->Is(Think_Simulation_MYPlanData_CombatStageTime))
				{
					if (LocalItem = GetOrCreatePlanItem(Ctx))
					{
						if (QStandardItem* StateItem = _p->_ItemModel->item(LocalItem->row(), 1))
						{
							StateItem->setText(StateText);
						}
					}
				}


				TSString c_IncludeTopic = GetAppConfig<TSString>("Root.CollaborativeEditing.IncludeTopic", "Think_Simulation_MYForces_TaskOrganization,Think_Simulation_MYForces_VirtualForce");

				std::vector<TSString> m_includeVec = TSStringUtil::Split(c_IncludeTopic, ",");
				for (auto & iter : m_includeVec)
				{
					TSTOPICHANDLE m_TopicHandle = GetTopicByName(iter);
					if (!TS_INVALID_HANDLE_VALUE(m_TopicHandle))
					{
						if (Ctx->Is(m_TopicHandle))
						{
							if (LocalItem = GetOrCreateForceItem(Ctx))
							{
								if (QStandardItem* StateItem = _p->_ItemModel->item(LocalItem->row(), 1))
								{
									StateItem->setText(StateText);
								}
							}
						}
					}
				}

				if (LocalItem)
				{
					std::vector<MYCECommitDataPtr> TopicCtxs =
						LocalItem->data(CommitDataRole).value<std::vector<MYCECommitDataPtr> >();

					TopicCtxs.push_back(Datas[i]);
					LocalItem->setData(QVariant::fromValue(TopicCtxs), CommitDataRole);
				}
			}
		}
	}

	ui->pushButton_Commit->setEnabled((bool)_p->_ItemModel->rowCount());
}

void MYCommitDataWidget::AddDataModifierVec(Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType DataModifier)
{
	DataModifier.Modifier = GetAppConfig<TSString>("Root.NC.SeatTypeName") + "(" + GetAppConfig<TSString>("Root.SeatLogin.SeatName") + ")";
	if (_p->_DataModifierVecPtr)
	{
		bool IsExist = false;
		for (int i = 0; i < _p->_DataModifierVecPtr->DataModifierVec.size(); ++i)
		{
			Think::Simulation::MYCollaborativeData::CollaborativeDataModifier::DataType data = _p->_DataModifierVecPtr->DataModifierVec[i];
			if (data.TopicHandle == DataModifier.TopicHandle && data.Handle == DataModifier.Handle
				&& data.Keyword == DataModifier.Keyword)
			{
				IsExist = true;
				if (data.Modifier != DataModifier.Modifier)
				{
					_p->_DataModifierVecPtr->DataModifierVec.erase(_p->_DataModifierVecPtr->DataModifierVec.begin() + i);
					_p->_DataModifierVecPtr->DataModifierVec.push_back(DataModifier);
				}
				break;
			}
		}
		if (!IsExist)
		{
			_p->_DataModifierVecPtr->DataModifierVec.push_back(DataModifier);
		}
	}
	else
	{
		bool isExist = false;
		if (TSITopicContextPtr Ctx = GetScenarioDomain()->GetFirstTopicByHandle(Think_Simulation_MYCollaborativeData_CollaborativeDataModifierVec))
		{
			if (_p->_DataModifierVecPtr = Ctx->GetTopicT<Think::Simulation::MYCollaborativeData::CollaborativeDataModifierVec::DataType>())
			{
				isExist = true;
			}
		}
		if (!isExist)
		{
			_p->_DataModifierVecPtr = boost::make_shared<Think::Simulation::MYCollaborativeData::CollaborativeDataModifierVec::DataType>();
		}
		_p->_DataModifierVecPtr->DataModifierVec.push_back(DataModifier);
	}
}
