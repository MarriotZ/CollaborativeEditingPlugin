#include "stdafx.h"

#include <QApplication>
#include <QtnThinkHelper.h>

#include <MYUICommon/MYIMenuManager.h>
#include <MYUICommon/MYIPluginManager.h>
#include <MYUICommon/MYAppConfig.h>

#include <MYScenarioLib/MYScenarioEventDecl.h>
#include <MYUserAppSkeleton/MYUserAppSkeleton.h>

#include "MYCollaborativeEditingPlugin_Decl.h"
#include "MYCollaborativeEditingPlugin.h"
#include "MYCollaborativeEditWidget.h"

bool g_IsScenarioEditing = false;

struct MYCollaborativePluginPrivate
{
	MYCollaborativePluginPrivate()
	{
		_CEAction = NULL;
		_Group = NULL;
	}

	QAction* _CEAction;
	MYIAppSkeleton::Group* _Group;
	MYCollaborativeEditRegisterPtr _CERegister;
};

MYCollaborativeEditingPlugin::MYCollaborativeEditingPlugin()
	: _p(new MYCollaborativePluginPrivate)
{

}

MYCollaborativeEditingPlugin::~MYCollaborativeEditingPlugin()
{
	delete _p;
}

void MYCollaborativeEditingPlugin::Initialize(MYIGlobalPluginParamPtr Param)
{
	MYIGlobalPlugin::Initialize(Param);

	//ʹ������ͨ�ţ��������붨�༭ģʽ���ſ���ʹ��Эͬ�༭
	if (!MYAppManager::Instance()->IsUseNetwork() || MYAppManager::Instance()->GetRunMode() != MYAppManager::DesignView)
	{
		return;
	}

	if (Param && GetAppConfig<bool>("Root.CollaborativeEdit.IsUseCESerivce", true))
	{
		if (MYIAppSkeleton::Page* Page = Param->AppSkeleton->GetOrAddPage(FromAscii("��֯ս��")))
		{
			if (_p->_Group = Page->GetOrAddGroup(FromAscii("��֯ս��")))
			{
				_p->_CEAction = _p->_Group->AddAction("MYNJPGC|mx-hf", FromAscii("Эͬ����"));

				connect(_p->_CEAction, SIGNAL(triggered()), this, SLOT(OnEditActionTriggered()));

				_p->_CERegister.reset(new MYCollaborativeEditRegister);
				_p->_CERegister->Initialize(MYAppManager::Instance()->GetAppSkeleton(), 7);

				MYEventBus::Instance()->SubscribeMainTMYeadEvent(
					SE_UI_SCENARIO_EDITING, &MYCollaborativeEditingPlugin::OnScenarioEdit, this);

				MYEventBus::Instance()->SubscribeMainTMYeadEvent(
					SE_UI_SCENARIO_SAVE_REQUEST, &MYCollaborativeEditingPlugin::OnScenarioSave, this);

				MYEventBus::Instance()->SubscribeMainTMYeadEvent(
					SE_UI_SCENARIO_LOAD_FINISHED, &MYCollaborativeEditingPlugin::OnScenarioLoadFinished, this);
			}
		}
	}
}

void MYCollaborativeEditingPlugin::Cleanup()
{
	MYIGlobalPlugin::Cleanup();

	if (_p->_CERegister)
	{
		_p->_CERegister->Cleanup();
		_p->_CERegister.reset();
	}

	if (_p->_Group && _p->_CEAction)
	{
		_p->_Group->RemoveAction(_p->_CEAction);
		_p->_CEAction = NULL;
	}
}

TSString MYCollaborativeEditingPlugin::GetName()
{
	return toAsciiData("Эͬ������");
}

TSString MYCollaborativeEditingPlugin::GetDescription()
{
	return toAsciiData("Эͬ������");
}

void MYCollaborativeEditingPlugin::OnEditActionTriggered()
{
	if (_p->_CERegister)
	{
		_p->_CERegister->SetIsCurrent();
	}
}

void MYCollaborativeEditingPlugin::OnScenarioLoadFinished(const TSVariant & Param)
{
	if (_p->_CERegister)
	{
		_p->_CERegister->UpdateRoom();
	}
}

void MYCollaborativeEditingPlugin::OnScenarioEdit(const TSVariant& Param)
{
	g_IsScenarioEditing = true;
}

void MYCollaborativeEditingPlugin::OnScenarioSave(const TSVariant& Param)
{
	g_IsScenarioEditing = false;
}

DEF_VIEWPLUGINCONSTRUCTOR(MYCollaborativeEditingPlugin);
REGISTER_UIPLUGINS()
{
	QueryModuleT<MYIPluginManager>()->RegisterGlobalPluginConstructor(MYCollaborativeEditingPluginConstructor::GetMetaTypeIdStatic());
	return true;
}