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

#include "webfilereport.h"
#include "webservicereply.h"

static const QString CLEAN_RESULT( "-" );
static const QString SERVICE_DATE_FORMAT( "yyyy.MM.dd" );

WebFileReport::WebFileReport( const ServiceReply*const reply, const QString& fileName, FileHasher*const hasher ) {
	setFileName( fileName );
	this->hasher = ( hasher != NULL ? new FileHasher( hasher ) : new FileHasher( fileName ) );
	this->processReply( reply );
}

WebFileReport::~WebFileReport() {
	if( hasher != NULL ) {
		delete hasher;
		hasher = NULL;
	}
}

void WebFileReport::processReply( const ServiceReply*const reply ) {
	if( reply != NULL && reply->isValid() ) {
		const WebServiceReply*const webReply = dynamic_cast< const WebServiceReply*const >( reply );
		if( webReply != NULL ) {
			BaseReport::setPermanentLink( QUrl() ); // Since no scanId available, some object should invoke the setPermanentLink() method below!
			setScanDate( QDateTime::currentDateTime() );
			setNumPositives( webReply->getNumPositives() );
			QList< ResultItem > resultList;
			const QList< QVariant > matrix = webReply->getScanResult()[ "resultList" ].toList();
			for( int i = 0, size = matrix.size(); i < size; i++ ) {
				QList< QVariant > item = matrix[ i ].toList();
				ResultItem resultItem;
				resultItem.antivirus  = item[ 1 ].toString();
				resultItem.version    = item[ 2 ].toString();
				resultItem.lastUpdate = QDateTime::fromString( item[ 3 ].toString(), SERVICE_DATE_FORMAT );
				resultItem.result     = item[ 4 ].toString();
				resultItem.infected   = ( CLEAN_RESULT == resultItem.result ? InfectionType::NO : InfectionType::YES );
				resultList.append( resultItem );
			}
			setResultList( resultList );
			
		}
		else {
			kError() << "ERROR: Unexpected implementation of ServiceReply object!!";
		}
	}
	else {
		kError() << "The reply is either null or invalid!!";
	}
}

void WebFileReport::setPermanentLink( const QUrl& permanentLink ) {
	BaseReport::setPermanentLink( permanentLink );
}

QString WebFileReport::getMd5Sum() const {
  return hasher->getMd5Sum();
}

QString WebFileReport::getSha1Sum() const {
  return hasher->getSha1Sum();
}


QString WebFileReport::getSha256Sum() const {
  return hasher->getSha256Sum();
}