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
#include <KNotification>
#include <QTimer>
#include <QProgressBar>

#include "constants.h"
#include "taskviewhandler.h"
#include "mainwindow.h"
#include "welcomewizard.h"
#include "settings.h"
#include "settingsdialog.h"
#include <httpconnectorfactory.h>

static const KUrl START_DIR_URL( "kfiledialog:///kvirustotal" );

MainWindow::MainWindow() {
	
	kDebug() << QString( "Starting up %1..." ).arg( General::APP_UI_NAME );
	networkManager = NULL;
	workloadConnector = NULL;
	wizard = NULL;
	setupUi( this );
	// Setup the toolbar
	KToolBar* toolbar = this->toolBar();
	KAction* fileAction = new KAction( KIcon( "document-open" ), i18n( "File..." ), this );
	fileAction->setHelpText( i18n( "Add a file" ) );
	fileAction->setShortcut( Qt::CTRL + Qt::Key_F );
	toolbar->addAction( fileAction );

	KAction* remoteFileAction = new KAction( KIcon( "download" ), i18n( "Remote file..." ), this );
	remoteFileAction->setHelpText( i18n( "Add a remote file" ) );
	remoteFileAction->setShortcut( Qt::CTRL + Qt::Key_R );
	toolbar->addAction( remoteFileAction );

	KAction* urlAction = new KAction( KIcon( "document-open-remote" ), i18n( "URL..." ), this );
	urlAction->setHelpText( i18n( "Add a URL" ) );
	urlAction->setShortcut( Qt::CTRL + Qt::Key_U );
	toolbar->addAction( urlAction );

	KAction* rescanAction = new KAction( KIcon( "view-refresh" ), i18n( "Re-scan" ), this );
	rescanAction->setHelpText( i18n( "Re-scan the selected finished task(s)" ) );
	rescanAction->setShortcut( Qt::CTRL + Qt::Key_S );
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

	KAction* quitAction = KStandardAction::quit( this, SLOT( closeRequested() ), this );
	quitAction->setHelpText( i18n( "Quit %1", General::APP_UI_NAME ) );
	toolbar->addSeparator();
	toolbar->addAction( quitAction );

	KMenuBar* menubar = menuBar();
	QMenu* fileMenu = menubar->addMenu( i18nc( "Menu name", "File" ) );
	fileMenu->addAction( fileAction );
	fileMenu->addAction( remoteFileAction );
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

	// Create a network access manager object
	networkManager = new QNetworkAccessManager( this );
	workloadConnector = HttpConnectorFactory::getHttpConnector( networkManager );

	// Setup the task table. If the serviceKey is empty, it will be updated later
	HttpConnectorFactory::loadHttpConnectorSettings();
	taskViewHandler = new TaskViewHandler( this, taskTableWidget, reportTableWidget );
	
	// Set up the workload progress bars
	setupWorkloadProgressBars();

	// Set up connections
	connect( fileAction, SIGNAL( triggered( bool ) ), this, SLOT( openFile() ) );
	connect( remoteFileAction, SIGNAL( triggered( bool ) ), this, SLOT( openRemoteFile() ) );
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
	connect( workloadConnector, SIGNAL( serviceWorkloadReady( ServiceWorkload ) ),
			 this, SLOT( onWorkloadReady( ServiceWorkload ) ) );
	
	// Show the minimum level and ask for the service workload
	ServiceWorkload workload = { ServiceWorkload::MIN_VALUE ,ServiceWorkload::MIN_VALUE };
	updateWorkloadProgressBars( workload );
	workloadConnector->onRetrieveServiceWorkload();
	
	// Validate version
	validateCurrentVersion();
	
	// Call the delayed connections and accept drops
	QTimer::singleShot( 1000, this, SLOT( delayedConnections() ) );
	setAcceptDrops( true );
}

