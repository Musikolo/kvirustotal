/*
    Copyright (c) 2011 Carlos López Sánchez <musikolo{AT}hotmail[DOT]com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <KFileDialog>
#include <KAction>
#include <KToolBar>
#include <KMenuBar>
#include <KStatusBar>
#include <KInputDialog>
#include <QRegExp>
#include <QRegExpValidator>
#include <KDebug>
#include <KMessageBox>
#include <QDesktopServices>
#include <KHelpMenu>
#include <KCmdLineArgs>
#include <KConfigDialog>
#include <QSplitter>
#include <KStandardAction>

#include "constants.h"
#include "httpconnector.h"
#include "taskviewhandler.h"
#include "mainwindow.h"
#include "welcomewizard.h"
#include "settings.h"
#include "settingsdialog.h"

static const KUrl START_DIR_URL( "kfiledialog:///kvirustotal" );

MainWindow::MainWindow()
{
	kDebug() << QString( "Starting up %1..." ).arg( General::APP_UI_NAME );
	wizard = NULL;
	setupUi( this );
	// Setup the toolbar
	KToolBar* toolbar = this->toolBar();
	KAction* fileAction = new KAction( KIcon( "document-open" ), i18n( "File..." ), this );
	fileAction->setHelpText( i18n( "Add a file" ) );
	fileAction->setShortcut( Qt::CTRL + Qt::Key_F );
	toolbar->addAction( fileAction );

	KAction* urlAction = new KAction( KIcon( "document-open-remote" ), i18n( "URL..." ), this );
	urlAction->setHelpText( i18n( "Add a URL" ) );
	urlAction->setShortcut( Qt::CTRL + Qt::Key_U );
	toolbar->addAction( urlAction );

	KAction* rescanAction = new KAction( KIcon( "view-refresh" ), i18n( "Re-scan" ), this );
	rescanAction->setHelpText( i18n( "Re-scan the selected finished task(s)" ) );
	rescanAction->setShortcut( Qt::CTRL + Qt::Key_R );
	toolbar->addSeparator();
	toolbar->addAction( rescanAction );

	// Add "Clean finished tasks" action
	KAction* cleanFinishedAction = new KAction( KIcon( "run-build-clean" ), i18n( "Clean" ), this );
	cleanFinishedAction->setHelpText( i18n( "Clean all finished tasks" ) );
	cleanFinishedAction->setShortcut( Qt::CTRL + Qt::Key_D );
	toolbar->addAction( cleanFinishedAction );

	KAction* abortAction = new KAction( KIcon( "dialog-cancel" ), i18n( "Abort..." ), this );
	abortAction->setHelpText( i18n( "Abort the selected tasks" ) );
	abortAction->setShortcut( Qt::CTRL + Qt::Key_B );
	toolbar->addAction( abortAction );

//TODO: Temporary button - Just for development use
/*
KAction * submitAction = new KAction( KIcon( "bookmark-new-list" ), "Submit", this );
submitAction->setHelpText( i18n( "Submit" ) );
submitAction->setShortcut( Qt::CTRL + Qt::Key_S );
toolbar->addSeparator();
toolbar->addAction( submitAction );

connect( submitAction, SIGNAL( triggered( bool ) ), this, SLOT( submit() ) );
*/

	KAction* quitAction = KStandardAction::quit( this, SLOT( closeRequested() ), this );
	quitAction->setHelpText( i18n( "Quit %1", General::APP_UI_NAME ) );
	toolbar->addSeparator();
	toolbar->addAction( quitAction );

	KMenuBar* menubar = menuBar();
	QMenu* fileMenu = menubar->addMenu( i18nc( "Menu name", "File" ) );
	fileMenu->addAction( fileAction );
	fileMenu->addAction( urlAction );
	fileMenu->addAction( quitAction );

	// Edit menu
	QMenu* editMenu = menubar->addMenu( i18n( "Edit" ) );
	KAction* selectAllAction = new KAction( i18n( "Select All" ), this );
	selectAllAction->setHelpText( i18n( "Select all tasks") );
	selectAllAction->setShortcut( Qt::CTRL + Qt::Key_A );
	editMenu->addAction( selectAllAction );

	KAction* invertSelectionAction = new KAction( i18n( "Invert Selection" ), this );
	invertSelectionAction->setHelpText( i18n( "Inverts the selection of tasks") );
	invertSelectionAction->setShortcut( Qt::CTRL + Qt::Key_I );
	editMenu->addAction( invertSelectionAction );

	KAction* settingsAction = new KAction( i18n( "Settings..." ), this );
	settingsAction->setHelpText( i18n( "Show the settings dialog" ) );
	settingsAction->setShortcut( Qt::CTRL + Qt::Key_T );
	editMenu->addSeparator();
	editMenu->addAction( settingsAction );
	
	// Action menu
	QMenu* actionMenu = menubar->addMenu( i18n( "Action" ) );
	actionMenu->addAction( rescanAction );
	actionMenu->addAction( cleanFinishedAction );
	actionMenu->addAction( abortAction );


	// Help menu
	QMenu* helpMenu = ( new KHelpMenu( this, KCmdLineArgs::aboutData(), false ) )->menu();
	helpMenu->addSeparator();
 	helpMenu->addAction( i18n( "Welcome wizard..." ), this, SLOT( showWelcomeWizard() ) );
	menubar->addMenu( helpMenu );
	
	// Set the frame literals
	scanAnalysisDateLabel->setText( i18n( "Scan analysis date:" ) );
	permanentLinkLabel->setText( i18n( "Permanent link:" ) );

	// Frame issues - set the result icon label
	this->resultIconLabel = new QLabel( frame );
	const QSize iconSize(48, 48);
	resultIconLabel->setMaximumSize( iconSize );
	resultIconLabel->resize( iconSize );
	resultIconLabel->setEnabled( false );
	KIcon icon( "security-high" ); // See setResultIcon() method
	resultIconLabel->setPixmap( icon.pixmap( iconSize ) );
	frameMainLayout->addWidget( resultIconLabel, Qt::AlignRight );
	frame->setLayout( frameMainLayout ); // Needed to get the layout resizes as the frame does
	permanentLinkValue->setToolTip( i18n( "Click to view detailed report" ) );

	// Needed to get the QSplitter to expand
	QVBoxLayout* layout = new QVBoxLayout( centralwidget );
	layout->addWidget( splitter );

	statusBar()->showMessage( i18n( "Ready..." ) );
