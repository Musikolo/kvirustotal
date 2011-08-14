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
#include "baseservicereply.h"

QMap< QString, QVariant > BaseServiceReply::getJsonMap( const QString& jsonText ) {
	// Process the json text
	QJson::Parser parser;
	QMap< QString, QVariant > json = parser.parse( jsonText.toAscii(), &valid ).toMap();
	if( !valid ) {
		kError() << "The given JSON text is invalid:\n" << jsonText;
		json = QMap< QString, QVariant >();
	}
	
	return json;		
}

QList< QVariant > BaseServiceReply::getJsonList( const QString& jsonText ) {
	// Process the json text
	QJson::Parser parser;
	QList< QVariant > json = parser.parse( jsonText.toAscii(), &valid ).toList();
	if( !valid ) {
		kError() << "The given JSON text is invalid:\n" << jsonText;
		json = QList< QVariant >();
	}
	
	return json;		
}
