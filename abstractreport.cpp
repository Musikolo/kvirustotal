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

#include <qjson/parser.h>
#include "abstractreport.h"

AbstractReport::AbstractReport() { }
AbstractReport::~AbstractReport() { }

static const QString SERVICE_DATE_TIME_FORMAT( "yyyy-MM-dd hh:mm:ss" );
static const QString URL_CLEAN_SITE( "Clean site" );

const QDateTime & AbstractReport::getReportDate() const {
	return this->reportDate;
}

void AbstractReport::setReportDate( const QString& date ) {
	this->setReportDate( QDateTime::fromString( date, SERVICE_DATE_TIME_FORMAT ) );
}

void AbstractReport::setReportDate( const QDateTime& date ) {
	this->reportDate = date;
}

const QMap< QString, RowResult > & AbstractReport::getResultMatrix(){
	return this->resultMatrix;
}

void AbstractReport::commonSetup( const QString& json ) {
	if( !json.isEmpty() ) {
		bool ok;
		QJson::Parser parser;
		const QVariant jsonData = parser.parse( json.toAscii(), &ok );
		if( ok ){
			const QVariantMap jsonMap = jsonData.toMap();
			commonSetup( jsonMap );
		}
	}
}

void AbstractReport::commonSetup( const QVariantMap& jsonMap ) {

	const QVariant report = jsonMap[ JsonTag::REPORT ];
	if( report.isValid() ){
		const QList< QVariant > list = report.toList();
		if( list.size() == 2  ) {
			// Set the report date
			setReportDate( list.at( 0 ).toString() );

			// Set the result matrix with structure like {antivirus, result}
			positives = 0;
			const QMap< QString, QVariant > map = list.at( 1 ).toMap();
			for( QMap< QString, QVariant >::const_iterator curItem = map.constBegin(); curItem != map.constEnd(); ++curItem ) {
				const QString result = curItem.value().toString();
				const RowResult rowResult = { result, !result.isEmpty() && result != URL_CLEAN_SITE };
				
				 // If the row is infected, add one to the positives' count
				if( rowResult.infected ) {
					positives++;
				}
				resultMatrix.insert( curItem.key(), rowResult );
			}
		}
	}

	// Set the result and call the concrete-instace extendedSetup() method
	setServiceReplyResult( jsonMap[ JsonTag::REPORT ].toInt() );
	extendedSetup( jsonMap );
}