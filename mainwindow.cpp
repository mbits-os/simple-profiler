#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QThread>
#include <QMovie>
#include "profiler_model.h"

namespace Ui
{
    class MainWindowEx: public MainWindow
    {
    public:
        QMovie* throbber;

        void setupUi(QMainWindow *MainWindow)
        {
            MainWindow::setupUi(MainWindow);

            throbber = new QMovie(":/res/throbber.gif", QByteArray(), MainWindow);
            throbber->setObjectName(QStringLiteral("throbber"));
            throbberLabel->setMovie(throbber);

            throbber->start();
            throbber->setPaused(true);
        }
    };
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindowEx),
    m_data(std::make_shared<profiler::data>()),
    m_nav(new Navigator(this)),
    m_model(new ProfilerModel(this)),
    m_animationCount(0)
{
    m_nav->setData(m_data);
    ui->setupUi(this);
    hasHistory(false);

    QObject::connect(this, SIGNAL(onBack()), m_nav, SLOT(back()));
    QObject::connect(this, SIGNAL(onHome()), m_nav, SLOT(home()));
    QObject::connect(m_nav, SIGNAL(hasHistory(bool)), this, SLOT(hasHistory(bool)));
    QObject::connect(m_nav, SIGNAL(selectStarted()),  this, SLOT(aTaskStarted()));
    QObject::connect(m_nav, SIGNAL(selectStopped()),  this, SLOT(aTaskStopped_nav()));

    m_model->appendColumn(Columns::Count::create());
    m_model->appendColumn(Columns::TotalTime::create());
    m_model->appendColumn(Columns::OwnTime::create());
    m_model->appendColumn(Columns::Graph::create());
    m_model->appendColumn(Columns::TotalTimeAvg::create());
    m_model->appendColumn(Columns::OwnTimeAvg::create());
    m_model->appendColumn(Columns::GraphAvg::create());
    m_model->appendColumn(Columns::Name::create());

    ui->treeView->setModel(m_model);
    ui->treeView->header()->setSectionsClickable(true);
    ui->treeView->sortByColumn(1, Qt::DescendingOrder);
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

    m_nav->cancel();

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

    m_data = std::make_shared<profiler::data>();

    aTaskStarted();
    doOpen(fileName);
}

void MainWindow::doOpen(const QString& fileName)
{
    auto data = m_data;
    OpenTask* task = new OpenTask(this, [data, fileName](){ return data->open(fileName); }, fileName);
    QObject::connect(task, SIGNAL(opened(bool,QString)), this, SLOT(onOpened(bool,QString)));
    connect(task, &OpenTask::finished, task, &QObject::deleteLater);
    task->start();
}

void OpenTask::run()
{
    bool success = call();
    emit opened(success, fileName);
}

void MainWindow::onOpened(bool success, QString fileName)
{
    ui->actionBack->setEnabled(true);
    ui->actionHome->setEnabled(true);
    ui->actionOpen->setEnabled(true);

    m_nav->setData(m_data);
    home();
    aTaskStopped();

    if (!success)
        QMessageBox::warning(this, tr("Profile Viewer"), tr("Cannot read file %1.").arg(fileName));
}

void MainWindow::aTaskStarted()
{
    if (!m_animationCount)
    {
        ui->throbber->setPaused(false);
        ui->stackedWidget->setCurrentIndex(1);
    }

    ++m_animationCount;
}

void MainWindow::aTaskStopped()
{
    if (!--m_animationCount)
    {
        ui->stackedWidget->setCurrentIndex(0);
        ui->throbber->setPaused(true);
    }
}

void MainWindow::aTaskStopped_nav()
{
    m_model->beginResetModel();

    m_model->setSecond(m_data->second());
    auto profile = m_nav->current();
    m_model->setMaxDuration(profile->max_duration());
    m_model->setProfileView(profile->get_cached());

    m_model->endResetModel();

    aTaskStopped();
}

void MainWindow::hasHistory(bool value)
{
    ui->actionBack->setEnabled(value);
}
