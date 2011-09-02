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

#include <KAboutData>
#include <KCmdLineArgs>
#include <KApplication>
#include "constants.h"
#include "mainwindow.h"

int main( int argc, char** argv ) {

	KAboutData about( General::APP_NAME, 0, ki18n( General::APP_UI_NAME ), General::APP_VERSION.toAscii(), 
					   ki18n( "Online antivirus and anti-phishing tool" ), KAboutData::License_GPL_V3 );
	about.setProgramIconName( "kvirustotal" );
	about.setCopyrightStatement( ki18n( "Copyright (c) 2011 - Carlos López Sánchez" ) );
	about.setHomepage( General::APP_HOMEPAGE );
	const QString esEmail( QString( General::APP_AUTHOR ).append( "<musikolo" ).append( "@" ).append( "hotmail" ).append( ".com>" ) );
	about.addAuthor( ki18n( General::APP_AUTHOR ), ki18n( "Main developer" ), esEmail.toLatin1(), General::APP_HOMEPAGE );
	const QString csEmail( QString( "Pavel Fric" ).append( "<pavelfric" ).append( "@" ).append( "seznam" ).append( ".cz>" ) );
	about.addCredit( ki18n( "Pavel Fric" ), ki18n( "Translator" ), csEmail.toLatin1(), "fripohled.blogspot.com" );
	const QString deEmail( QString( "Sascha Manns" ).append( "<saigkill" ).append( "@" ).append( "opensuse" ).append( ".org>" ) );
	about.addCredit( ki18n( "Sascha Manns" ), ki18n( "Translator" ), deEmail.toLatin1(), "http://saigkill.homelinux.net" );
	const QString itEmail( QString( "Gianluca Boiano" ).append( "<morf3089" ).append( "@" ).append( "gmail" ).append( ".com>" ) );
	about.addCredit( ki18n( "Gianluca Boiano" ), ki18n( "Translator" ), itEmail.toLatin1() );
	about.addCredit( ki18n( General::APP_AUTHOR ), ki18n( "Translator" ), esEmail.toLatin1(), General::APP_HOMEPAGE );
	about.setOtherText( ki18n( "KVirusTotal is based on the service provided by VirusTotal which is owned by \
Hispasec Sistemas S.L. Although KVirusTotal is a powerful tool, remember that <b>it is not \
intended as a replacement of a full-fledged antivirus program.</b><br><br>\
VirusTotal and the logo are trademarks of Hispasec Sistemas S.L. Please, \
visit <a href='http://www.virustotal.com/terms.html'>\
http://www.virustotal.com/terms.html</a> for more details." ) );

	KCmdLineArgs::init( argc, argv, &about );
    KApplication app;
	KGlobal::locale()->setActiveCatalog( "kvirustotal" );

	// We need to allocate the MainWindow object in the heap ( new ), rather than in the stack.
	// It will be freed once the application finishes.
	MainWindow* mainWindow = new MainWindow();
	mainWindow->show();

    return app.exec();
}