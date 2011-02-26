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

#include "settingsdialog.h"
#include <QVBoxLayout>
#include <KLocalizedString>
#include <QCheckBox>
#include <settings.h>

SettingsDialog::SettingsDialog( QWidget * parent, Qt::WindowFlags flags ) : QWidget( parent, flags ) {
	QCheckBox* secureProtocol = new QCheckBox( i18n( "Use secure connections (HTTPS)" ) );
	secureProtocol->setObjectName( "kcfg_SecureProtocol" );
	
	QCheckBox* reuseLastReport = new QCheckBox( i18n( "Show first existing reports, if available" ) );
	reuseLastReport->setObjectName( "kcfg_ReuseLastReport" );
	
	QVBoxLayout* layout = new QVBoxLayout( this );
	layout->addWidget( secureProtocol );
	layout->addWidget( reuseLastReport );
	this->setLayout( layout );
}

SettingsDialog::~SettingsDialog() {

}

