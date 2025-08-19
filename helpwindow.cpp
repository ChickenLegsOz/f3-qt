#include <QPushButton>
#include "helpwindow.h"
#include "ui_helpwindow.h"

HelpWindow::HelpWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HelpWindow)
{
    ui->setupUi(this);
    
    QString helpText;
    helpText = "<h3>About F3</h3>\n"
               "<p>This program is a free software.</p>\n\n"
               "<p>You can redistribute it and/or modify it under the terms of "
               "the GNU Library General Public License as published by "
               "the Free Software Foundation; either version 3 of the License, "
               "or (at your option) any later version.</p>\n\n"
               "<p>This package is distributed in the hope that it will be useful, but WITHOUT "
               "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or "
               "FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public License "
               "for more details.</p>\n";
    
    ui->helpBrowser->setHtml(helpText);
}

HelpWindow::~HelpWindow()
{
    delete ui;
}
