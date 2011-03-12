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

#include <KLocalizedString>
#include <QLabel>
#include <KLineEdit>
#include <QVBoxLayout>
#include <KIcon>
#include <QMovie>
#include <KDebug>

#include "constants.h"
#include "welcomepage.h"
#include "welcomewizard.h"

WelcomeWizard::WelcomeWizard( QNetworkAccessManager*const networkManager, QWidget* parent ) : QWizard( parent ) {
	this->networkManager = networkManager;
	setupWizard();
}

WelcomeWizard::~WelcomeWizard() { 
}

void WelcomeWizard::setupWizard() {
	// Create setup the wizard and its pages
	setTitleFormat( Qt::RichText );
	setSubTitleFormat( Qt::RichText );
	setWindowIcon( KIcon( "kvirustotal") );
	setPage( WelcomePageConst::INTRO_PAGE, WelcomePage::createIntroPage( this ) );
	setPage( WelcomePageConst::GET_SERVICE_KEY_PAGE, WelcomePage::createGetServiceKeyPage( this ) );
	setPage( WelcomePageConst::CHECK_SERVICE_KEY_PAGE, WelcomePage::createCheckServiceKeyPage( this ) );
	setPage( WelcomePageConst::CONCLUSION_PAGE, WelcomePage::createConclusionPage( this ) );

	// Set the common properties for the wizard
	setWindowTitle( i18n( "Welcome wizard - %1", General::APP_UI_NAME ) );
}

void WelcomeWizard::show() {
//FIXME: For some misterious reason, without wizard varible the show() method does not work!!
	QWizard* wizard = this;
	wizard->setModal( true );
	wizard->show();
}

QNetworkAccessManager* WelcomeWizard::networkAccessManager() const {
	return this->networkManager;
}