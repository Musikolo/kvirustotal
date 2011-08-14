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

#include "webservicereply.h"
#include <QRegExp>

static QRegExp scanIdRegex( "([abcdef\\d]+)-(\\d+)" ); //Expr: <hexadecimal number>-<decimal number>

WebServiceReply::WebServiceReply( const QString& textReply, WebServiceReplyType::WebServiceReplyTypeEnum replyType, bool htmlReply ) {
		// Process the text reply
		this->replyType = replyType;
		if( htmlReply ) {
			this->processHtml( textReply );
		}
		else if( replyType == WebServiceReplyType::URL_SERVICE_1 ) {
			const QList< QVariant > json = getJsonList( textReply );
			this->processJsonList( json );
		}
		else {
			const QMap< QString, QVariant > json = getJsonMap( textReply );
			this->processJsonMap( json );
		}
}

WebServiceReply::~WebServiceReply() {

}

void WebServiceReply::processHtml( const QString& htmlReply ) {
	if( !htmlReply.isNull() && !htmlReply.isEmpty() ) {
		if( scanIdRegex.indexIn( htmlReply ) > -1 ) {
			const QString scanId = scanIdRegex.cap();
			kDebug() << "Found scanId" << scanId << "in reply!";
			setScanId( scanId );
			return;
		}
	}
	setValid( false );
	kError() << "No scanId could be found in the reply!!";
}


void WebServiceReply::processJsonList( const QList< QVariant >& json ) {
	// If valid JSON reply, process the content
	if( !json.isEmpty() && json.size() > 1 ) {
		// Status
		QVariant aux = json[ 0 ];
		if( !aux.isNull() && aux.canConvert( QVariant::Int ) ) {
			setStatus( aux.toInt() );
		}
		else {
			setValid( false );
			kError() << "ERROR: No status value received!!";
			return;
		}

		// Get the scan Id from given Url like this "/url-scan/report.html?id=40e9fadf8d8b8fed3fb0189d9db024a9-1312391819"
		aux = json[ 1 ];
		if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
			if( scanIdRegex.indexIn( aux.toString() ) > -1 ) {
				const QString scanId = scanIdRegex.cap();
				kDebug() << "Found scanId" << scanId << "in reply!";
				setScanId( scanId );
			}
			else {
				setValid( false );
				kError() << "ERROR: No scanId value found!!";
				return;
			}
		}
		else {
			setValid( false );
			kError() << "ERROR: No scanId value received!!";
			return;
		}

		// Last report date
		if( json.size() > 2 ) {
			aux = json[ 2 ];
			if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
				QString utcDate = aux.toString().left( 20 ); // Get date from string like this "2011-08-03 19:16:59 (UTC) [0/16]"
				QDateTime date = QDateTime::fromString( utcDate );
				date.setTimeSpec( Qt::UTC );
				setScanDate( date );
			}
		}
		else {
			setScanDate( QDateTime::currentDateTime() );
		}
	}
	else {
		setValid( false );
		kError() << "ERROR: No JSON content received!";
	}
	
}
	
void WebServiceReply::processJsonMap( const QMap< QString, QVariant >& json ) {
	// If valid JSON reply, process the content
	if( !json.isEmpty() ) {
		// Status
		QVariant aux = json[ "status" ];
		if( !aux.isNull() && aux.canConvert( QVariant::Int ) ) {
			setStatus( aux.toInt() );
		}
		else {
			setValid( false );
			kError() << "ERROR: No status tag received!!";
		}

		// Result list
		switch( replyType ) {
			case WebServiceReplyType::FILE:
				aux = json[ "avres" ];
				break;
			case WebServiceReplyType::URL_SERVICE_2:
				aux = json[ "scan" ];
				break;
			default:
				kError() << "ERROR: Unexpected reply type:" << replyType;
				setValid( false );
				return;
		}
		if( !aux.isNull() && aux.canConvert( QVariant::List ) ) {
			QMap< QString, QVariant > resultList;
			resultList[ "resultList" ] = aux;
			setMatrix( resultList );
		}

		// File report
		aux = json[ "permalink" ];
		if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
			setFileReportId( aux.toString() );
		}	

		// Number of positives
		aux = json[ "detected" ];
		if( !aux.isNull() && aux.canConvert( QVariant::Int ) ) {
			setNumPositives( aux.toInt() );
		}
	}
	else {
		setValid( false );
		kError() << "ERROR: No JSON content received!";
	}
}

WebServiceReplyResult::WebServiceReplyResultEnum WebServiceReply::getStatus() {
	
	switch( this->replyType ) {
		case WebServiceReplyType::FILE: {
			switch( getInnerStatus() ) {
				case 1: 
					return WebServiceReplyResult::SCAN_STARTED;
				case 2:
					return WebServiceReplyResult::SCAN_QUEUED;
				case 3:
					return WebServiceReplyResult::SCAN_RUNNING;
				case 4:
					return WebServiceReplyResult::SCAN_COMPLETED;
				default:
					kWarning() << "WARN: Unknown status:" << getInnerStatus() << ". SCAN_ERROR status will be assumed!";
				case 5:
					return WebServiceReplyResult::SCAN_ERROR;
			}
		}
		case WebServiceReplyType::URL_SERVICE_1: {
			switch( getInnerStatus() ) {
				default:
					kWarning() << "WARN: Unknown status:" << getInnerStatus() << ". SCAN_ERROR status will be assumed!";
				case -1:
					return WebServiceReplyResult::SCAN_ERROR;
				case 0:
					return WebServiceReplyResult::SCAN_REPORT_REUSABLE;
				case 1: 
					return WebServiceReplyResult::SCAN_REPORT_NOT_REUSABLE;
			}
		}
		case WebServiceReplyType::URL_SERVICE_2: {
			switch( getInnerStatus() ) {
				case 0:
					return WebServiceReplyResult::SCAN_STARTED;
				case 1: 
					return WebServiceReplyResult::SCAN_RUNNING;
				case 2:
					return WebServiceReplyResult::SCAN_COMPLETED;
				default:
					kWarning() << "WARN: Unknown status:" << getInnerStatus() << ". SCAN_UNCOMPLETED status will be assumed!";
					return WebServiceReplyResult::SCAN_ERROR;
			}
		}
		default:
			kError() << "ERROR: Unexpected reply type:" << replyType;
			return WebServiceReplyResult::SCAN_ERROR;
	}
}