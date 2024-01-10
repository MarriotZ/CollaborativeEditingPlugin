#include <MYUICommon/MYEventDecl.h>
#include <MYUICommon/MYIPluginManager.h>
#include <MYUICommon/MYISceneCreator.h>
#include <MYUICommon/MYAppConfig.h>

#include <MYSceneCommon/MYSceneCommon.h>
#include <MYSceneCommon/MYIPresentationScene.h>
#include <MYSceneCommon/MYIPresentationWorld.h>
#include <MYSceneCommon/MYPresentationDriver.h>
#include <MYSceneCommon/MYIPresentationViewport.h>

#include <MYEventDeclare/MYEventDeclare.h>
#include <MYScenarioLib/MYScenarioEventDecl.h>
#include <MYScenarioLib/MYSceneScenarioAbout.h>
#include <MYScenarioLib/MYIScenarioDataHandler.h>
#include <MYScenarioLib/MYScenarioCEHelper.h>
#include <MYScenarioLib/MYIEngineManager.h>
#include <MYScenarioLib/MYScenarioMgrCenterViewer.h>
#include <MYScenarioLib/MYMissionHandler.h>
#include <MYScenarioLib/MYCollaborativeEditingPRC.h>

#include "MYCollaborativeEditingPlugin_Decl.h"

SINGLETON_IMPLEMENT(MYCollaborativeCommon);

MYDeployLayerPtr MYCollaborativeCommon::GetDeployLayerByHandle(const TSHANDLE& Handle)
{
	TSTopicFindSetType SetType = GetScenarioDomain()->CreateTopicFindSet(Think_Simulation_Scenario_DeployLayer);

	while (TSITopicContextPtr Ctx = GetScenarioDomain()->GetNextTopic(SetType))
	{
		if (MYDeployLayerPtr LocalLayer = Ctx->GetTopicT<MYDeployLayer>())
		{
			if ((LocalLayer->StartHandleIndex <= Handle)
				&& (LocalLayer->EndHandleIndex >= Handle))
			{
				return LocalLayer;
			}
		}
	}

	return MYDeployLayerPtr();
}

QString MYCollaborativeCommon::GetMissionName(QString MissionName)
{
	QString Str = MissionName;

	int Index = MissionName.lastIndexOf("_");

	if (Index > 0)
	{
		Str = MissionName.mid(0, Index);
	}

	return Str;
}

MYLogicEntityInitInfoPtr MYCollaborativeCommon::GetEntityInfo(const TSHANDLE& Handle, TSDomainPtr Domain)
{
	if (TSITopicContextPtr Ctx = Domain->GetFirstTopicByHandle(Handle, Think_Simulation_MMF_LogicEntityInitInfo))
	{
		return Ctx->GetTopicT<MYLogicEntityInitInfo>();
	}

	return MYLogicEntityInitInfoPtr();
}

MYSimResourcePtr MYCollaborativeCommon::GetSimResource(const TSHANDLE& Handle, TSDomainPtr Domain)
{
	if (TSITopicContextPtr Ctx = Domain->GetFirstTopicByHandle(Handle, Think_Simulation_SimResource))
	{
		return Ctx->GetTopicT<MYSimResource>();
	}

	return MYSimResourcePtr();
}

void OnDataUpdateCallback(const std::vector<MYCEUpdateDataPtr>& UpdateDatas)
{
	if (MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>())
	{
		MYScenarioBasicInfoPtr ScenarioInfo = DataHandler->GetScenarioBasicInfo();
		ScenarioInfo->LayerNameList.clear();

		for (std::size_t i = 0; i < UpdateDatas.size(); ++i)
		{
			if (UpdateDatas[i]->State == MYCEDataState::DS_Update
				|| UpdateDatas[i]->State == MYCEDataState::DS_Add)
			{
				//如果更新的是基本信息，则过滤了
				if (UpdateDatas[i]->TopicHandle == Think_Simulation_Scenario_BasicInfo)
				{
					continue;
				}

				if (UpdateDatas[i]->TopicHandle == Think_Simulation_Scenario_DeployLayer)
				{
					if (MYDeployLayerPtr DeployLayer = TS_CAST(UpdateDatas[i]->IObjs, MYDeployLayerPtr))
					{
						DeployLayer->LayerObjects.clear();

						GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_DeployLayer, DeployLayer.get());

						ScenarioInfo->LayerNameList.push_back(DeployLayer->Name);
					}
				}
			}
		}

		GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_BasicInfo, ScenarioInfo.get());

		std::vector<TSHANDLE> Handles;

		for (std::size_t i = 0; i < UpdateDatas.size(); ++i)
		{
			if (UpdateDatas[i]->State == MYCEDataState::DS_Update
				|| UpdateDatas[i]->State == MYCEDataState::DS_Add)
			{
				if (UpdateDatas[i]->TopicHandle == Think_Simulation_Scenario_BasicInfo
					|| UpdateDatas[i]->TopicHandle == Think_Simulation_Scenario_DeployLayer)
				{
					continue;
				}

				if (MYHandleObjectPtr HandleObject = TS_CAST(UpdateDatas[i]->IObjs, MYHandleObjectPtr))
				{
					if (TS_CAST(HandleObject, MYLogicEntityInitInfoPtr) || TS_CAST(HandleObject, MYSimResourcePtr))
					{
						if (MYDeployLayerPtr Layer = MYCollaborativeCommon::Instance()->GetDeployLayerByHandle(HandleObject->Handle))
						{
							Layer->LayerObjects.push_back(HandleObject->Handle);
							GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_DeployLayer, Layer.get());
						}

						GetScenarioDomain()->UpdateTopic(UpdateDatas[i]->TopicHandle, HandleObject.get());
						Handles.push_back(HandleObject->Handle);
						DataHandler->AddHandleObject(HandleObject, UpdateDatas[i]->TopicHandle, false);
					}
					else
					{
						GetScenarioDomain()->UpdateTopic(UpdateDatas[i]->TopicHandle, UpdateDatas[i]->IObjs.get());
					}
				}
				else
				{
					GetScenarioDomain()->UpdateTopic(UpdateDatas[i]->TopicHandle, UpdateDatas[i]->IObjs.get());
				}
			}
		}

		if (Handles.size())
		{
			MYEventBus::Instance()->PostEventBlocking(SE_UI_HANDLE_OBJECT_BATCH_ADD, TSVariant::FromValue(Handles));
		}
	}
}