//	setWindowIcon( KIcon( General::APP_NAME ) );
	setWindowTitle( i18n( "Online antivirus and anti-phishing tool - %1", General::APP_UI_NAME ) );
	
	// Restore the main window state, if available
	const Settings* settings = Settings::self();
	const QSize size = settings->mainWindowSize(); 
	if( size.isValid() && size.width() > 100 && size.height() > 100 )  {
		move( settings->mainWindowPos() );
		resize( size );
	}
	else {
		showMaximized();
	}

	// Restore its state too, if available
	QList< int > splitterState = settings->splitterState();
	if( !splitterState.isEmpty() ) {
		splitter->setSizes( splitterState );
	}	
	
	// Initialize the network manager
	networkManager = new QNetworkAccessManager( this );

	// Load the user's service key
	const QString serviceKey = settings->serviceKey();
	if( serviceKey.isEmpty() ) {
		showWelcomeWizard();
	}

	// Setup the task table. If the serviceKey is empty, it will be updated later
	HttpConnector::loadSettings();
	taskViewHandler = new TaskViewHandler( this, taskTableWidget, reportTableWidget, networkManager, serviceKey );

	// Set up connections
	connect( fileAction, SIGNAL( triggered( bool ) ), this, SLOT( openFile() ) );
	connect( urlAction, SIGNAL( triggered( bool ) ), this, SLOT( openUrl() ) );
	connect( abortAction, SIGNAL( triggered( bool ) ), taskViewHandler, SLOT( abortSelectedTask() ) );
	connect( permanentLinkValue, SIGNAL( leftClickedUrl() ), this, SLOT( openPermanentLink() ) );
	connect( selectAllAction, SIGNAL( triggered( Qt::MouseButtons, Qt::KeyboardModifiers ) ),
			 taskViewHandler, SLOT( selectAll() ) );
	connect( invertSelectionAction, SIGNAL( triggered( Qt::MouseButtons, Qt::KeyboardModifiers ) ),
			 taskViewHandler, SLOT( invertSelection() ) );
	connect( settingsAction, SIGNAL( triggered(Qt::MouseButtons,Qt::KeyboardModifiers ) ),
			 this, SLOT( showSettingsDialog() ) );
	connect( rescanAction, SIGNAL( triggered( bool ) ), taskViewHandler, SLOT( rescanTasks() ) );
	connect( cleanFinishedAction, SIGNAL( triggered( bool ) ), taskViewHandler, SLOT( clearFinishedRows() ) );
	
	// Finally call the delayed connections. 
	QTimer::singleShot( 1000, this, SLOT( delayedConnections() ) );
}

MainWindow::~MainWindow() {
	if( networkManager ) {
		delete networkManager;
	}
	if( wizard != NULL ) {
		delete wizard;
	}
}

void MainWindow::delayedConnections() {
	// Needed to be delayed, since MainWindow will throw the resizeWidth signal at construction time
	connect( this, SIGNAL( resizeWidth( int ) ), taskViewHandler, SLOT( tableWidthChanged( int ) ) );
}

