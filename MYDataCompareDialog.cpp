#include <QScrollBar>
#include <QStandardItemModel>
#include <QtnThinkHelper.h>

#include <TopSimMMF_Inter/ThinkUtils.h>
#include <MYUtil/MYUtil.h>

#include <MYControls/MYModelBoxFilterWidget.h>
#include <MYControls/MYUserSortFilterProxyModel.h>
#include <MYScenarioLib/MYScenarioCEHelper.h>
#include <MYScenarioLib/MYMissionHandler.h>
#include <MYScenarioLib/MYIScenarioDataHandler.h>

#include "MYCollaborativeEditingPlugin_Decl.h"
#include "MYDataCompareDialog.h"

#include "ui_MYDataCompareDialog.h"

#include <TopSimRuntime/TSXMLArchive.h>
#include <TopSimRuntime/pugixml.hpp>

#define Space "                    "

#define OrignColor Qt::white

Q_DECLARE_METATYPE(MYCEDiffDataPtr);

class  MYCEXmlWriter : public pugi::xml_writer
{
public:
	virtual void write(const void* data, size_t size)
	{
		const char * bufferdata = (const char*)(data);
		bool IsFirst = false;
		for (std::size_t i = 0; i < size; ++i)
		{
			_buffer.push_back(bufferdata[i]);
		}
	}

	TSString  getbuffer()
	{
		TSString Text;
		std::vector<TSString> StrList = TSStringUtil::Split(_buffer, "\n");

		if (StrList.size() > 2)
		{
			for (size_t i = 1; i < StrList.size() -1; ++i)
			{
				Text += StrList[i];

				if ( (i + 1) < StrList.size() - 1)
				{
					Text += "\n";
				}
			}
		}
		else
		{
			return _buffer;
		}

		return Text;
	}

private:
	TSString _buffer;
};

TSString ToText(TSVariant Val, const TSString& PropName)
{
	TSDataArchiveContextPtr ArchiveCtx(new TSDataArchiveContext);
	TSXMLSaveArchive Archive(ArchiveCtx);

	pugi::xml_document Doc;

	Archive.SetRootNode(&Doc);
	Archive.SaveValue(PropName, Val);

	MYCEXmlWriter Writer;
	Doc.save(Writer);

	return Writer.getbuffer();
}

struct MYDataCompareDialogPrivate
{
	MYDataCompareDialogPrivate()
		: _ItemModel(NULL)
		, _ProxyModel(NULL)
		, _DataDelegate(NULL)
	{

	}

	MYUserSortFilterProxyModel *_ProxyModel;
	QStandardItemModel* _ItemModel;
	MYCollaborativeDataDelegate* _DataDelegate;
};

MYDataCompareDialog::MYDataCompareDialog(QWidget *parent /*= 0*/)
	: MYUserDialog(parent)
	, _p(new MYDataCompareDialogPrivate())
{
	QWidget* W = new QWidget(this);

	ui = new Ui::MYDataCompareDialog();
	ui->setupUi(W);
	SetContentWidget(W);

	connect(ui->pushButton_Close, SIGNAL(clicked()), this, SLOT(OnPushButtonCloseClicked()));

	_p->_ItemModel = new QStandardItemModel();
	_p->_ProxyModel = new MYUserSortFilterProxyModel();
	_p->_ProxyModel->setShowChildern(true);
	_p->_ProxyModel->setFilterKeyColumn(-1);

	_p->_ProxyModel->setSourceModel(_p->_ItemModel);
	ui->treeView->setModel(_p->_ProxyModel);

	_p->_DataDelegate = new MYCollaborativeDataDelegate(ui->treeView);
	_p->_DataDelegate->Init(_p->_ItemModel, _p->_ProxyModel);
	ui->treeView->setItemDelegate(_p->_DataDelegate);

	connect(ui->treeView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(OnTreeViewDoubleClicked(const QModelIndex &)));
	setWindowTitle(FromAscii("数据对比"));
}

MYDataCompareDialog::~MYDataCompareDialog()
{
	delete _p;
	delete ui;
}

