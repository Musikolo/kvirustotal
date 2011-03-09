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


#ifndef WELCOMEPAGE_H
#define WELCOMEPAGE_H

#include <QWizardPage>
#include <QAbstractButton>
#include <QLabel>
#include <KLineEdit>

#include "httpconnector.h"
#include "filereport.h"
#include "welcomewizard.h"

namespace WelcomePageConst {
	enum WelComePageEnum { INTRO_PAGE, GET_SERVICE_KEY_PAGE, CHECK_SERVICE_KEY_PAGE, CONCLUSION_PAGE };
}
class WelcomePage : public QWizardPage {
Q_OBJECT
private:
	 WelcomeWizard* wizard;
	 HttpConnector* connector;
	 KLineEdit* serviceKeyLineEdit;
	 QLabel* validationStatus;
	 QAbstractButton* validationKeyOk;

     void setMandatoryField( const QString& name, QWidget* widget, const char* property = 0, const char* changedSignal = 0 );
	 void freeConnector();

private slots:
	void openLink( const QString& link ) const;
	void setValidationStatus( const QString& status );
	void testReportReceived( AbstractReport*const report );
	void testReportInvalidKey();
	void testReportFailed( const QString& message );
	
public:
    WelcomePage( WelcomeWizard* wizard );
    virtual ~WelcomePage();
	static QWizardPage* createIntroPage( WelcomeWizard* wizard );
	static QWizardPage* createGetServiceKeyPage( WelcomeWizard* wizard );
	static QWizardPage* createCheckServiceKeyPage(WelcomeWizard* wizard );
	static QWizardPage* createConclusionPage( WelcomeWizard* wizard );
	void startServiceKeyValidation();
	QString serviceKey() const;
	virtual void initializePage(); // Overrride
	virtual bool validatePage(); // Override
	virtual void cleanupPage(); // Overrride
};
#endif // WELCOMEPAGE_H
