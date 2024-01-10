#ifndef __MYCOLLABORATIVEEDITINGPLUGIN_H__
#define __MYCOLLABORATIVEEDITINGPLUGIN_H__


#include <MYUICommon/MYIGlobalPlugin.h>

struct MYCollaborativePluginPrivate;

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYCollaborativeEditingPlugin
///	Author:		MaxTsang
///	Time:		
/// Summary:	
////////////////////////////////////////////////////////////////////////////////////////////////////

class MYCollaborativeEditingPlugin : public MYIGlobalPlugin
{
	Q_OBJECT
public:
	MYCollaborativeEditingPlugin();
	~MYCollaborativeEditingPlugin();

protected:
	virtual void Initialize(MYIGlobalPluginParamPtr Param);
	virtual void Cleanup();
	virtual TSString GetName();
	virtual TSString GetDescription();

private slots:
	void OnEditActionTriggered();

private:
	void OnScenarioLoadFinished(const TSVariant& Param);
	void OnScenarioEdit(const TSVariant& Param);
	void OnScenarioSave(const TSVariant& Param);

private:
	MYCollaborativePluginPrivate* _p;
};

#endif //__MYCOLLABORATIVEEDITINGPLUGIN_H__