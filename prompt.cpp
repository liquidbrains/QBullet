#include "prompt.h"
#include "ui_prompt.h"

Prompt::Prompt(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Prompt)
{
    ui->setupUi(this);
}

Prompt::~Prompt()
{
    delete ui;
}
int Prompt::showPrompt(QString windowTitle, QString name, QString text)
{
    this->setWindowTitle(windowTitle);
    ui->txtTextBlock->setPlainText(text);
    ui->txtTitle->clear();
    ui->txtTitle->setPlaceholderText(name);

    return this->exec();
}

const QString Prompt::getTitle()
{
    return ui->txtTitle->text();
}

const QString Prompt::getText()
{
    return ui->txtTextBlock->toPlainText();
}
