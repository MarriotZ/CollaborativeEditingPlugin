#ifndef __MYCOLLABORATIVEEDITINGPLUGIN_DECL_H__
#define __MYCOLLABORATIVEEDITINGPLUGIN_DECL_H__

#include <TopSimRuntime/TSSingleton.h>

#include <MYEventDeclare/MYEventDeclare.h>

enum ScenarioDataType
{
	SDT_Unknown = 1,
	SDT_Commit = 1 << 1,
	SDT_Conflict = 1 << 2,
	SDT_Update = 1 << 3,
};

Q_DECLARE_METATYPE(ScenarioDataType);

enum OperatorType
{
	OT_Unknown = 1,
	OT_Commit,
	OT_Update,
};

Q_DECLARE_METATYPE(OperatorType);

enum DiffDataSolveWay
{
	DDS_Default = 1,
	DDS_UseLocal,
	DDS_UseRemote,
};

Q_DECLARE_METATYPE(DiffDataSolveWay);

#define ScenarioDataTypeRole Qt::UserRole + 2
#define CommitDataRole Qt::UserRole + 3
#define DiffDataRole Qt::UserRole + 4
#define UpdateDataRole Qt::UserRole + 5
#define SolveWayRole Qt::UserRole + 6
#define IsSolveConflictRole Qt::UserRole + 7
#define ICONSIZE QSize(24, 24)

class MYCollaborativeCommon :public TSSingleton<MYCollaborativeCommon>
{
	SINGLETON_DECLARE(MYCollaborativeCommon);

public:
	//通过实体句柄获取当前所在的部署层
	MYDeployLayerPtr GetDeployLayerByHandle(const TSHANDLE& Handle);

	QString GetMissionName(QString MissionName);


	MYLogicEntityInitInfoPtr GetEntityInfo(const TSHANDLE& Handle, TSDomainPtr Domain);
	MYSimResourcePtr GetSimResource(const TSHANDLE& Handle, TSDomainPtr Domain);

	bool UpdateScenarioDataToLocal(MYScenarioBasicInfoPtr BasicInfo, MYDeployLayerPtr Layer, MYIdentManagerPtr IdentManager);
};

#endif //__MYCOLLABORATIVEEDITINGPLUGIN_DECL_H__