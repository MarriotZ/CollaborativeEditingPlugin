
#include <MYUtil/MYUtil.h>

#include <MYUserAppSkeleton/MYUserMessageBox.h>

#include "MYCollaborativeUserNameDialog.h"

#include "ui_MYCollaborativeUserNameDialog.h"

struct MYCollaborativeUserNameDialogPrivate
{
	MYCollaborativeUserNameDialogPrivate()
	{

	}

	MYCERoomInfoExPtr _RoomInfo;
};

MYCollaborativeUserNameDialog::MYCollaborativeUserNameDialog(MYCERoomInfoExPtr CERoomInfo, QWidget *parent /*= 0*/)
	: MYUserDialog(parent)
	, _p(new MYCollaborativeUserNameDialogPrivate())
{
	QWidget* W = new QWidget(this);

	ui = new Ui::MYCollaborativeUserNameDialog();
	ui->setupUi(W);
	SetContentWidget(W);
	setWindowTitle(FromAscii("层名称"));

	connect(ui->pushButton_OK, SIGNAL(clicked()), this, SLOT(OnPushButtonOkClicked()));
	connect(ui->pushButton_Cancel, SIGNAL(clicked()), this, SLOT(OnPushButtonCancelClicked()));

	_p->_RoomInfo = CERoomInfo;
}

MYCollaborativeUserNameDialog::~MYCollaborativeUserNameDialog()
{
	delete _p;
	delete ui;
}

QString MYCollaborativeUserNameDialog::GetLayerName()
{
	return ui->lineEdit_LayerName->text();
}

bool MYCollaborativeUserNameDialog::Check(const QString & LoginName)
{
	if (_p->_RoomInfo && CEProxy())
	{
		try
		{
			return !CEProxy()->IsJoin(_p->_RoomInfo, QString2TSString(LoginName));
		}
		catch (TSException& e)
		{
			tMYow TSException(e.what());
		}
	}
	else
	{
		return true;
	}
}

void MYCollaborativeUserNameDialog::OnPushButtonOkClicked()
{
	try
	{
		ui->lineEdit_LayerName->Verify();
	}
	catch (const TSException& e)
	{
		MYUserMessageBox::warning(this, FromAscii("系统提示"), TSString2QString(e.what()));
		return;
	}

	QString UserName = ui->lineEdit_LayerName->text();

	try
	{
		if (!Check(UserName))
		{
			MYUserMessageBox::warning(this, FromAscii("系统提示"), FromAscii("此层已被其他席位使用!"));
			return;
		}
		else
		{
			accept();
		}
	}
	catch (TSException& e)
	{
		MYUserMessageBox::warning(this, FromAscii("系统提示"), TSString2QString(e.what()));
	}
}

void MYCollaborativeUserNameDialog::OnPushButtonCancelClicked()
{
	reject();
}