bool MYCollaborativeCommon::UpdateScenarioDataToLocal(MYScenarioBasicInfoPtr BasicInfo, MYDeployLayerPtr Layer, MYIdentManagerPtr IdentManager)
{
	MYIScenarioDataSourcePtr DataSource = QueryModuleT<MYIScenarioDataSource>();
	MYIScenarioDataHandlerPtr DataHandler = QueryModuleT<MYIScenarioDataHandler>();
	MYIIdentificationManagerPtr IdentMgr = QueryModuleT<MYIIdentificationManager>();
	MYISceneCreatorPtr SceneCreator = QueryModuleT<MYISceneCreator>();
	MYIPluginManagerPtr PluginMgr = QueryModuleT<MYIPluginManager>();

	if (DataSource && DataHandler && IdentMgr && SceneCreator && PluginMgr)
	{
		PluginMgr->DestroyWidgetPlugins();

		SceneCreator->DestroyScenes();
		DataHandler->ResetDomain();

		TSString DomainId = GetAppConfig<TSString>("Root.ScenarioEdit.DomainId", "150");

		MYDefaultNameAllocator::Instance()->Init();

		if (MYStudyInfoPtr StudyInfo = DataSource->GetStudyInfo(DataSource->GetContextStudyId()))
		{
			DataHandler->CreateDomain(DomainId, BasicInfo->Name);
			SceneCreator->CreateScenes(GetScenarioDomain());
			PluginMgr->CreateWidgetPlugins(GetScenarioDomain());

			MYEventBus::Instance()->PostEventBlocking(SE_UI_APP_PLUGINS_INITED);
			MYEventBus::Instance()->PostEventBlocking(SE_UI_SCENARIO_LOAD_BEGIN);

// 			MYIEngineManagerPtr EngineMgr = QueryModuleT<MYIEngineManager>();//协同模式不支持运行
// 
// 			if (EngineMgr)
// 			{
// 				EngineMgr->SetupEngine();
// 			}

			MYCollaborativeEditingConstructor::Instance()->CEInitialize(false);
			MYCollaborativeEditingConstructor::Instance()->StartUpdateData();

			GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_BasicInfo, BasicInfo.get());
			GetScenarioDomain()->UpdateTopic(Think_Simulation_Scenario_DeployLayer, Layer.get());
			DataHandler->SetCurrentDeployLayer(Layer->Name);

			MYCollaborativeEditingConstructor::Instance()->EndUpdateData();
			if (std::find(StudyInfo->Scenarios.begin(), StudyInfo->Scenarios.end(), BasicInfo->Name) == StudyInfo->Scenarios.end())
			{
				StudyInfo->Scenarios.push_back(BasicInfo->Name);
			}

			MYCollaborativeEditingConstructor::Instance()->Update(NULL, boost::bind(OnDataUpdateCallback, _1), true);

			if (MYIPresentationViewportPtr Viewport = MYGetCurrentWorld()->GetScene()->GetMainViewport())
			{
				MYViewportRect Rect(
					RADTODEG(BasicInfo->Region.WLon),
					RADTODEG(BasicInfo->Region.NLat),
					RADTODEG(BasicInfo->Region.ELon),
					RADTODEG(BasicInfo->Region.SLat));

				Viewport->SetViewprotRect(Rect);
			}

			MYEventBus::Instance()->PostEventBlocking(SE_UI_SCENARIO_LOAD_FINISHED);
			MYEventBus::Instance()->PostEventBlocking(SE_UI_SCENARIO_EDITING);
			MYEventBus::Instance()->PostEventBlocking(SE_UI_STUDY_KEY_ADD);

			/*MYScenarioMgrPluginViewer::Instance()->SetFocusScenario(
				StudyInfo->StudyId, BasicInfo->Name);*/
			return true;
		}
	}

	return false;
}

