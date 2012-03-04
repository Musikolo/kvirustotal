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

#include "apiurlreport.h"
#include <KLocalizedString>

static const QString URL_CLEAN_SITE( "clean site" );
static const QString URL_UNRATED_SITE( "unrated site" ); 
static const QString URL_ERROR( "error" ); 

ApiUrlReport::ApiUrlReport( const ServiceReply*const reply ) : BaseUrlReport( ) {
	processReply( reply );
}

ApiUrlReport::~ApiUrlReport() {

}

void ApiUrlReport::processReply( const ServiceReply*const reply ) {
	if( reply != NULL && reply->isValid() ) {
		BaseUrlReport::setScanDate( reply->getScanDate() );
		BaseUrlReport::setFileReportId( reply->getFileReportId() );
		const QMap< QString, QVariant > report = reply->getScanResult();
			// Set the result matrix with structure like {browswer, result}
			QList< ResultItem > resultList;
			int positives = 0;
			for( QMap< QString, QVariant >::const_iterator curItem = report.constBegin(); curItem != report.constEnd(); ++curItem ) {
				ResultItem rowItem;
				const QString& result = curItem.value().toString();
				rowItem.result = result;
				if( result.toLower() == URL_CLEAN_SITE ) {
					rowItem.infected = InfectionType::NO;
				}
				else if( result.toLower() == URL_UNRATED_SITE || result.toLower() == URL_ERROR ){
					rowItem.infected = InfectionType::UNKNOWN;
				}
				else {
					rowItem.infected = InfectionType::YES;
				}
				rowItem.antivirus = curItem.key();
				rowItem.lastUpdate = reply->getScanDate(); // This is the only date provided by the API, so far
				rowItem.version = i18n( "N/A" );
				
				 // If the row is infected, add one to the positives' count
				if( rowItem.infected == InfectionType::YES ) {
					positives++;
				}
				resultList.append( rowItem );
			}
		setResultList( resultList );
		setNumPositives( positives );
	}
}

void ApiUrlReport::setPermanentLink( const QUrl& permanetLink ) {
	BaseUrlReport::setPermanentLink( permanetLink );
}