void MYDataCompareDialog::SetCompareData(const std::vector<char>& Data1, const std::vector<char>& Data2)
{
	ui->widget_Tree->hide();

	std::vector<TSString> StrList1, StrList2;

	TSString StrText;

	for (size_t i = 0; i < Data1.size(); ++i)
	{
		if (Data1[i] == '\n')
		{
			StrList1.push_back(StrText);
			StrText = "";
		}
		else
		{
			StrText += Data1[i];
		}
	}

	StrText = "";

	for (size_t i = 0; i < Data2.size(); ++i)
	{
		if (Data2[i] == '\n')
		{
			StrList2.push_back(StrText);
			StrText = "";
		}
		else
		{
			StrText += Data2[i];
		}
	}

	size_t Lenth = StrList1.size() > StrList2.size() ? StrList1.size() : StrList2.size();

	for (size_t i = 1; i < Lenth; ++i)
	{
		TSString Str1, Str2;

		if (StrList1.size() > i)
		{
			Str1 = StrList1[i];
		}

		if (StrList2.size() > i)
		{
			Str2 = StrList2[i];
		}

		if (Str1 != Str2)
		{
			SetTextCursorColor(Qt::red);
		}
		else
		{
			SetTextCursorColor(OrignColor);
		}

		ui->textEdit_Local->append(TSString2QString(Str1));
		ui->textEdit_Remote->append(TSString2QString(Str2));
	}

	UpateWidget();
}

void MYDataCompareDialog::SetCompareData(TSITopicContextPtr Ctx1, TSITopicContextPtr Ctx2)
{
	ui->widget_Tree->hide();
	if (Ctx1->GetDataTopicHandle() == Ctx2->GetDataTopicHandle())
	{
		TSVariant Val1 = ThinkUtils::FromValue(Ctx1->GetDataTopicHandle(), Ctx1->GetTopic());
		TSVariant Val2 = ThinkUtils::FromValue(Ctx2->GetDataTopicHandle(), Ctx2->GetTopic());

		AppendData(Val1, Val2);
		UpateWidget();
	}
	else
	{
		SetCompareData(MYScenarioCEHelper::Instance()->ToText(Ctx1), MYScenarioCEHelper::Instance()->ToText(Ctx2));
	}
}

void MYDataCompareDialog::SetCompareData(const std::vector<MYCEDiffDataPtr>& Datas)
{
	resize(QSize(400, 800));

	ui->widget_Data->hide();
	_p->_ItemModel->setHorizontalHeaderLabels(QStringList() << FromAscii("本地数据") << FromAscii("远端数据"));
	ui->treeView->setColumnWidth(0, 200);

	for (size_t i = 0; i < Datas.size(); ++i)
	{
		QStandardItem* LocalItem = new QStandardItem;
		QStandardItem* RemoteItem = new QStandardItem;

		LocalItem->setData(QVariant::fromValue(SDT_Conflict), ScenarioDataTypeRole);
		LocalItem->setData(QVariant::fromValue(Datas[i]), DiffDataRole);
		LocalItem->setEditable(false);
		RemoteItem->setEditable(false);
		_p->_ItemModel->appendRow(LocalItem);
		_p->_ItemModel->setItem(_p->_ItemModel->rowCount() - 1, 1, RemoteItem);

		TSTopicHelperPtr TopicHelper = TSTopicTypeManager::Instance()->GetTopicHelperByTopic(Datas[i]->ThierTopicHandle);

		if (TopicHelper->CanConvert(Think_Simulation_MMF_ComponentInitInfo))
		{
			MYComponentInitInfoPtr LocalData = TS_CAST(Datas[i]->MimeIObj, MYComponentInitInfoPtr);
			MYComponentInitInfoPtr RemoteData = TS_CAST(Datas[i]->TheirIObj, MYComponentInitInfoPtr);

			if (LocalData && RemoteData)
			{
				LocalItem->setText(TSString2QString(LocalData->Name));
				RemoteItem->setText(TSString2QString(RemoteData->Name));
			}
		}
		else if (TopicHelper->CanConvert(Think_Simulation_MMF_MissionParam))
		{
			MYMissionParamPtr LocalData = TS_CAST(Datas[i]->MimeIObj, MYMissionParamPtr);
			MYMissionParamPtr RemoteData = TS_CAST(Datas[i]->TheirIObj, MYMissionParamPtr);

			if (LocalData && RemoteData)
			{
				LocalItem->setText(MYCollaborativeCommon::Instance()->GetMissionName(TSString2QString(LocalData->MissionName)));
				RemoteItem->setText(MYCollaborativeCommon::Instance()->GetMissionName(TSString2QString(RemoteData->MissionName)));
			}
		}
		else if (TopicHelper->CanConvert(Think_Simulation_MMF_MissionSchedParam))
		{
			LocalItem->setText(MYMissionHandler::Instance()->GetSchedNameByTopicHandle(Datas[i]->MimeTopicHandle));
			RemoteItem->setText(MYMissionHandler::Instance()->GetSchedNameByTopicHandle(Datas[i]->ThierTopicHandle));
		}
		else if (TopicHelper->CanConvert(Think_Simulation_Models_MotionPlan))
		{
			LocalItem->setText(FromAscii("机动计划"));
			RemoteItem->setText(FromAscii("机动计划"));
		}
		else if (TopicHelper->CanConvert(Think_Simulation_MMF_LogicEntityInitInfo))
		{
			MYLogicEntityInitInfoPtr LocalEntityInfo = TS_CAST(Datas[i]->MimeIObj, MYLogicEntityInitInfoPtr);
			MYLogicEntityInitInfoPtr RemoteEntityInfo = TS_CAST(Datas[i]->TheirIObj, MYLogicEntityInitInfoPtr);

			if (LocalEntityInfo && RemoteEntityInfo)
			{
				LocalItem->setText(TSString2QString(LocalEntityInfo->Name));
				RemoteItem->setText(TSString2QString(RemoteEntityInfo->Name));
			}
		}
		else if (TopicHelper->CanConvert(Think_Simulation_Formation))
		{
			MYFormationInfoPtr LocalData = TS_CAST(Datas[i]->MimeIObj, MYFormationInfoPtr);
			MYFormationInfoPtr RemoteData = TS_CAST(Datas[i]->TheirIObj, MYFormationInfoPtr);

			if (LocalData && RemoteData)
			{
				LocalItem->setText(TSString2QString(LocalData->FormationName));
				RemoteItem->setText(TSString2QString(RemoteData->FormationName));
			}
		}
		else
		{
			_p->_ItemModel->removeRow(LocalItem->row());
		}
	}
}

