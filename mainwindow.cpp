#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_nav(new Navigator(this))
{
    m_nav->setData(&m_data);
    ui->setupUi(this);
    QObject::connect(this, SIGNAL(onBack()), m_nav, SLOT(back()));
    QObject::connect(this, SIGNAL(onHome()), m_nav, SLOT(home()));
    QObject::connect(m_nav, SIGNAL(hasHistory(bool)), this, SLOT(hasHistory(bool)));
}

MainWindow::~MainWindow()
{
    delete m_nav;
    delete ui;
}

void MainWindow::open()
{
    bool backEnabled = ui->actionBack->isEnabled();

    ui->actionBack->setEnabled(false);
    ui->actionHome->setEnabled(false);
    ui->actionOpen->setEnabled(false);

    QString fileName = QFileDialog::getOpenFileName(this, QString(), QString(), tr("XML files (*.xml)"));
    if (fileName.isEmpty())
    {
        ui->actionBack->setEnabled(backEnabled);
        ui->actionHome->setEnabled(true);
        ui->actionOpen->setEnabled(true);
        return;
    }

    QString title("%1 - %2");
    setWindowTitle(title
                   .arg(QFileInfo(fileName).fileName())
                   .arg(tr("Profile Viewer"))
                   );

    onOpened(false /* m_data.open(fileName) */, fileName);
}

void MainWindow::onOpened(bool success, const QString &fileName)
{
    ui->actionBack->setEnabled(true);
    ui->actionHome->setEnabled(true);
    ui->actionOpen->setEnabled(true);

    home();

    if (!success)
        QMessageBox::warning(this, tr("Profile Viewer"), tr("Cannot read file %1.").arg(fileName));
}

void MainWindow::hasHistory(bool value)
{
    ui->actionBack->setEnabled(value);
}
