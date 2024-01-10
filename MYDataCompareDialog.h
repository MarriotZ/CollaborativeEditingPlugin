#ifndef __MYDATACOMPAREDIALOG_H__
#define __MYDATACOMPAREDIALOG_H__

#include <MYUserAppSkeleton/MYUserDialog.h>

#include <MYScenarioLib/MYCollaborativeEditingPRC.h>


namespace Ui { class MYDataCompareDialog; };

////////////////////////////////////////////////////////////////////////////////////////////////////
///	FileName:	MYDataCompareDialog
///	Author:		MaxTsang
///	Time:		
/// Summary:	Ð­Í¬×é
////////////////////////////////////////////////////////////////////////////////////////////////////

struct MYDataCompareDialogPrivate;

class MYDataCompareDialog : public MYUserDialog
{
	Q_OBJECT

public:
	MYDataCompareDialog(QWidget* parent = 0);
	~MYDataCompareDialog();

	void SetCompareData(const std::vector<char>& Data1, const std::vector<char>& Data2);
	void SetCompareData(TSITopicContextPtr Ctx1, TSITopicContextPtr  Ctx2);
	void SetCompareData(const std::vector<MYCEDiffDataPtr>& Datas);
	void SetCompareData(MYCEDiffDataPtr DiffData);

private slots:
	void OnVScrollLocalValueChanged(int value);
	void OnVScrollRemotValueChanged(int value);

	void OnHScrollLocalValueChanged(int value);
	void OnHScrollRemotValueChanged(int value);

	void OnPushButtonCloseClicked();
	void OnTreeViewDoubleClicked(const QModelIndex& index);

private:
	void SetTextCursorColor(QColor Color);
	void UpateWidget();
	void AppendData(TSVariant Val1, TSVariant Val2, QString StrSpace = "");

private:
	Ui::MYDataCompareDialog* ui;
	MYDataCompareDialogPrivate* _p;
};

#include <QItemDelegate>

class QStandardItemModel;
class QStandardItem;
class MYUserSortFilterProxyModel;

class MYCollaborativeDataDelegate : public QItemDelegate
{
	Q_OBJECT

public:
	MYCollaborativeDataDelegate(QObject* parent = NULL);

	void Init(QStandardItemModel* Model, MYUserSortFilterProxyModel* ProxyModel);

protected:
	void paint(QPainter* painter,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;

	virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
	QStandardItemModel* _Model;
	MYUserSortFilterProxyModel* _ProxyModel;
};

#endif //__MYDATACOMPAREDIALOG_H__