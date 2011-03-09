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

#include <QUrl>
#include <KLocalizedString>
#include <QLabel>
#include <KLineEdit>
#include <QRadioButton>
#include <QVBoxLayout>
#include <KIcon>
#include <QMovie>
#include <QDesktopServices>
#include <KStandardDirs>
#include <KDebug>

#include "settings.h"
#include "welcomepage.h"
#include "constants.h"

static const QString TEST_SERVICE_KEY_SCAN_ID( "4feed2c2e352f105f6188efd1d5a558f24aee6971bdf96d5fdb19c197d6d3fad" );

WelcomePage::WelcomePage( WelcomeWizard* wizard ) : QWizardPage( wizard ) {
	this->wizard = wizard;
	connector = NULL; // Initialized in the startServiceKeyValidation() method
	serviceKeyLineEdit = NULL; // Initialized in the createGetServiceKeyPage() method
	validationStatus   = NULL; // Initialized in the createCheckServiceKeyPage() method
	validationKeyOk    = NULL; // Initialized in the createCheckServiceKeyPage() method
}

WelcomePage::~WelcomePage() { 
	if( connector != NULL ) {
		 // We reach this point, it's quite like the deletion of the connector crashes!! It should have been done already!
		kDebug() << "~WelcomePage() pageId=" << wizard->currentId();
		freeConnector();
	}
}

void WelcomePage::freeConnector() {
	if( connector != NULL ) {
		kDebug() << "Deleting connector";
		connector->deleteLater();
		connector = NULL;
	}
}

QWizardPage* WelcomePage::createIntroPage( WelcomeWizard* wizard ) {
	kDebug() << "Creating the intro page...";
	WelcomePage* page = new WelcomePage( wizard );
	page->setTitle( i18nc( "Application name", "<h2>Welcome to %1!</h2>", General::APP_UI_NAME ) );

	QLabel* subTitle = new QLabel( i18nc( "Application name", "<h3>What's %1?</h3>", General::APP_UI_NAME ), page ) ;
	subTitle->setMargin( 10 );

	QString text = i18nc( "Application name", "%1 is an online-based antivirus and anti-phising tool. \
It allows you to submit files that will be analysed by more \
than 40 up-to-dated antivirus. Besides, it will accept URLs \
that will be tested against the main anti-phising sytems.<br><br>\
Although %1 is a powerful tool, remember that <b>it is not \
intended as a replacement of a full-fledged antivirus program.</b><br><br>\
Please, click on the 'Next' button to set up the application.", General::APP_UI_NAME );
	QLabel* label = new QLabel( text, page );
	label->setAlignment( Qt::AlignJustify | Qt::AlignVCenter );
	label->setWordWrap( true );

	QVBoxLayout* layout = new QVBoxLayout( page );
	layout->addWidget( subTitle );
	layout->addWidget( label );
	page->setLayout( layout );
	kDebug() << "Intro page done!";
	return page;
}

QWizardPage* WelcomePage::createGetServiceKeyPage( WelcomeWizard* wizard ) {

	WelcomePage* page = new WelcomePage( wizard );
	page->setTitle( i18n( "%1Configuration wizard - Step 1/3%2" ).arg( "<h2>").arg( "</h2>" ) );
	QLabel* subTitle = new QLabel( i18n( "%1Enter your VirusTotal Community Key%2").arg( "<h3>").arg( "</h3>" ), page ) ;
	subTitle->setMargin( 10 );

static const QString VIRUSTOTAL_REGISTRATION_PAGE="http://www.virustotal.com/vt-community/register.html";
	QString text = i18nc( "Application name",  "Since %1 relies on the services provided by \
<a href='http://virustotal.com'>VirusTotal (c)</a>, a key is required to access its API. \
If you still don't have one, you can get one free of charge by registering at <a href='%2'>%2</a>. \
Once you have signed up to VT Community (using the sign in box at the top left hand side of the page), \
you will find your personal API key in the inbox of your account (sign in and drop down the <i>My account</i> menu). \
Now, just copy and paste it below:", General::APP_UI_NAME, VIRUSTOTAL_REGISTRATION_PAGE );
	QLabel* label = new QLabel( text, page );
	label->setAlignment( Qt::AlignJustify | Qt::AlignVCenter );
	label->setWordWrap( true );
	label->setOpenExternalLinks( true );
	connect( label, SIGNAL( linkActivated( QString ) ), page, SLOT( openLink( QString ) ) );

	QLabel* keyLabel = new QLabel( i18n( "Service &key:" ), page );
	KLineEdit* keyText = page->serviceKeyLineEdit = new KLineEdit( page );
	keyLabel->setBuddy( keyText );
	keyText->setFocus();
	keyText->setClearButtonShown( true );

	QVBoxLayout* layout = new QVBoxLayout( page ); // Must be created before keyLayout to work
	QHBoxLayout* keyLayout = new QHBoxLayout( page );
	keyLayout->setContentsMargins( 0, 40, 0, 0 );
	keyLayout->addWidget( keyLabel );
	keyLayout->addWidget( keyText );
	layout->addWidget( subTitle );
	layout->addWidget( label );
	layout->addLayout( keyLayout );
	page->setLayout( layout );

	// Set mandatory fields
//FIXME: When the keyText is not empty by default, is shoudl allow to continue
	page->setMandatoryField( "keyText*", keyText );

	return page;
}

