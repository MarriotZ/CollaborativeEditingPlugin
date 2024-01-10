#include <QTextCharFormat>
#include <QTextCursor>

#include "MYCollaborativeEditLogWidget.h"
#include "ui_MYCollaborativeEditLogWidget.h"


struct MYCollaborativeLogWidgetPrivate
{
	MYCollaborativeLogWidgetPrivate()
	{

	}
};

MYCollaborativeEditLogWidget::MYCollaborativeEditLogWidget(QWidget *parent /*= 0*/)
	: QWidget(parent)
	, _p(new MYCollaborativeLogWidgetPrivate())
{
	ui = new Ui::MYCollaborativeEditLogWidget();
	ui->setupUi(this);
}

MYCollaborativeEditLogWidget::~MYCollaborativeEditLogWidget()
{
	delete _p;
	delete ui;
}

void MYCollaborativeEditLogWidget::ClearHintInfo()
{
	ui->textEdit->clear();
}

void MYCollaborativeEditLogWidget::AddSuccessfulInfo(QString Info)
{
	ui->textEdit->append(Info);
}

void MYCollaborativeEditLogWidget::AddFaildInfo(QString Info)
{
	QTextCursor TextCursor = ui->textEdit->textCursor();
	ui->textEdit->append(Info);

	QTextCharFormat Fmt;
	Fmt.setForeground(Qt::red);

	TextCursor.mergeCharFormat(Fmt);
	ui->textEdit->mergeCurrentCharFormat(Fmt);
}