void MYDataCompareDialog::SetCompareData(MYCEDiffDataPtr DiffData)
{
	ui->widget_Tree->hide();

	if (DiffData)
	{
		TSTypeSupportPtr Support1 = TSTopicTypeManager::Instance()->GetTypeSupportByTopic(DiffData->MimeTopicHandle);
		TSTypeSupportPtr Support2 = TSTopicTypeManager::Instance()->GetTypeSupportByTopic(DiffData->ThierTopicHandle);

		if (Support1 && Support2)
		{
			TSVariant Val1(TSMetaType::GetType(Support1->GetTypeName()), &(DiffData->MimeIObj), TSVariant::Is_SmartPointer);
			TSVariant Val2(TSMetaType::GetType(Support2->GetTypeName()), &(DiffData->TheirIObj), TSVariant::Is_SmartPointer);

			AppendData(Val1, Val2);
			UpateWidget();
		}
	}
}

void MYDataCompareDialog::OnVScrollLocalValueChanged(int value)
{
	disconnect(ui->textEdit_Remote->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnVScrollRemotValueChanged(int)));

	ui->textEdit_Remote->verticalScrollBar()->setSliderPosition((double)value / (double)ui->textEdit_Local->verticalScrollBar()->maximum() * (double)ui->textEdit_Remote->verticalScrollBar()->maximum());

	connect(ui->textEdit_Remote->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnVScrollRemotValueChanged(int)));
}

void MYDataCompareDialog::OnVScrollRemotValueChanged(int value)
{
	disconnect(ui->textEdit_Local->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnVScrollLocalValueChanged(int)));

	ui->textEdit_Local->verticalScrollBar()->setSliderPosition((double)value / (double)ui->textEdit_Remote->verticalScrollBar()->maximum() * (double)ui->textEdit_Local->verticalScrollBar()->maximum());

	connect(ui->textEdit_Local->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnVScrollLocalValueChanged(int)));
}

void MYDataCompareDialog::OnHScrollLocalValueChanged(int value)
{
	disconnect(ui->textEdit_Remote->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnHScrollRemotValueChanged(int)));

	ui->textEdit_Remote->horizontalScrollBar()->setSliderPosition((double)value / (double)ui->textEdit_Local->horizontalScrollBar()->maximum() * (double)ui->textEdit_Remote->horizontalScrollBar()->maximum());

	connect(ui->textEdit_Remote->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnHScrollRemotValueChanged(int)));
}

void MYDataCompareDialog::OnHScrollRemotValueChanged(int value)
{
	disconnect(ui->textEdit_Local->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnHScrollLocalValueChanged(int)));

	ui->textEdit_Local->horizontalScrollBar()->setSliderPosition((double)value / (double)ui->textEdit_Remote->horizontalScrollBar()->maximum() * (double)ui->textEdit_Local->horizontalScrollBar()->maximum());

	connect(ui->textEdit_Local->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnHScrollLocalValueChanged(int)));
}