QWizardPage* WelcomePage::createCheckServiceKeyPage( WelcomeWizard* wizard ) {
	WelcomePage* page = new WelcomePage( wizard );
	page->setTitle( i18n( "%1Configuration wizard - Step 2/3%2" ).arg( "<h2>").arg( "</h2>" ) );
	QLabel* subTitle = new QLabel( i18n( "%1Service key test%2").arg( "<h3>").arg( "</h3>" ), page ) ;
	subTitle->setMargin( 10 );

	QString text = i18n( "<br><br>%1 is verifying that the key you entered is valid. Please, wait for a few seconds...", General::APP_UI_NAME );
	QLabel* label = new QLabel( text, page );
	label->setAlignment( Qt::AlignJustify | Qt::AlignVCenter );
	label->setWordWrap( true );
	
	QLabel* labelPixmap = new QLabel( page );
	labelPixmap->setAlignment( Qt::AlignCenter | Qt::AlignVCenter );
	const QString moviePath = KStandardDirs::locate( "data", "kvirustotal/pics/kvirustotal-connecting.gif" );
	kDebug() << "Movie path is " <<	moviePath;
	QMovie* movie = new QMovie( moviePath, "gif", page );
	movie->start();
	labelPixmap->setMovie( movie );
	labelPixmap->setMargin( 20 );
	
	QVBoxLayout* layout = new QVBoxLayout( page ); // Must be created before keyLayout to work
	QHBoxLayout* statusLayout = new QHBoxLayout( page );
	QLabel* statusLabel = new QLabel( i18n( "<b>Status:</b>" ), page );
	QLabel* status = page->validationStatus = new QLabel( i18n( "Connecting..." ) );
	statusLabel->setBuddy( status );
	statusLayout->addWidget( statusLabel );
	statusLayout->addWidget( status );
	statusLayout->setContentsMargins( 0, 10, 0, 0 );
	statusLayout->setAlignment( Qt::AlignLeft );
	
	// We'll use a very light centinel object that will tell us when the 'Next' button must be enabled
	QAbstractButton * operationOkCentinel = page->validationKeyOk = new QRadioButton( page );
	operationOkCentinel->setHidden( true );
	
	layout->addWidget( subTitle );
	layout->addWidget( label );
	layout->addWidget( labelPixmap );
	layout->addLayout( statusLayout );
	layout->addWidget( operationOkCentinel );
	page->setLayout( layout );
	
	// Register some files. Only the one ending in * are mandatory
	page->setMandatoryField( "status", status );
	page->setMandatoryField( "resultLabel*", operationOkCentinel );
	
	return page;
}

QWizardPage* WelcomePage::createConclusionPage( WelcomeWizard* wizard ) {
	WelcomePage* page = new WelcomePage( wizard );
	page->setTitle( i18n( "%1Configuration wizard - Step 3/3%2" ).arg( "<h2>").arg( "</h2>" ) );
	QLabel* subTitle = new QLabel( i18n( "%1Configuration completed successfully%2").arg( "<h3>").arg( "</h3>" ), page ) ;
	subTitle->setMargin( 10 );

	QString text = i18n( "<br><br>%1 is ready to work! Please, click on the 'Finish' button to close this wizard.", General::APP_UI_NAME );
	QLabel* label = new QLabel( text, page );
	label->setAlignment( Qt::AlignJustify | Qt::AlignVCenter );
	label->setWordWrap( true );

	QVBoxLayout* layout = new QVBoxLayout( page );
	layout->addWidget( subTitle );
	layout->addWidget( label );
	page->setLayout( layout );
	page->setCommitPage( true ); // 'Back' button disabled
	page->setFinalPage( true ); 
	return page;
}

