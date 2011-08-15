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
#include <QButtonGroup>
#include <KDebug>

#include "settings.h"
#include "welcomepage.h"
#include "constants.h"
#include <api/apihttpconnector.h>

static const QString TEST_SERVICE_KEY_SCAN_ID( "4feed2c2e352f105f6188efd1d5a558f24aee6971bdf96d5fdb19c197d6d3fad" );

WelcomePage::WelcomePage( WelcomeWizard* wizard ) : QWizardPage( wizard ) {
	this->wizard = wizard;
	connector = NULL; // Initialized in the startServiceKeyValidation() method
	serviceKeyLineEdit = NULL; // Initialized in the createGetServiceKeyPage() method
	validationStatus   = NULL; // Initialized in the createCheckServiceKeyPage() method
	validationKeyOk    = NULL; // Initialized in the createCheckServiceKeyPage() method
	connectorValidation    = NULL; // Initialized in the createConnectorSelectionPage() method
	connectorType = HttpConnectorType::WEB_HTTPCONNECTOR;
	connectorChosen = false;
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

QWizardPage* WelcomePage::createConnectorSelectionPage( WelcomeWizard* wizard ) {

	WelcomePage*const page = new WelcomePage( wizard );
	page->setTitle( i18n( "%1Configuration wizard - Step 1/4%2" ).arg( "<h2>").arg( "</h2>" ) );
	QLabel* subTitle = new QLabel( i18n( "%1Connector selection%2").arg( "<h3>").arg( "</h3>" ), page ) ;
	subTitle->setMargin( 10 );

	QHBoxLayout*const featureLabelLayout = new QHBoxLayout();
	featureLabelLayout->setContentsMargins( 0, 0, 0, 10 );
	featureLabelLayout->addWidget( new QLabel( i18n( "%1Features%2" ).arg( "<b>" ).arg( "</b>") ) );
	
	QVBoxLayout*const featureLayout = new QVBoxLayout();
	featureLayout->addLayout( featureLabelLayout );
	featureLayout->addWidget( new QLabel( i18n( "%1Service key:%2" ).arg( "<b>" ).arg( "</b>") ) );
	featureLayout->addWidget( new QLabel( i18n( "%1Instant reports:%2" ).arg( "<b>" ).arg( "</b>") ) );
	featureLayout->addWidget( new QLabel( i18n( "%1Connection limit:%2" ).arg( "<b>" ).arg( "</b>") ) );
	featureLayout->addWidget( new QLabel( i18n( "%1Priorized service:%2" ).arg( "<b>" ).arg( "</b>") ) );
	featureLayout->addWidget( new QLabel( i18n( "%1HTTP supported:%2" ).arg( "<b>" ).arg( "</b>") ) );
	featureLayout->addWidget( new QLabel( i18n( "%1HTTPs supported:%2" ).arg( "<b>" ).arg( "</b>") ) );
	
	QRadioButton*const apiRadio = new QRadioButton();
	apiRadio->setFixedWidth( 25 );
	QLabel*const apiCheckBoxLabel = new QLabel( i18n( "%1API connector%2" ).arg( "<b>" ).arg( "</b>") );
	apiCheckBoxLabel->setBuddy( apiRadio );
	QHBoxLayout*const apiCheckboxLayout = new QHBoxLayout();
	apiCheckboxLayout->setContentsMargins( 0, 0, 0, 10 );
	apiCheckboxLayout->addWidget( apiRadio );
	apiCheckboxLayout->addWidget( apiCheckBoxLabel, 0, Qt::AlignLeft );
	
	QVBoxLayout*const apiLayout = new QVBoxLayout();
	apiLayout->addLayout( apiCheckboxLayout );
	apiLayout->addWidget( new QLabel( i18n( "Required (freely available)" ) ), 0, Qt::AlignCenter );
	apiLayout->addWidget( new QLabel( i18n( "Yes" ) ), 0, Qt::AlignCenter );
	apiLayout->addWidget( new QLabel( i18n( "20 every 5 minutes" ) ), 0, Qt::AlignCenter );
	apiLayout->addWidget( new QLabel( i18n( "No" ) ), 0, Qt::AlignCenter );
	apiLayout->addWidget( new QLabel( i18n( "Yes" ) ), 0, Qt::AlignCenter );
	apiLayout->addWidget( new QLabel( i18n( "Yes" ) ), 0, Qt::AlignCenter );

	QRadioButton*const webRadio = new QRadioButton();
	webRadio->setFixedWidth( 25 );
	QLabel*const webCheckBoxLabel = new QLabel( i18n( "%1Web connector%2" ).arg( "<b>" ).arg( "</b>") );
	webCheckBoxLabel->setBuddy( webRadio );
	QHBoxLayout*const webCheckboxLayout = new QHBoxLayout();
	webCheckboxLayout->setContentsMargins( 0, 0, 0, 10 );
	webCheckboxLayout->addWidget( webRadio );
	webCheckboxLayout->addWidget( webCheckBoxLabel, 0, Qt::AlignLeft );
	
	QVBoxLayout*const webLayout = new QVBoxLayout();
	webLayout->addLayout( webCheckboxLayout );
	webLayout->addWidget( new QLabel( i18n( "Not used" ) ), 0, Qt::AlignCenter );
	webLayout->addWidget( new QLabel( i18n( "No" ) ), 0, Qt::AlignCenter );
	webLayout->addWidget( new QLabel( i18n( "Unlimited" ) ), 0, Qt::AlignCenter );
	webLayout->addWidget( new QLabel( i18n( "Yes" ) ), 0, Qt::AlignCenter  );
	webLayout->addWidget( new QLabel( i18n( "Yes" ) ), 0, Qt::AlignCenter  );
	webLayout->addWidget( new QLabel( i18n( "Yes" ) ), 0, Qt::AlignCenter  );
	
	// We'll use a very light centinel object that will tell us when the 'Next' button must be enabled
	page->connectorValidation = new QRadioButton( page );
	page->connectorValidation->setHidden( true );
	connect( apiRadio, SIGNAL( toggled( bool) ), page, SLOT( onApiConnectorChosen( bool ) ) );
	connect( webRadio, SIGNAL( toggled( bool) ), page, SLOT( onWebConnectorChosen( bool ) ) );

	QHBoxLayout*const tableLayout = new QHBoxLayout();
	tableLayout->addLayout( featureLayout );
	tableLayout->addLayout( apiLayout );
	tableLayout->addLayout( webLayout );
	tableLayout->addWidget( page->connectorValidation );
	tableLayout->setContentsMargins( 0, 20, 0, 0 );

	QVBoxLayout*const layout = new QVBoxLayout( page );
	layout->addWidget( subTitle );
	layout->addLayout( tableLayout );

	QButtonGroup*const connectorCheckBox = new QButtonGroup();
	connectorCheckBox->addButton( apiRadio );
	connectorCheckBox->addButton( webRadio );
	
	//Fields ending in * are mandatory
	page->setMandatoryField( "connectorChosen*", page->connectorValidation );
	page->setLayout( layout );

	// If not first run, restore the user's chosen connector
	if( !Settings::self()->currentVersion().isEmpty() ) {
		if( Settings::self()->httpConnectorType() == HttpConnectorType::API_HTTPCONNECTOR ) {
			page->onApiConnectorChosen( true );
			apiRadio->setChecked( true );
		}
		else {
			webRadio->setChecked( true );
			page->onWebConnectorChosen( true );
		}
		page->connectorValidation->toggle();
	}

	return page;
}


QWizardPage* WelcomePage::createGetServiceKeyPage( WelcomeWizard* wizard ) {

	WelcomePage* page = new WelcomePage( wizard );
	page->setTitle( i18n( "%1Configuration wizard - Step 2/4%2" ).arg( "<h2>").arg( "</h2>" ) );
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

	QHBoxLayout* keyLayout = new QHBoxLayout();
	keyLayout->setContentsMargins( 0, 40, 0, 0 );
	keyLayout->addWidget( keyLabel );
	keyLayout->addWidget( keyText );

	QVBoxLayout* layout = new QVBoxLayout( page );
	layout->addWidget( subTitle );
	layout->addWidget( label );
	layout->addLayout( keyLayout );
	page->setLayout( layout );

	// Set mandatory fields. Only the one ending in * are mandatory
	page->setMandatoryField( "keyText*", keyText );

	return page;
}

QWizardPage* WelcomePage::createCheckServiceKeyPage( WelcomeWizard* wizard ) {
	WelcomePage*const page = new WelcomePage( wizard );
	page->setTitle( i18n( "%1Configuration wizard - Step 3/4%2" ).arg( "<h2>").arg( "</h2>" ) );
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
	page->setTitle( i18n( "%1Configuration wizard - Step 4/4%2" ).arg( "<h2>").arg( "</h2>" ) );
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
	freeConnector(); // Should do nothing, but keep it just in case
	ApiHttpConnector::loadSettings();
 	connector = new ApiHttpConnector( wizard->networkAccessManager(), key );
	
	// Connect the connector needed signals
	kDebug() << "WelcomePageId=" << wizard->currentId();
	connect( connector, SIGNAL( reportReady( Report*const ) ),
		     this, SLOT( testReportReceived( Report*const ) ) );
	connect( connector, SIGNAL( invalidServiceKeyError() ),
			 this, SLOT( testReportInvalidKey() ) );
	connect( connector, SIGNAL( errorOccurred( QString ) ),
			 this, SLOT( testReportFailed( QString ) ) );
	
	// Try to connect the to API
	setValidationStatus( i18n( "Validating your key..." ) );
	connector->retrieveFileReport( TEST_SERVICE_KEY_SCAN_ID );
}

void WelcomePage::testReportReceived( Report*const report ) {
//kDebug() << "testReportReceived() FileReport* report=" << report;
		if( report != NULL && report->getReportDate().isValid() ) {
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

void WelcomePage::testReportInvalidKey() {
	testReportFailed( i18n( "Your key is invalid!" ) );
}

void WelcomePage::testReportFailed( const QString& message ) {
	setValidationStatus( i18n( "ERROR: \"%1\". Please, go back and try again.", message ) );
}

void WelcomePage::onApiConnectorChosen( bool chosen ) {
kDebug() << "api chosen" << chosen;
	onWebConnectorChosen( !chosen );
}


void WelcomePage::onWebConnectorChosen( bool chosen ) {
kDebug() << "web chosen" << chosen;
	connectorValidation->setChecked( true );
	connectorChosen = true;
	connectorType = chosen ? HttpConnectorType::WEB_HTTPCONNECTOR : HttpConnectorType::API_HTTPCONNECTOR;
kDebug() << "connectorType=" << connectorType;
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
	const int pageId = wizard->currentId();
	if( pageId == WelcomePageConst::CONNECTOR_SELECTION_PAGE ) {
		if( connectorValidation != NULL && connectorChosen ) {
			connectorValidation->click();
			onWebConnectorChosen( connectorValidation->isChecked() );
		}
	}
	else if( pageId == WelcomePageConst::GET_SERVICE_KEY_PAGE ) {
		// Set the current user's service key, if available
		const QString serviceKey = Settings::self()->serviceKey();
		if( !serviceKey.isEmpty() && this->serviceKeyLineEdit != NULL ) {
			this->serviceKeyLineEdit->setText( serviceKey );
		}
	}
	else if( pageId == WelcomePageConst::CHECK_SERVICE_KEY_PAGE ) {
		startServiceKeyValidation();
	}
	else if( pageId == WelcomePageConst::CONCLUSION_PAGE ) {
		connectorType = ( (WelcomePage*) wizard->page( WelcomePageConst::CONNECTOR_SELECTION_PAGE ) )->connectorType;
	}
}

bool WelcomePage::validatePage() {
	// If we are in the last page
	kDebug() << "pageId" << wizard->currentId();
	const int pageId = wizard->currentId();
	if( pageId == WelcomePageConst::CONCLUSION_PAGE ) {
		kDebug() << "Storing user data...";
		const QString key = serviceKey();
		if( !key.isEmpty() ) {
			kDebug() << "Storing user service key" << key;
			Settings::self()->setServiceKey( key );
		}
			kDebug() << "Storing user connector type" << connectorType;
		Settings::self()->setHttpConnectorType( connectorType );
		Settings::self()->writeConfig();
	}
	return true;
}

int WelcomePage::nextId() const {
	const int pageId = wizard->currentId();
	if( pageId == WelcomePageConst::CONNECTOR_SELECTION_PAGE && connectorType == HttpConnectorType::WEB_HTTPCONNECTOR ) {
		return WelcomePageConst::CONCLUSION_PAGE;
	}
    return QWizardPage::nextId();
}

#include "welcomepage.moc"