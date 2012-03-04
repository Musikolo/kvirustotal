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
#include <jsonutil.h>
#include <QStringList>

static QRegExp scanIdRegex( "([abcdef\\d]+)-(\\d+)" ); //Expr: <hexadecimal number>-<decimal number>
static const QString SERVICE_DATE_FORMAT( "yyyy-MM-dd hh:mm:ss" );

WebServiceReply::WebServiceReply( const QString& textReply, WebServiceReplyType::WebServiceReplyTypeEnum replyType ) {
	// Process the text reply
	this->replyType = replyType;

	bool valid;
	const QMap< QString, QVariant > json = JsonUtil::getJsonMap( textReply, valid );
	if( !valid ) {
		setValid( false );
		return;
	}
	this->processJsonMap( json );
}

WebServiceReply::~WebServiceReply() { }

void WebServiceReply::processJsonMap( const QMap< QString, QVariant >& json ) {
	// If valid JSON reply, process the content
	if( !json.isEmpty() ) {
		// Status
		QVariant aux = json[ "status" ];
		if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
			setStatus( getStatusFromString( aux.toString() ) );
		}
		else {
			setValid( false );
			kError() << "ERROR: No status tag received!!";
			return;
		}

		// Result list
		switch( replyType ) {
			case WebServiceReplyType::FILE:
				aux = json[ "info" ];
				if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
					setScanDate( retrieveScanDate( aux.toString() ) );
				}
				
				aux = json[ "results" ];
				if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
					QMap< QString, QVariant > resultList;
					resultList[ "resultList" ] = processResultData( aux.toString() );
					setMatrix( resultList );
				}
				
				// File report
				aux = json[ "sha256" ];
				if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
					setFileReportId( aux.toString() );
				}	
				break;
			case WebServiceReplyType::URL_SERVICE:
				aux = json[ "info" ];
				if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
					setScanDate( retrieveScanDate( aux.toString() ) );
				}
				
				aux = json[ "results" ];
				if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
					QMap< QString, QVariant > resultList;
					resultList[ "resultList" ] = processResultData( aux.toString() );
					setMatrix( resultList );
				}
				// Permanet link
				aux = json[ "permaid" ];
				if( !aux.isNull() && aux.canConvert( QVariant::String ) ) {
					setPermanentLink( aux.toString() );
				}	
				break;
			default:
				kError() << "ERROR: Unexpected reply type:" << replyType;
				setValid( false );
				return;
		}

		// Number of positives
		setNumPositives( -1 ); // Not available in this reply
	}
	else {
		setValid( false );
		kError() << "ERROR: No JSON content received!";
	}
}

QDateTime WebServiceReply::retrieveScanDate( const QString& data ) {
	QRegExp rx( "(\\d{4}-\\d{2}-\\d{2}\\s\\d{2}:\\d{2}:\\d{2})" ); // Reg. Expr. to extract a date YYYY-MM-DD HH:MM:SS
	rx.indexIn( data );
	if( rx.captureCount() > 0 ) {
		const QString value = rx.cap( 1 );
		kDebug() << "Found scan date:" << value << "UTC";
		QDateTime dateTime = QDateTime::fromString( value, SERVICE_DATE_FORMAT );
		dateTime.setTimeSpec( Qt::UTC );
		return dateTime.toLocalTime();
	}
	kWarning() << "It was not possible to find out the scan date. Returning current date...";
	return QDateTime();
}

QList< QVariant > WebServiceReply::processResultData( const QString& data ) {
	QList< QVariant > matrix;
	QRegExp rx( "<td.*>(.*)</td>" ); // Reg. Expr. to extract text surrounded by <td> and </td>
	rx.setMinimal( true ); // Change to non-greddy mode
	int pos = data.indexOf( "<tbody>" );
	int item = 0;
	QMap< QString, QVariant >* row = NULL;
	while( pos >= 0 ) {
		pos = rx.indexIn( data, pos );
		if( pos >= 0){
			if( rx.captureCount() > 0 ) {
				switch( item ) {
				case 0:
					row = new QMap< QString, QVariant >();
					(*row)[ "antivirus" ] = rx.cap( 1 );
					item++;
					break;
				case 1:
					(*row)[ "result" ] = rx.cap( 1 );
					item++;
					if( replyType == WebServiceReplyType::URL_SERVICE ) {
						matrix.append( *row );
						if( row != NULL ) {
							delete row;
							row = NULL;
						}
						item = 0;
					}
					break;
				default:
					(*row)[ "update" ] = rx.cap( 1 );
					matrix.append( *row );
					if( row != NULL ) {
						delete row;
						row = NULL;
					}
					item = 0;
					break;
				break;
				};
			}
			pos++;
		}
	}

	if( row != NULL ) {
		kWarning() << "The row pointer should be freed at this point, but it isn't. Freeing row memory now...";
		delete row;
		row = NULL;
	}
	
	return matrix;
}

int WebServiceReply::getStatusFromString( const QString& status ) {
	if( !status.isEmpty() ) {
		const QString value = status.trimmed().toLower();
		if( value == "pending" ) {
			return WebServiceReplyResult::SCAN_QUEUED;
		}
		if( value == "analysing" ) {
			return WebServiceReplyResult::SCAN_RUNNING;
		}
		if( value == "completed" ) {
			return WebServiceReplyResult::SCAN_COMPLETED;
		}
	}
	return WebServiceReplyResult::SCAN_ERROR; // Value is "failed" or anything else
}

WebServiceReplyResult::WebServiceReplyResultEnum WebServiceReply::getStatus() {
	
	switch( getInnerStatus() ) {
	case 2: 
		return WebServiceReplyResult::SCAN_STARTED;
	case 3:
		return WebServiceReplyResult::SCAN_QUEUED;
	case 4:
		return WebServiceReplyResult::SCAN_RUNNING;
	case 5:
		return WebServiceReplyResult::SCAN_COMPLETED;
	default:
		kWarning() << "WARN: Unknown status:" << getInnerStatus() << ". SCAN_ERROR status will be assumed!";
	case 6:
		return WebServiceReplyResult::SCAN_ERROR;
	}
}