void WelcomePage::startServiceKeyValidation() {
	QString key = serviceKey();
	kDebug() << "Validating the user-provided key " << key;
	QNetworkAccessManager* manager = wizard->networkAccessManager();
	freeConnector(); // Should do nothing, but keep it just in case
	connector = new HttpConnector( wizard->networkAccessManager(), key );
	
	// Connect the connector needed signals
	kDebug() << "WelcomePageId=" << wizard->currentId();
	connect( connector, SIGNAL( reportReady( AbstractReport*const ) ),
		     this, SLOT( testReportReceived( AbstractReport*const ) ) );
	connect( connector, SIGNAL( invalidServiceKeyError() ),
			 this, SLOT( testReportInvalidKey() ) );
	connect( connector, SIGNAL( errorOccurred( QString ) ),
			 this, SLOT( testReportFailed( QString ) ) );
	
	// Try to connect the to API
	setValidationStatus( i18n( "Validating your key..." ) );
	connector->retrieveFileReport( TEST_SERVICE_KEY_SCAN_ID );
}

void WelcomePage::testReportReceived( AbstractReport*const report ) {
//kDebug() << "testReportReceived() FileReport* report=" << report;
	if( report != NULL ) {
		const ServiceReplyResult::ServiceReplyResultEnum result = report->getServiceReplyResult();
		if( result != ServiceReplyResult::INVALID_SERVICE_KEY ) {
			setValidationStatus( i18n( "Operation done successfully!" ) );
			if( validationKeyOk != NULL ) {
				validationKeyOk->setChecked( true ); // This will enable the "Next" button
			}
		}
		else {
			testReportInvalidKey();
			if( validationKeyOk != NULL ) {
				validationKeyOk->setChecked( false ); // This will disable the "Next" button
			}
		}
		
		// Free the report memory as it won't be used anymore
		delete report;
	}
	else {
		testReportFailed( i18n( "Unknown error" ) );
		if( validationKeyOk != NULL ) {
			validationKeyOk->setChecked( false ); // This will disable the "Next" button
		}
	}
}

void WelcomePage::testReportInvalidKey() {
	testReportFailed( i18n( "Your key is invalid!" ) );
}

void WelcomePage::testReportFailed( const QString& message ) {
	setValidationStatus( i18n( "ERROR: \"%1\". Please, go back and try again.", message ) );
}

void WelcomePage::setValidationStatus( const QString& status ) {
	if( validationStatus != NULL ) {
		validationStatus->setText( status );
	}
}

void WelcomePage::setMandatoryField( const QString& name, QWidget* widget, const char* property, const char* changedSignal ) {
	registerField( name, widget, property, changedSignal );
}

void WelcomePage::openLink( const QString& link ) const {
	kDebug() << "Opening link " << link;
	QDesktopServices::openUrl( QUrl( link )  );
}

QString WelcomePage::serviceKey() const {
	QVariant key = field( "keyText" );
	if( key.isValid() ) {
		return key.toString();
	}
	return QString::null;
}

void WelcomePage::initializePage() {
    QWizardPage::initializePage();
	if( wizard->currentId() == WelcomePageConst::GET_SERVICE_KEY_PAGE ) {
		// Set the current user's service key, if available
		const QString serviceKey = Settings::self()->serviceKey();
		if( !serviceKey.isEmpty() && this->serviceKeyLineEdit != NULL ) {
			this->serviceKeyLineEdit->setText( serviceKey );
		}
	}
	else if( wizard->currentId() == WelcomePageConst::CHECK_SERVICE_KEY_PAGE ) {
		startServiceKeyValidation();
	}
}

bool WelcomePage::validatePage() {
	// If we are in the last page
kDebug() << wizard->currentId();
	const int pageId = wizard->currentId();
	if( pageId == WelcomePageConst::CHECK_SERVICE_KEY_PAGE ) {
		freeConnector(); // Delete the connector when pressing 'Next'
	}
	else if( pageId == WelcomePageConst::CONCLUSION_PAGE ) {
		QString key = serviceKey();
		kDebug() << "Storing user service key " << key;
		Settings::self()->setServiceKey( key );
		Settings::self()->writeConfig();
	}
	return true;
}

void WelcomePage::cleanupPage() {
    QWizardPage::cleanupPage();
	const int pageId = wizard->currentId();
	if( pageId == WelcomePageConst::CHECK_SERVICE_KEY_PAGE ) {
		kDebug() << pageId;
		freeConnector(); // Delete the connector when pressing 'Back' or 'Cancel'
	}
}

#include "welcomepage.moc"