bool MainWindow::closeRequested() {
	if( taskViewHandler->isUnfinishedTasks() &&
		KMessageBox::warningContinueCancel( centralWidget(),
											i18n( "There is one or more tasks in course and will be aborted. Do you really want to quit?" ),
										    i18n( "Tasks in course" ),
											KStandardGuiItem::cont(),
											KStandardGuiItem::cancel(),
											QString::null,
											KMessageBox::Dangerous ) != KMessageBox::Continue ) {
		return false;
	}
	
	// Save current config
	Settings* settings = Settings::self();
	settings->setCurrentVersion( General::APP_VERSION );
	settings->setMainWindowPos( pos()  );
	settings->setMainWindowSize( size() );
	QList< int > taskTableCols;
	for( int i = 0; i < taskTableWidget->columnCount(); i++ ) {
		taskTableCols.append( taskTableWidget->columnWidth( i ) );
	}
	settings->setReportTableCols( taskTableCols );
	QList< int > reportTableCols;
	for( int i = 0; i < reportTableWidget->columnCount(); i++ ) {
		reportTableCols.append( reportTableWidget->columnWidth( i ) );
	}
	settings->setReportTableCols( reportTableCols );
	settings->setSplitterState( splitter->sizes() );
	kDebug() << "Storing user config...";
	settings->writeConfig();
	return close();
}

void MainWindow::openFile() {

  QString fileName = KFileDialog::getOpenFileName( START_DIR_URL );
  taskViewHandler->submitFile( fileName );
//  QFile( fileName ).p
//TODO: It might be interesting to be able to get files from a network location
/*  QString tmpFile;
  if(KIO::NetAccess::download( fileName, tmpFile, this)) {
    QFile file(tmpFile);
    file.open(QIODevice::ReadOnly);
    textArea->setPlainText(QTextStream(&file).readAll());
    KIO::NetAccess::removeTempFile(tmpFile);
  }
  else {
    KMessageBox::error( this, KIO::NetAccess::lastErrorString() );
  }
*/

}

void MainWindow::openUrl() {
	bool ok;
	const QRegExp regex( "^((http)|(ftp))[s]?://[^..]+(\\.[^..]+)+$", Qt::CaseInsensitive );
	QRegExpValidator * const validator = new QRegExpValidator( regex, this );
//TODO: Remove this after testing
	QString url = KInputDialog::getText( i18n( "URL inputbox" ), i18n( "Please, enter a URL:" ), "http://", &ok, this, validator );
//	QString url = KInputDialog::getText( i18n( "URL inputbox" ), i18n( "Please, enter a URL:" ), "http://blog.ben2367.fr/wp-includes/images/smilies/icon_wink.gif", &ok, this, validator );
	if( ok && !url.isEmpty() ) {
		kDebug() << "Entered URL is" << url;
		taskViewHandler->submitUrl( url );
	}
	validator->deleteLater();
}

//TODO: Just for testing
/*
void MainWindow::submit() {
	taskViewHandler->submitFile( "/home/musikolo/workspace-qt/kvirustotal/test/fileReportTest.json" );
}
*/
void MainWindow::setResultIcon( const QString& iconName, bool enabled ) {
	KIcon icon( iconName ); // Expected values are: security-high, security-medium, security-low
	resultIconLabel->setEnabled( enabled );
	resultIconLabel->setPixmap( icon.pixmap( resultIconLabel->size() ) );
}

void MainWindow::openPermanentLink() {
	QDesktopServices::openUrl( permanentLinkValue->text() );
}

void MainWindow::resizeEvent ( QResizeEvent * event ) {
	int width = event->size().width();
	int oldWidth = event->oldSize().width();
	if( width != oldWidth ) {
		kDebug() << QString("resizeEvent() - newWitdh:%1 oldWidth:%2").arg( width ).arg( oldWidth );
		emit( resizeWidth( width ) );
	}
}

void MainWindow::showWelcomeWizard() {
	if( wizard != NULL ) {
		wizard->deleteLater();
		wizard = NULL;
	}
	wizard = new WelcomeWizard( networkManager, centralwidget );
	wizard->show();
	connect( wizard, SIGNAL( finished( int ) ), this, SLOT( wizardFinished( int ) ) );
}

void MainWindow::wizardFinished( int result ) {
	kDebug() << "result=" << result;
	const QString serviceKey = Settings::self()->serviceKey();
	if( serviceKey.isEmpty() ) {
		KMessageBox::sorry( this, i18n( "No service key found! Thus, the application must be closed." ) );
		close();
	}
	
	// Refresh the service key
	taskViewHandler->setServiceKey( serviceKey );
}	

void MainWindow::showSettingsDialog() {
	// If we have an instance created, do nothing
	if ( KConfigDialog::showDialog( "settings" ) ) {
		return; 
	}
	
	// KConfigDialog didn't find an instance of this dialog, so lets create it : 
	KConfigDialog* dialog = new KConfigDialog( this, "settings", Settings::self() ); 
	SettingsDialog* settingsDialog =  new SettingsDialog( this );
	dialog->addPage( settingsDialog, i18n( "General" ), "preferences-other" );
	dialog->show();

	// If a change takes place, reload the affected settings
	connect( dialog, SIGNAL( settingsChanged( const QString& ) ), this, SLOT( settingsChanged() ) );
}

void MainWindow::settingsChanged() {
	HttpConnector::loadSettings(); // Re-establish the protocol (HTTPS or HTTP)
}

#include "mainwindow.moc"