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

#include <KLocalizedString>

#include "apifilereport.h"

ApiFileReport::ApiFileReport( const ServiceReply*const reply, const QString& fileName, FileHasher*const hasher ) {

	this->hasher = ( hasher != NULL ? new FileHasher( hasher ) : new FileHasher( fileName ) );
	setFileName( fileName );
	processReply( reply );
}

ApiFileReport::~ApiFileReport() {
	if( hasher != NULL ) {
		delete hasher;
		hasher = NULL;
	}
}

void ApiFileReport::processReply( const ServiceReply*const reply ) {
	
	if( reply != NULL && reply->isValid() ){
		setScanDate( reply->getScanDate() );
		setPermanentLink( reply->getPermanentLink() );
		const QMap< QString, QVariant > report = reply->getScanResult();
		
		if( !report.isEmpty() ) {
			// Set the result matrix with structure like {antivirus, result}
			QList< ResultItem > resultList;
			int positives = 0;
			for( QMap< QString, QVariant >::const_iterator curItem = report.constBegin(); curItem != report.constEnd(); ++curItem ) {
				ResultItem rowItem;
				const QString& result = curItem.value().toString();
				rowItem.result = result;
				rowItem.infected = ( result.isEmpty() ? InfectionType::NO : InfectionType::YES );
				rowItem.antivirus = curItem.key();
				rowItem.lastUpdate = reply->getScanDate(); // This is the only date provided by the API
				rowItem.version = i18n( "N/A" );
				
				 // If the row is infected, add one to the positives' count
				if( rowItem.infected == InfectionType::YES ) {
					positives++;
				}
				resultList.append( rowItem );
			}
			setNumPositives( positives );
			setResultList( resultList );
		}
	}
}

QString ApiFileReport::getMd5Sum() const {
 	return hasher->getMd5Sum();
}

QString ApiFileReport::getSha1Sum() const {
	return hasher->getSha1Sum();
}

QString ApiFileReport::getSha256Sum() const {
	return hasher->getSha256Sum();
}