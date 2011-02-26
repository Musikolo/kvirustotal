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


#include "urlreport.h"

UrlReport::UrlReport( const QString & json ) {
	commonSetup( json ); // It will call local extendedSetup() method
}

void UrlReport::extendedSetup( const QVariantMap & jsonMap ) {
	// Set the permanent link and result
	this->fileReportId = jsonMap[ JsonTag::FILE_REPORT ].toString();
}

const QString& UrlReport::getFileReportId() {
	return this->fileReportId;
}
