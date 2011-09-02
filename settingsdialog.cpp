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

#include <QVBoxLayout>
#include <KLocalizedString>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>

#include <settings.h>
#include "settingsdialog.h"

SettingsDialog::SettingsDialog( QWidget * parent, Qt::WindowFlags flags ) : QWidget( parent, flags ) {
	QCheckBox*const secureProtocol = new QCheckBox( i18n( "Use secure connections (HTTPS)" ) );
	secureProtocol->setObjectName( "kcfg_SecureProtocol" );
	
	QCheckBox*const reuseLastReport = new QCheckBox( i18n( "Show first existing reports, if available" ) );
	reuseLastReport->setObjectName( "kcfg_ReuseLastReport" );
	
	QSpinBox*const notificationTime = new QSpinBox();
	notificationTime->setSingleStep( 10 ); // 10-unit steps
	notificationTime->setObjectName( "kcfg_TaskNotificationTime" );
	QLabel*const notificationTimeLabel = new QLabel( i18n( "Time tasks must take to show a notification (0=disabled)" ) );
	notificationTimeLabel->setBuddy( notificationTime );
	QHBoxLayout*const notificationLayout = new QHBoxLayout();
	notificationLayout->addWidget( notificationTimeLabel );
	notificationLayout->addWidget( notificationTime );
	
	QCheckBox*const infectedTaskNotification = new QCheckBox( i18n( "Show always notifications for infected tasks" ) );
	infectedTaskNotification->setObjectName( "kcfg_InfectedTaskNotification" );

	QCheckBox*const checkNewVersion = new QCheckBox( i18n( "Check for new versions at startup" ) );
	checkNewVersion->setObjectName( "kcfg_CheckVersionAtStartup" );
	
	QVBoxLayout*const layout = new QVBoxLayout( this );
	layout->addWidget( secureProtocol );
	layout->addWidget( reuseLastReport );
	layout->addLayout( notificationLayout );
	layout->addWidget( infectedTaskNotification );
	layout->addWidget( checkNewVersion );
	this->setLayout( layout );
}

SettingsDialog::~SettingsDialog() {

}