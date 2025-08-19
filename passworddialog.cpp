#include "passworddialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

PasswordDialog::PasswordDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Enter Password"));
    setModal(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *label = new QLabel(tr("This operation requires sudo access.\nPlease enter your password:"), this);
    mainLayout->addWidget(label);

    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    mainLayout->addWidget(passwordEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *okButton = new QPushButton(tr("OK"), this);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &QDialog::accept);
}

QString PasswordDialog::getPassword() const
{
    return passwordEdit->text();
}
