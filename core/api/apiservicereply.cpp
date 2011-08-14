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

#include "apiservicereply.h"

ApiServiceReply::ApiServiceReply( QString jsonText ) {
		
		// Process the json text
		const QMap< QString, QVariant > json = getJsonMap( jsonText );
		this->processJson( json );
}

void ApiServiceReply::processJson( const QMap< QString, QVariant >& json ) {
	if( !json.isEmpty() ) {
		// Set the status of the reply
		QVariant aux = json[ "result" ];
		if( !aux.isNull() && aux.isValid() ) {
			setStatus( aux.toInt() );
		}
		else {
			setValid( false );
			kError() << "ERROR: No result tag available!";
		}
		
		// Set the scan ID
		aux = json[ "scan_id" ];
		if( !aux.isNull() && aux.isValid() ) {
			setScanId( aux.toString() );
		}
		
		// Set the permanent link
		aux = json[ "permalink" ];
		if( !aux.isNull() && aux.isValid() ) {
			setPermanentLink( aux.toUrl() );
		}
		
		// Set the scan date and the result matrix
		aux = json[ "report" ];
		if( !aux.isNull() && aux.isValid() ) {
			const QList< QVariant > report = aux.toList();
			if( report.size() > 1 ) {
				setScanDate( report[ 0 ].toDateTime() );
				setMatrix( report[ 1 ].toMap() );
			}
		}
		
		// Set the file-report identifier (for URL reports only)
		aux = json[ "file-report" ];
		if( !aux.isNull() && aux.isValid() ){
			setFileReportId( aux.toString() );
		}
	} 
	else {
		setValid( false );
		kError() << "No JSON content to process!";
	}
}

ApiServiceReplyResult::ApiServiceReplyResultEnum ApiServiceReply::getStatus() {
	switch( getInnerStatus() ){
		case ApiServiceReplyResult::INVALID_SERVICE_KEY:
			kDebug() << "INVALID_SERVICE_KEY";
			return ApiServiceReplyResult::INVALID_SERVICE_KEY;
		
		case ApiServiceReplyResult::ITEM_PRESENT:
			return ApiServiceReplyResult::ITEM_PRESENT;
		
		case ApiServiceReplyResult::REQUEST_LIMIT_REACHED:
			return ApiServiceReplyResult::REQUEST_LIMIT_REACHED;
		
		default:
			kError() << "Invalid service result value" << getInnerStatus() << ". The next value will assumed:" <<  ApiServiceReplyResult::ITEM_NOT_PRESENT;
		case ApiServiceReplyResult::ITEM_NOT_PRESENT:
			return ApiServiceReplyResult::ITEM_NOT_PRESENT;
	}
}