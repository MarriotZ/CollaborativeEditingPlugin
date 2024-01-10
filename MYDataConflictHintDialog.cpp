#include <QButtonGroup>
#include <MYUtil/MYUtil.h>

#include "MYDataConflictHintDialog.h"
#include "MYCollaborativeEditingPlugin_Decl.h"

#include "ui_MYDataConflictHintDialog.h"

struct MYDataConflictHintDialogPrivate
{
	MYDataConflictHintDialogPrivate()
		: _ButtonGroup(NULL)
	{

	}

	QButtonGroup* _ButtonGroup;
};

MYDataConflictHintDialog::MYDataConflictHintDialog(QWidget *parent /*= 0*/)
	: MYUserDialog(parent)
	, _p(new MYDataConflictHintDialogPrivate())
{
	QWidget* W = new QWidget(this);

	ui = new Ui::MYDataConflictHintDialog();
	ui->setupUi(W);
	SetContentWidget(W);
	setWindowTitle(FromAscii("协同组"));

	ui->radioButton_Modify->setChecked(true);
	ui->checkBox->setCheckState(Qt::Checked);
	_p->_ButtonGroup = new QButtonGroup(this);

	connect(ui->radioButton_Skip, SIGNAL(clicked()), this, SLOT(OnRadioButtonClicked()));
	connect(ui->radioButton_Modify, SIGNAL(clicked()), this, SLOT(OnRadioButtonClicked()));

	connect(ui->pushButton_OK, SIGNAL(clicked()), this, SLOT(OnPushButtonOkClicked()));
	connect(ui->pushButton_Cancel, SIGNAL(clicked()), this, SLOT(OnPushButtonCancelClicked()));
}

MYDataConflictHintDialog::~MYDataConflictHintDialog()
{
	delete _p;
	delete ui;
}

void MYDataConflictHintDialog::Initialize(const QString& Name, const UINT32& Type)
{
	if (Type == SDT_Commit)
	{
		ui->radioButton_Skip->setText(FromAscii("不提交"));
		ui->radioButton_Modify->setText(FromAscii("提交，但保留这两个数据,重命名为:") + Name);
	}
	else if (Type == SDT_Update)
	{
		ui->radioButton_Skip->setText(FromAscii("不更新"));
		ui->radioButton_Modify->setText(FromAscii("更新，但保留这两个数据,重命名为:") + Name);
	}
}

void MYDataConflictHintDialog::Cleanup()
{

}

void MYDataConflictHintDialog::GetConflictWay(bool& IsShowDlg, UINT32& Solution)
{
	IsShowDlg = !(ui->checkBox->checkState() == Qt::Checked);
	Solution = _p->_ButtonGroup->checkedId();
}

void MYDataConflictHintDialog::OnRadioButtonClicked()
{

}

void MYDataConflictHintDialog::OnPushButtonOkClicked()
{
	accept();
}

void MYDataConflictHintDialog::OnPushButtonCancelClicked()
{
	reject();
}

