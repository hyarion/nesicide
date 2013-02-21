#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "FamiTracker.h"

#include <QFileInfo>
#include <QUrl>
#include <QMimeData>

MainWindow::MainWindow(QWidget *parent) :
   QMainWindow(parent),
   ui(new Ui::MainWindow)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiTracker");
   
   ui->setupUi(this);
   
   // CPTODO: this is a hack
   theApp.InitInstance();
   
   m_pMainFrame = (CMainFrame*)theApp.m_pMainWnd;
   setCentralWidget(m_pMainFrame);

   QObject::connect(m_pMainFrame,SIGNAL(addToolBarWidget(QToolBar*)),this,SLOT(addToolBarWidget(QToolBar*)));
   QObject::connect(m_pMainFrame,SIGNAL(removeToolBarWidget(QToolBar*)),this,SLOT(removeToolBarWidget(QToolBar*)));
   QObject::connect(m_pMainFrame,SIGNAL(editor_modificationChanged(bool)),this,SLOT(editor_modificationChanged(bool)));

   restoreGeometry(settings.value("FamiTrackerWindowGeometry").toByteArray());
   restoreState(settings.value("FamiTrackerWindowState").toByteArray());
}

MainWindow::~MainWindow()
{
   // TODO: Handle unsaved documents or other pre-close stuffs
   theApp.ExitInstance();

   delete ui;
}

void MainWindow::on_actionExit_triggered()
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiTracker");
   
   settings.setValue("FamiTrackerWindowGeometry",saveGeometry());
   settings.setValue("FamiTrackerWindowState",saveState());
   
   // Closing the main window kills the app
   close();
}

void MainWindow::addToolBarWidget(QToolBar* toolBar)
{
   addToolBar(toolBar);
}

void MainWindow::removeToolBarWidget(QToolBar* toolBar)
{
   removeToolBar(toolBar);
}

void MainWindow::editor_modificationChanged(bool m)
{
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
   if ( event->mimeData()->hasUrls() )
   {
      event->acceptProposedAction();
   }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
   if ( event->mimeData()->hasUrls() )
   {
      event->acceptProposedAction();
   }
}

void MainWindow::dropEvent(QDropEvent *event)
{
   QList<QUrl> fileUrls;
   QString     fileName;
   QFileInfo   fileInfo;
   QStringList extensions;

   if ( event->mimeData()->hasUrls() )
   {
      fileUrls = event->mimeData()->urls();

      foreach ( QUrl fileUrl, fileUrls )
      {
         fileName = fileUrl.toLocalFile();

         fileInfo.setFile(fileName);

         if ( !fileInfo.suffix().compare("ftm",Qt::CaseInsensitive) )
         {
            m_pMainFrame->setFileName(fileName);
            event->acceptProposedAction();
         }
      }
   }
}

void MainWindow::closeEvent(QCloseEvent *)
{
   QSettings settings(QSettings::IniFormat, QSettings::UserScope, "CSPSoftware", "FamiTracker");
   
   settings.setValue("FamiTrackerWindowGeometry",saveGeometry());
   settings.setValue("FamiTrackerWindowState",saveState());
}