void MYDataCompareDialog::OnPushButtonCloseClicked()
{
	QDialog::accept();
}

void MYDataCompareDialog::OnTreeViewDoubleClicked(const QModelIndex & index)
{
	if (index.isValid())
	{
		if (QStandardItem* Item = _p->_ItemModel->item(index.row(), 0))
		{
			if (MYCEDiffDataPtr DiffData = Item->data(DiffDataRole).value<MYCEDiffDataPtr>())
			{
				MYDataCompareDialog Dlg(this);
				Dlg.SetCompareData(DiffData);
				Dlg.exec();
			}
		}
	}
}

void MYDataCompareDialog::SetTextCursorColor(QColor Color)
{
	QTextCursor TextCursor = ui->textEdit_Local->textCursor();
	QTextCharFormat Fmt;
	Fmt.setForeground(Color);

	TextCursor.mergeCharFormat(Fmt);
	ui->textEdit_Local->mergeCurrentCharFormat(Fmt);

	QTextCursor TextCursor1 = ui->textEdit_Local->textCursor();
	QTextCharFormat Fmt1;
	Fmt1.setForeground(Color);

	TextCursor.mergeCharFormat(Fmt1);
	ui->textEdit_Remote->mergeCurrentCharFormat(Fmt1);
}

void MYDataCompareDialog::UpateWidget()
{
	ui->textEdit_Local->verticalScrollBar()->setValue(0);
	ui->textEdit_Remote->verticalScrollBar()->setValue(0);
	ui->textEdit_Local->horizontalScrollBar()->setValue(0);
	ui->textEdit_Remote->horizontalScrollBar()->setValue(0);

	connect(ui->textEdit_Local->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnVScrollLocalValueChanged(int)));
	connect(ui->textEdit_Remote->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnVScrollRemotValueChanged(int)));

	connect(ui->textEdit_Local->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnHScrollLocalValueChanged(int)));
	connect(ui->textEdit_Remote->horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(OnHScrollRemotValueChanged(int)));
}

void MYDataCompareDialog::AppendData(TSVariant Val1, TSVariant Val2, QString StrSpace)
{
	MetaPropertyCollection Properties1 = TSMetaType::GetProperties(Val1.GetType());
	MetaPropertyCollection Properties2 = TSMetaType::GetProperties(Val2.GetType());

	size_t Count = Properties1.size() > Properties2.size() ? Properties1.size() : Properties2.size();

	for (std::size_t i = 0; i < Count; ++i)
	{
		if (i < Properties1.size() && i < Properties2.size())
		{
			TSMetaProperty* Property1 = Properties1[i];
			TSMetaProperty* Property2 = Properties2[i];

			TSVariant PropertyValueVal1 = Property1->PropertyOperator->GetValue(Val1.GetDataPtr());
			TSVariant PropertyValueVal2 = Property2->PropertyOperator->GetValue(Val2.GetDataPtr());

			QString Str1 = TSString2QString(ToText(PropertyValueVal1, Property1->Name));
			QString Str2 = TSString2QString(ToText(PropertyValueVal2, Property2->Name));

			if (Str1 != Str2)
			{
				QStringList StrList1 = Str1.split("\n");
				QStringList StrList2 = Str2.split("\n");
				int Row1 = StrList1.size();
				int Row2 = StrList2.size();

				if (TSMetaType::GetProperties(PropertyValueVal1.GetType()).size() > 1
					&& Row1 > 1 && Row2 > 1)
				{
					ui->textEdit_Local->append(StrSpace + StrList1[0]);
					ui->textEdit_Remote->append(StrSpace + StrList2[0]);
					AppendData(PropertyValueVal1, PropertyValueVal2, StrSpace + Space);

					SetTextCursorColor(OrignColor);
					ui->textEdit_Local->append(StrSpace + StrList1[Row1 - 1]);
					ui->textEdit_Remote->append(StrSpace + StrList2[Row2 - 1]);
				}
				else
				{
					if (Row1 > Row2)
					{
						for (int j = 0; j < Row1; ++j)
						{
							if (j < Row2 - 1)
							{
								SetTextCursorColor(StrList1[j] != StrList2[j] ? Qt::red : OrignColor);

								ui->textEdit_Remote->append(StrSpace + StrList2[j]);
							}
							else
							{
								if (j < Row1 - 1)
								{
									SetTextCursorColor(Qt::red);
									ui->textEdit_Remote->append("");
								}
								else
								{
									SetTextCursorColor(StrList2[Row2 - 1] != StrList1[j] ? Qt::red : OrignColor);

									ui->textEdit_Remote->append(StrSpace + StrList2[Row2 - 1]);
								}
							}

							ui->textEdit_Local->append(StrSpace + StrList1[j]);
						}
					}
					else
					{
						for (int j = 0; j < Row2; ++j)
						{
							if (j < Row1 - 1)
							{
								SetTextCursorColor(StrList1[j] != StrList2[j] ? Qt::red : OrignColor);
								ui->textEdit_Local->append(StrSpace + StrList1[j]);
							}
							else
							{
								if (j < Row2 - 1)
								{
									SetTextCursorColor(Qt::red);
									ui->textEdit_Local->append("");
								}
								else
								{
									SetTextCursorColor(StrList1[Row1 - 1] != StrList2[j] ? Qt::red : OrignColor);
									ui->textEdit_Local->append(StrSpace + StrList1[Row1 - 1]);
								}
							}

							ui->textEdit_Remote->append(StrSpace + StrList2[j]);
						}
					}
				}
			}
			else
			{
				SetTextCursorColor(OrignColor);
				ui->textEdit_Local->append(StrSpace + Str1);
				ui->textEdit_Remote->append(StrSpace + Str2);
			}
		}
		else if (Properties1.size() < Properties2.size())
		{
			TSMetaProperty* Property2 = Properties2[i];
			TSVariant PropertyValueVal2 = Property2->PropertyOperator->GetValue(Val2.GetDataPtr());
			QString Str2 = TSString2QString(ToText(PropertyValueVal2, Property2->Name));

			SetTextCursorColor(Qt::red);
			ui->textEdit_Remote->append(StrSpace + Str2);
		}
		else if (Properties2.size() < Properties1.size())
		{
			TSMetaProperty* Property1 = Properties1[i];
			TSVariant PropertyValueVal1 = Property1->PropertyOperator->GetValue(Val1.GetDataPtr());
			QString Str1 = TSString2QString(ToText(PropertyValueVal1, Property1->Name));
			SetTextCursorColor(Qt::red);
			ui->textEdit_Local->append(StrSpace + Str1);
		}
	}
}

