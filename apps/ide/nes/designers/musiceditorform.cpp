#include "musiceditorform.h"
#include "ui_musiceditorform.h"

MusicEditorForm::MusicEditorForm(QString fileName,QByteArray musicData,IProjectTreeViewItem* link,QWidget* parent) :
   CDesignerEditorBase(link,parent),
   ui(new Ui::MusicEditorForm)
{
   ui->setupUi(this);
   
   m_fileName = fileName;
   
   m_pMainFrame = (CMainFrame*)theApp.m_pMainWnd;

   m_pMainFrame->setFileName(fileName);

   ui->stackedWidget->addWidget(m_pMainFrame);
   ui->stackedWidget->setCurrentWidget(m_pMainFrame);
   
   QObject::connect(m_pMainFrame,SIGNAL(addToolBarWidget(QToolBar*)),this,SIGNAL(addToolBarWidget(QToolBar*)));
   QObject::connect(m_pMainFrame,SIGNAL(removeToolBarWidget(QToolBar*)),this,SIGNAL(removeToolBarWidget(QToolBar*)));
   QObject::connect(m_pMainFrame,SIGNAL(editor_modificationChanged(bool)),this,SLOT(editor_modificationChanged(bool)));
}

MusicEditorForm::~MusicEditorForm()
{
   delete ui;
}

void MusicEditorForm::editor_modificationChanged(bool m)
{
   setModified(m);
   
   emit editor_modified(m);
}

void MusicEditorForm::onSave()
{
   CDesignerEditorBase::onSave();
   
   CFamiTrackerDoc* pDoc = (CFamiTrackerDoc*)m_pMainFrame->GetActiveDocument();

   pDoc->OnSaveDocument((TCHAR*)m_fileName.toLatin1().constData()); 
   
   setModified(false);
}
