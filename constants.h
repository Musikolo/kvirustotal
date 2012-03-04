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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace General {
	static const char* APP_UI_NAME  	  = "KVirusTotal";
	static const char* APP_NAME 		  = "kvirustotal";
	static const uchar APP_VERSION_MAJOR  =  0;
	static const uchar APP_VERSION_MINOR  = 30;
	static const uchar APP_VERSION_BUGFIX =  0;
	static const QString APP_VERSION	  = QString( QString( "%1.%2.%3" ).arg( APP_VERSION_MAJOR ).arg( APP_VERSION_MINOR ).arg( APP_VERSION_BUGFIX ) );
	static const char* APP_HOMEPAGE 	  = "http://kde-apps.org/content/show.php?content=139065";
	static const char* APP_AUTHOR		  = "Carlos López Sánchez";
	
}
/** Intended just to remove unused warning */
class __WarningRemover {
private:
	__WarningRemover(){};
	void removeUnusedWarnings() {
		Q_UNUSED( General::APP_UI_NAME);
		Q_UNUSED( General::APP_NAME);
		Q_UNUSED( General::APP_VERSION_MINOR);
		Q_UNUSED( General::APP_VERSION_MINOR);
		Q_UNUSED( General::APP_VERSION_BUGFIX);
		Q_UNUSED( General::APP_VERSION);
		Q_UNUSED( General::APP_HOMEPAGE);
		Q_UNUSED( General::APP_AUTHOR);
	}
};

#endif // CONSTANTS_H