//////////////////////////////////////
#include <QPainter>

static QColor ConflictColor = QtnThinkThemeServices::Instance()->GetThemeResourceColor(
	"CollaborativeData", "DataConflict", OrignColor);

static QColor UpdateColor = QtnThinkThemeServices::Instance()->GetThemeResourceColor(
	"CollaborativeData", "DataUpdate", OrignColor);

static QColor CommitColor = QtnThinkThemeServices::Instance()->GetThemeResourceColor(
	"CollaborativeData", "DataCommit", OrignColor);

static QColor MouseOverColor = QtnThinkThemeServices::Instance()->GetThemeResourceColor("CollaborativeData", "ItemMouseOver");

static QColor HighlightColor = QtnThinkThemeServices::Instance()->GetThemeResourceColor("CollaborativeData", "ItemBackground");

MYCollaborativeDataDelegate::MYCollaborativeDataDelegate(QObject* parent /*= NULL*/)
	:QItemDelegate(parent)
{

}

void MYCollaborativeDataDelegate::Init(QStandardItemModel* Model, MYUserSortFilterProxyModel* ProxyModel)
{
	_ProxyModel = ProxyModel;
	_Model = Model;
}

void MYCollaborativeDataDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QStyleOptionViewItem ViewOption(option);
	QColor FontColor(OrignColor);

	if (QStandardItem *Item = _Model->itemFromIndex(_ProxyModel->mapToSource(index)))
	{
		if (option.state & QStyle::State_MouseOver)
		{
			painter->save();		
			painter->fillRect(option.rect, QBrush(MouseOverColor));
			painter->restore();
		}

		Item = _Model->item(Item->row(), 0);

		if (Item)
		{
			ScenarioDataType Type = Item->data(ScenarioDataTypeRole).value<ScenarioDataType>();
			bool IsSolve = Item->data(IsSolveConflictRole).value<bool>();

			if (SDT_Conflict == Type
				&& !IsSolve)
			{
				FontColor = ConflictColor;
			}
		}

		if (option.state & QStyle::State_Selected)
		{
			ViewOption.palette.setColor(QPalette::Highlight, HighlightColor);
			ViewOption.palette.setColor(QPalette::HighlightedText, FontColor);
		}
	}

	ViewOption.palette.setColor(QPalette::Text, FontColor);
	QItemDelegate::paint(painter, ViewOption, index);
}

QSize MYCollaborativeDataDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize Size = QItemDelegate::sizeHint(option, index);

	Size.setHeight(34);

	return Size;
}
