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

#include "weburlreport.h"
#include "webservicereply.h"

#include <KLocalizedString>

static const QString URL_CLEAN_SITE( "Clean site" );
static const QString URL_UNRATED_SITE( "Unrated site" ); 
static const QString URL_ERROR( "Error" ); 

WebUrlReport::WebUrlReport( const ServiceReply*const reply ) : BaseUrlReport( ) {
	processReply( reply );
}

WebUrlReport::~WebUrlReport() {

}

void WebUrlReport::processReply( const ServiceReply*const reply ) {

	if( reply != NULL && reply->isValid() ) {
		const WebServiceReply*const webReply = dynamic_cast< const WebServiceReply*const >( reply );
		if( webReply != NULL ) {
			setPermanentLink( QUrl() ); // Since no full URL available, some object should invoke the setPermanentLink() method below!
			setScanDate( reply->getScanDate() );
			
			int positives = 0;
			QList< ResultItem > resultList;
			const QList< QVariant > matrix = webReply->getScanResult()[ "resultList" ].toList();
			for( int i = 0, size = matrix.size(); i < size; i++ ) {
				QMap< QString, QVariant > item = matrix[ i ].toMap();
				ResultItem rowItem;
				const QString& result = item[ "result" ].toString();
				rowItem.result = result;
				if( result == URL_CLEAN_SITE ) {
					rowItem.infected = InfectionType::NO;
				}
				else if( result == URL_UNRATED_SITE || result == URL_ERROR ){
					rowItem.infected = InfectionType::UNKNOWN;
				}
				else {
					rowItem.infected = InfectionType::YES;
					positives++;
				}
				rowItem.antivirus = item[ "antivirus" ].toString();
				rowItem.lastUpdate = reply->getScanDate();
				rowItem.version = i18n( "N/A" );
				resultList.append( rowItem );
			}
			setResultList( resultList );
			setNumPositives( positives );
		}
		else {
			kError() << "ERROR: Unexpected implementation of ServiceReply object!!";
		}
	}
}

void WebUrlReport::setPermanentLink( const QUrl& permanetLink ) {
	BaseReport::setPermanentLink( permanetLink );
}