#ifndef __MYCOLLABORATIVEEDITLOGWIDGET_H__
#define __MYCOLLABORATIVEEDITLOGWIDGET_H__

#include <QWidget>
#include <TopSimRuntime/TopSimCommTypes.h>

class QTreeWidget;
class QTreeWidgetItem;

namespace Ui { class MYCollaborativeEditLogWidget; };

struct MYCollaborativeLogWidgetPrivate;

class MYCollaborativeEditLogWidget : public QWidget
{
	Q_OBJECT

public:
	MYCollaborativeEditLogWidget(QWidget* parent = 0);
	~MYCollaborativeEditLogWidget();

	void ClearHintInfo();

private slots:
	void AddSuccessfulInfo(QString Info);
	void AddFaildInfo(QString Info);

private:
	Ui::MYCollaborativeEditLogWidget* ui;
	MYCollaborativeLogWidgetPrivate* _p;
};

#endif //__MYCOLLABORATIVEEDITLOGWIDGET_H__