MainWindow::~MainWindow() {
	if( wizard != NULL ) {
		delete wizard;
	}
	if( workloadConnector != NULL ) {
		workloadConnector->abort(); // Needed to prevent the app from crashing if started up and closed too fast.
		workloadConnector->deleteLater();
	}
	if( networkManager != NULL ) {
		networkManager->deleteLater();;
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
	settings->setTaskTableCols( taskTableCols );
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
	// Get the file name and submit it
	QString fileName = KFileDialog::getOpenFileName( START_DIR_URL );
	taskViewHandler->submitFile( QFile( fileName ) );
  
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

void MainWindow::openRemoteFile() {
	// Get the remote file's URL and submit it
	const QUrl url = promptUrl( i18n( "Remote file inputbox" ), i18n( "Please, enter a remote file's URL:" ) );
	if( !url.isEmpty() ) {
		taskViewHandler->submitRemoteFile( networkManager, url );
	}
}

void MainWindow::openUrl() {
	// Get the URL and submit it
	const QUrl url = promptUrl( i18n( "URL inputbox" ), i18n( "Please, enter a URL:" ) );
	if( !url.isEmpty() ) {
		taskViewHandler->submitUrl( url );
	}
}

QUrl MainWindow::promptUrl( const QString& title, const QString& message ) {
	bool ok;
	const QRegExp regex( "^((http)|(ftp))[s]?://[^..]+(\\.[^..]+)+$", Qt::CaseInsensitive );
	QRegExpValidator validator( regex, this );
	QString url = KInputDialog::getText( title, message, "http://", &ok, this, &validator );
	if( ok && !url.isEmpty() ) {
		kDebug() << "Entered URL is" << url;
		return QUrl( url );
	}
	return QUrl();
}


void MainWindow::setResultIcon( const QString& iconName, bool enabled ) {
	KIcon icon( iconName ); // Expected values are: security-high, security-medium, security-low
	resultIconLabel->setEnabled( enabled );
	resultIconLabel->setPixmap( icon.pixmap( resultIconLabel->size() ) );
}

void MainWindow::openPermanentLink() {
	QDesktopServices::openUrl( permanentLinkValue->text() );
}

void MainWindow::resizeEvent ( QResizeEvent* event ) {
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
	if( HttpConnectorFactory::getFileHttpConnectorCfg().serviceKeyRequired ||
		HttpConnectorFactory::getUrlHttpConnectorCfg().serviceKeyRequired ) {
		const QString serviceKey = Settings::self()->serviceKey();
		if( serviceKey.isEmpty() ) {
			KMessageBox::sorry( this, i18n( "No service key found! Thus, the application must be closed." ) );
			close();
		}
	}
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
	HttpConnectorFactory::loadHttpConnectorSettings();// Re-establish the protocol (HTTPS or HTTP)
	setProgressBarSecurityIcon();
}

void MainWindow::dragEnterEvent( QDragEnterEvent* event ) {
	kDebug() << "Receiving dragged object with mimeData" << event->mimeData()->text();
	QUrl url( event->mimeData()->text() );
	if( url.isValid() ) {
		const QString scheme = url.scheme();
		if( !scheme.isEmpty() && ( scheme == "file" || scheme.startsWith( "http" ) || scheme.startsWith( "ftp" ) ) ) {
			event->acceptProposedAction();
		}
	}
}

void MainWindow::dropEvent( QDropEvent* event ) {
	kDebug() << "Receiving dropped object with mimeData" << event->mimeData()->text();
	QUrl url( event->mimeData()->text() );
	const QString scheme = url.scheme();
	if( scheme == "file" ) {
		QFile file( url.path() );
		if( file.exists() ) {
			taskViewHandler->submitFile( file );
		}
	}
	else {
		static const QChar dot( '.' );
		if( url.path().contains( dot ) ) {
			taskViewHandler->submitRemoteFile( networkManager, url );
		}
		else {
			taskViewHandler->submitUrl( url );
		}
	}
}

void MainWindow::setupWorkloadProgressBars() {
	kDebug() << "Setting up workload progress bars...";

	// Antivirus progress bar
	QProgressBar* progressBar = new QProgressBar( this );
	progressBar->setObjectName( "antivirusProgressBar" );
	progressBar->setMinimum( ServiceWorkload::MIN_VALUE - 1 ); // Set one below the min not to show zero
	progressBar->setMaximum( ServiceWorkload::MAX_VALUE );
	progressBar->setToolTip( i18n( "Antivirus service workload" ) );
	progressBar->setFormat( i18n( "Antivirus %1", QString( "%p%" ) ) );
	progressBar->setMaximumSize( 120, 20 );
	statusBar()->setContentsMargins( 6, -2, 6, 0 );
	statusBar()->addPermanentWidget( progressBar );
	
	// Anti-phising progress bar 
	progressBar = new QProgressBar( this );
	progressBar->setObjectName( "antiphisingProgressBar" );
	progressBar->setMinimum( ServiceWorkload::MIN_VALUE - 1 ); // Set one below the min not to show zero
	progressBar->setMaximum( ServiceWorkload::MAX_VALUE );
	progressBar->setToolTip( i18n( "Anti-phising service workload" ) );
	progressBar->setFormat( i18n( "Anti-phising %1", QString( "%p%" ) ) );
	progressBar->setMaximumSize( 120, 20 );
	statusBar()->addPermanentWidget( progressBar );
	kDebug() << "Workload progress bars ready!";
	
	// Set the security icon
	setProgressBarSecurityIcon();
}

void MainWindow::setProgressBarSecurityIcon() {
	// Find the icon container or create a new one, if none exists
	QLabel* iconContainer = statusBar()->findChild< QLabel* >( "padlock" );
	if( iconContainer == NULL ) {
		iconContainer = new QLabel();
		iconContainer->setObjectName( "padlock" );
		statusBar()->addPermanentWidget( iconContainer );
	}
	
	// Update the icon accordingly
	KIcon padlock;
	if( Settings::self()->secureProtocol() ) {
		padlock = KIcon( "padlock-green" );
		iconContainer->setToolTip( i18n( "HTTPs is enabled, so data is being transmitted securely!" ) );
	}
	else {
		padlock = KIcon( "padlock-red" );
		iconContainer->setToolTip( i18n( "HTTPs is disabled, so data is being transmitted insecurely. Thus, it is highly recommended to enable it!" ) );
	}
	iconContainer->setPixmap( padlock.pixmap( 20, 20 ) );
}

void MainWindow::onWorkloadReady( ServiceWorkload workload  ) {
	// Update the progress bars with the given argument
	updateWorkloadProgressBars( workload );
	
	// Prepare the next connection to refresh the workload
	const int nextShot = 300;
	kDebug() << "Preparing" << nextShot << "seconds singleShot to refresh the workload bars...";
	QTimer::singleShot( nextShot * 1000, workloadConnector, SLOT( retrieveServiceWorkload() ) );
}

void MainWindow::updateWorkloadProgressBars( ServiceWorkload workload ) {
	kDebug() << "Updating workload progress bars...";
	static QProgressBar* const antivirusPb   = statusBar()->findChild< QProgressBar* >( "antivirusProgressBar" );
	static QProgressBar* const antiphisingPb = statusBar()->findChild< QProgressBar* >( "antiphisingProgressBar" );
	static QColor colors[] = { QColor( 72, 182, 0 ), QColor( 72, 182, 0 ), QColor( 255, 140, 0 ), QColor( 255, 140, 0 )
							 , QColor( 255, 69, 0 ), QColor( 255, 69, 0 ), QColor( 255, 0, 0 )  };
	
	// Update the antivirus progress bar
	int value = workload.file;
	antivirusPb->setValue( value );
	QPalette palette = antiphisingPb->palette();
	palette.setColor( QPalette::Highlight, colors[ value - 1 ] );
	antivirusPb->setPalette( palette );

	// Update the anti-phising progress bar
	value = workload.url;
	antiphisingPb->setValue( value );
	palette = antiphisingPb->palette();
	palette.setColor( QPalette::Highlight, colors[ value - 1 ] );
	antiphisingPb->setPalette( palette );
}

void MainWindow::showInfoNotificaton( const QString& msg ) {
	KNotification::event( KNotification::Notification, 
						  i18nc( "Application name", "%1 information", General::APP_UI_NAME ), 
						  msg, KIcon( General::APP_NAME ).pixmap( 48, 48 ) );
}

void MainWindow::showCompleteTaskNotificaton( const QString& msg, const QString& iconName ) {
	KNotification::event( KNotification::Notification, 
						  i18nc( "Application name", "%1 - Task completed", General::APP_UI_NAME ), 
						  msg, KIcon( iconName ).pixmap( 48, 48 ) );
}

void MainWindow::showWarningNotificaton( const QString& msg ) {
	KNotification::event( KNotification::Warning, 
						  i18nc( "Application name", "%1 warning", General::APP_UI_NAME ), 
						  msg, KIcon( "task-attention" ).pixmap( 48, 48 ), NULL, KNotification::CloseWhenWidgetActivated );
}

void MainWindow::showErrorNotificaton( const QString& msg ) {
	KNotification::event( KNotification::Warning, 
						  i18nc( "Application name", "%1 error", General::APP_UI_NAME ), 
						  msg, KIcon( "task-reject" ).pixmap( 48, 48 ), NULL, KNotification::Persistent );
}

void MainWindow::validateCurrentVersion() {
	// If no currentVersion, then it's the first run. Thus, show the welcome wizard
	if( Settings::self()->currentVersion().isEmpty() ) {
		// This connection will guarantee that the task table's columns adapt the size of the window
		connect( this, SIGNAL( resizeWidth( int ) ), taskViewHandler, SLOT( tableWidthChanged( int ) ) );
		showWelcomeWizard();
	}
	// If the serviceKey is required and it's empty, show the welcome wizard
	else if( ( HttpConnectorFactory::getFileHttpConnectorCfg().serviceKeyRequired ||
			   HttpConnectorFactory::getUrlHttpConnectorCfg().serviceKeyRequired  ) &&
			 Settings::self()->serviceKey().isEmpty() ) {
		KMessageBox::information( this, 
									i18n( "The current connector requires a service key, but none has been found. Thus, the welcome wizard will be shown so you can now either set it up or change the connector to be used."),
									i18n( "Service key required" ) );
		// This connection will guarantee that the task table's columns adapt the size of the window
		connect( this, SIGNAL( resizeWidth( int ) ), taskViewHandler, SLOT( tableWidthChanged( int ) ) );
		showWelcomeWizard();
	}
	// Show the a message to warn the user that a new connector has been implemented
	else if ( isPreviousVersionTo( (uchar)0, (uchar)20, (uchar)0 ) ) {
		if( KMessageBox::questionYesNo( this, 
									    i18nc( "Application name", 
											   "The new version of %1 has a new connector. Do you want the welcome wizard to be shown so you can change it?",
											   General::APP_UI_NAME ), 
										i18n( "Show welcome wizard" ) ) == KMessageBox::Yes ) {
			// This connection will guarantee that the task table's columns adapt the size of the window
			connect( this, SIGNAL( resizeWidth( int ) ), taskViewHandler, SLOT( tableWidthChanged( int ) ) );
			showWelcomeWizard();
		}
		else {
			// Leave the API connector
			Settings::self()->setHttpConnectorType( HttpConnectorType::API_HTTPCONNECTOR );
			Settings::self()->writeConfig();
		}
	} 
}

bool MainWindow::isPreviousVersion() {
	return isPreviousVersionTo( General::APP_VERSION_MAJOR, General::APP_VERSION_MINOR, General::APP_VERSION_BUGFIX );
}

bool MainWindow::isPreviousVersionTo( uchar major, uchar minor, uchar bugfix ) {
	const QString version = Settings::self()->currentVersion();
	if( !version.isEmpty() ) {
		QStringList versions = version.split( ".", QString::SkipEmptyParts );
		// Major
		if( versions.size() > 0 ) {
			bool ok = false;
			int aux = versions[ 0 ].toInt( &ok );
			if( ok && aux >= major ) {
				// Minor
				if( versions.size() > 1 ) {
					aux = versions[ 1 ].toInt( &ok );
					if( ok && aux >= minor ) {
						if( versions.size() > 2 ) {
							aux = versions[ 2 ].toInt( &ok );
							if( ok && aux >= bugfix ) {
								return false;
							}
						}
					}
				}
			}
		}
	}
	kDebug() << "Previous version detected!";
	return true;
}

#include "mainwindow.moc"