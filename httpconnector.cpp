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

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <qjson/parser.h>
#include <KDebug>

#include "filereport.h"
#include "httpconnector.h"
#include <klocalizedstring.h>
#include "settings.h"

/** Tags used to manage JSON objects. Extends ServiceBasicReply namespace. */
namespace JsonTag {
	static const QString KEY	  = "key";
	static const QString RESOURCE = "resource";
	static const QString URL 	  = "url";
	static const QString FILE	  = "file";
	static const QString SCAN 	  = "scan";
	static const QString SCAN_ID  = "scan_id";
}

/** VirusTotal service URLs */
namespace ServiceUrl {
	static QString GET_FILE_REPORT;
	static QString GET_URL_REPORT;
	static QString SEND_FILE_SCAN;
	static QString SEND_URL_SCAN;
	static QString MAKE_COMMENT;
}

// Static method
void HttpConnector::loadSettings() {
	static const QString GET_FILE_REPORT_PATTERN = "%1://www.virustotal.com/api/get_file_report.json";
	static const QString GET_URL_REPORT_PATTERN  = "%1://www.virustotal.com/api/get_url_report.json";
	static const QString SEND_FILE_SCAN_PATTERN  = "%1://www.virustotal.com/api/scan_file.json";
	static const QString SEND_URL_SCAN_PATTERN   = "%1://www.virustotal.com/api/scan_url.json";
	static const QString MAKE_COMMENT_PATTERN    = "%1://www.virustotal.com/api/make_comment.json";
	
	const QString protocol( Settings::self()->secureProtocol() ? "https" : "http" );
	kDebug() << "Establishing the " << protocol << " protocol for any forthcomming connection...";
	ServiceUrl::GET_FILE_REPORT = GET_FILE_REPORT_PATTERN.arg( protocol );
	ServiceUrl::GET_URL_REPORT  = GET_URL_REPORT_PATTERN.arg( protocol );
	ServiceUrl::SEND_FILE_SCAN  = SEND_FILE_SCAN_PATTERN.arg( protocol );
	ServiceUrl::SEND_URL_SCAN   = SEND_URL_SCAN_PATTERN.arg( protocol );
	ServiceUrl::MAKE_COMMENT    = MAKE_COMMENT_PATTERN.arg( protocol );
}

HttpConnector::HttpConnector( QNetworkAccessManager * manager, const QString & key ) {
	this->manager = manager;
	this->key = key;
	this->reply = NULL;
	this->file = NULL;
	this->timer = NULL;
	this->scanId = QString();
	this->reportMode = ReportMode::NONE;
	this->abortRequested = false;
	this->hasher = NULL;
	this->reusingLastReport = false;
	this->requestDelay = ServiceRequestDelay::DELAY_LEVEL_3;
}

HttpConnector::~HttpConnector() {
	// Since we receive ownership of the reply object, we need to handle deletion.
	if( reply ) {
		reply->deleteLater();
		reply = NULL;
	}
	if( hasher ) {
		delete hasher;
		hasher = NULL;
	}
}

void HttpConnector::createNetworkReply( const QNetworkRequest& request, const QByteArray& multipart ) {
	// Since we receive ownership of the reply object, we need to handle deletion.
	if( reply ) {
		reply->deleteLater();
		reply = NULL;
	}

	this->reply = manager->post( request, multipart );
}

void HttpConnector::submitFile( const QString& fileName ){
	kDebug() << "Submitting file " << fileName << "...";
	if( fileName.isEmpty() ) {
		kDebug() << "Nothing will be done as the file name is empty!";
		return;
	}
	this->fileName = fileName;
	emit( startScanning() );

	const bool reuseLastReport = Settings::self()->reuseLastReport();
	if( reuseLastReport ) {
		retrieveLastReport( fileName );
	}
	else {
		uploadFile( fileName );
	}
}

void HttpConnector::retrieveLastReport( const QString& fileName ) {

	// Se the report mode. If other than none is active, abort.
	if( this->reportMode != ReportMode::NONE ) {
		QString msg( i18n( "An invalid report mode is active: %1. Aborting...", reportMode ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}
	reportMode = ReportMode::FILE_MODE;
	reusingLastReport = true;

	if( hasher ) {
		delete hasher;
	}
	kDebug() << "Generating hashes...";
	hasher = new FileHasher( fileName );
	kDebug() << "Reusing existing report, if possible...";
	emit( retrieveFileReport( hasher->getSha256Sum() ) );
}

void HttpConnector::uploadFile( const QString& fileName ) {

	// Se the report mode. If other than none is active, abort.
	if( this->reportMode != ReportMode::NONE ) {
		QString msg( i18n( "An invalid report mode is active: %1. Aborting...", reportMode ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}
	reportMode = ReportMode::FILE_MODE;
	reusingLastReport = false;

	// Set up the request
	QUrl url( ServiceUrl::SEND_FILE_SCAN );
	url.addQueryItem( JsonTag::KEY, key );
	QNetworkRequest request( url );

	// Prepare the multipart form
	QByteArray multipartform;
	if( !setupMultipartRequest( request, multipartform, fileName ) ) {
		QString msg( i18n( "An error has ocurred while submitting the file %1...", fileName ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}

	// Submit data and establish all connections to this object from scratch
	createNetworkReply( request, multipartform );
	connect( reply, SIGNAL( finished() ), this, SLOT( submissionReply() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
		     this, SLOT( submissionReplyError( QNetworkReply::NetworkError ) ) );
	connect( reply, SIGNAL( uploadProgress( qint64, qint64 ) ),
			 this, SLOT( uploadProgressRate( qint64, qint64 ) ) );
	connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ),
			 this, SLOT( downloadProgressRate( qint64, qint64 ) ) );

	// Retransmit the uploadProgress signal
	connect( reply, SIGNAL( uploadProgress( qint64, qint64 ) ),
			 this, SIGNAL( uploadingProgressRate( qint64, qint64 ) ) );

	kDebug() << "File sent, waiting for reply...";
}


bool HttpConnector::setupMultipartRequest( QNetworkRequest& request, QByteArray& multipartform, const QString& fileName ) {
//TODO: See whether it's possible to replace QFile for QIODevice
	// Open the file and verify it's accessible. Otherwise, return false.
	QFile file( fileName );
	file.open( QIODevice::ReadOnly );
	if( !file.isOpen() ) {
		kDebug() << "Could not open file " << fileName;
		return false;
	}

	// Special thanks to Mathias G. - http://forums.dropbox.com/topic.php?id=26946#post-167940
	// Prepare the boundary
	QString crlf("\r\n");
	QString boundaryStr("---------------------------109074266748897678777839994");
	QString boundary="--"+boundaryStr+crlf;

	// Make up the multipart section
	multipartform.append(boundary.toAscii());
	multipartform.append(QString("Content-Disposition: form-data; name=\"" + JsonTag::FILE + "\"; filename=\"" + file.fileName() + "\"" + crlf).toAscii());
	multipartform.append(QString("Content-Type: application/octet-stream" + crlf + crlf).toAscii());
	multipartform.append(file.readAll());
	file.close();
	multipartform.append(QString(crlf + "--" + boundaryStr + "--" + crlf).toAscii());

	// Set the right multipart and boundary headers
	request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + boundaryStr);
	return true;
}

//-void HttpConnector::submissionReply( QNetworkReply * reply ) {
void HttpConnector::submissionReply() {

	kDebug()  << reply << " - File submission reply received. Processing data...";
	if( this->abortRequested ) {
		abortCurrentTask(); // Will free and reset all resources
	}
	else if( reply->error() == QNetworkReply::NoError ) {
		// Convert the replay into a JSON string
		const QString json( reply->readAll() );
		kDebug() << "Data: " << json;

		// Extract the scan Id from the JSON reply
		bool ok;
		QJson::Parser parser;
		QMap< QString, QVariant > data = parser.parse( json.toAscii(), &ok ).toMap();
		if( ok && !data.isEmpty() ) {
			// Set and check the service reply result
			setServiceReplyResult( data[ JsonTag::RESULT ].toInt() );
			const ServiceReplyResult::ServiceReplyResultEnum resultReply = getServiceReplyResult();
			if( resultReply == ServiceReplyResult::ITEM_PRESENT ) {
				// Emit the signal that informs about the scan Id
				const QString scanId = data[ JsonTag::SCAN_ID ].toString();
				if( !scanId.isEmpty() ){
					emit( scanIdReady() );
					switch( reportMode ) {
						case ReportMode::FILE_MODE:
							retrieveFileReport( scanId );
							break;
						case ReportMode::URL_MODE:
							retrieveUrlReport( scanId );
							break;
					}
				}
			}
			else if( resultReply == ServiceReplyResult::REQUEST_LIMIT_REACHED ) {
				// Reset the report mode and emit the service limit reached signal with value -1 (try later).
				reportMode = ReportMode::NONE;
				emit( serviceLimitReached( -1 ) );
			}
			else {
				QString msg;
				msg.append( i18n( "ERROR: Unexpected service result reply: %1", getServiceReplyResult() ) );
				kDebug() << msg;
				emit( errorOccurred( msg ) );
			}
		}
		else {
			QString msg( "ERROR: No valid reponse received!" );
			kDebug() << msg;
			emit( errorOccurred( msg ) );
		}
	}
	else {
		QString msg;
		msg.append( i18n( "ERROR: %1", reply->errorString() ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
	}

	// If the with are in file report mode and the file is open, it must be close
//	if( reportMode == ReportMode::FILE_MODE && file != NULL && file->isOpen() ) {
	// If the the file is open, it must be close
	if( file != NULL && file->isOpen() ) {
		file->close();
		file = NULL;
	}
}

void HttpConnector::retrieveFileReport( const QString & scanId ){
//	kDebug() << "Received scan Id " << scanId;
kDebug() << "Received scan Id " << scanId;
	emit( retrievingReport() );

	// Update the scan Id inner property
	this->scanId = scanId;

	// Prepare the data to submit
	QNetworkRequest request( QUrl( ServiceUrl::GET_FILE_REPORT ) );
	request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
	QByteArray params;
	params.append( JsonTag::KEY ).append( "=" ).append( key ).append( "&" ).
		   append( JsonTag::RESOURCE ).append( "=" ).append( scanId );

	// Submit data and establish all connections to this object from scratch
	reportMode = ReportMode::FILE_MODE;
	createNetworkReply( request, params );
	connect( reply, SIGNAL( finished() ), this, SLOT( reportComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( submissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving file report, waiting for reply...";
}

void HttpConnector::submitUrl( const QUrl & url2Scan ) {
	emit( startScanning() );
	if( !url2Scan.isValid() ) {
		QString msg;
		msg.append( i18n( "Invalid URL: %1", url2Scan.toString() ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}

	// Se the report mode. If other than none is active, abort.
	if( this->reportMode != ReportMode::NONE ) {
		QString msg;
		msg.append( i18n( "An invalid report mode is active: %1. Aborting...", reportMode ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}
	reportMode = ReportMode::URL_MODE;

	// Prepare the request
	QNetworkRequest request( ServiceUrl::SEND_URL_SCAN );
	QByteArray params;
	params.append( JsonTag::KEY ).append( "=" ).append( key ).append("&").
		   append( JsonTag::URL ).append( "=" ).append( url2Scan.toString() );

	 // Submit data and establish all connections to this object from scratch
	createNetworkReply( request, params );
	connect( reply, SIGNAL( finished() ), this, SLOT( submissionReply() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
		     this, SLOT( submissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "URL sent, waiting for reply...";
}

void HttpConnector::retrieveUrlReport( const QString& scanId ) {
	kDebug() << "Received scan Id " << scanId;
	emit( retrievingReport() );

	// Update the scan Id inner property
	this->scanId = scanId;

	// Prepare the data to submit
	QNetworkRequest request( QUrl( ServiceUrl::GET_URL_REPORT ) );
	request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
	QByteArray params;
	params.append( JsonTag::KEY ).append( "=" ).append( key ).append( "&" ).
		   append( JsonTag::SCAN ).append( "=1&").
		   append( JsonTag::RESOURCE ).append( "=" ).append( scanId );

	// Submit data and establish all connections to this object from scratch
	reportMode = ReportMode::URL_MODE;
	createNetworkReply( request, params );
	connect( reply, SIGNAL( finished() ), this, SLOT( reportComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( submissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving URL report, waiting for reply...";
}

void HttpConnector::reportChecker() {
	switch( reportMode ) {
		case ReportMode::FILE_MODE:
			retrieveFileReport( this->scanId );
			break;
		case ReportMode::URL_MODE:
			retrieveUrlReport( this->scanId );
			break;
	}
}

void HttpConnector::reportComplete() {
	kDebug() << "Report reply received. Processing data...";

	if( this->abortRequested ) {
		abortCurrentTask(); // Will free and reset all resources
	}
	else if( reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		const QString json( reply->readAll() );
		kDebug() << "Data: " << json;
		if( json.isEmpty() ) {
			kDebug() << "The reply is empty. Ignoring event... ";
			return;
		}

		// Extract the result from the JSON reply
		bool ok;
		QJson::Parser parser;
		QMap< QString, QVariant > jsonMap = parser.parse( json.toAscii(), &ok ).toMap();
		if( ok && !jsonMap.isEmpty() ) {
			// Set and check the service reply result and chekc if the result state. If not present, wait for some time...
			setServiceReplyResult( jsonMap[ JsonTag::RESULT ].toInt() );
			ServiceReplyResult::ServiceReplyResultEnum replyResult = getServiceReplyResult();
			if( replyResult == ServiceReplyResult::ITEM_NOT_PRESENT ) {
				if( reusingLastReport ) {
					kDebug() << QString( "There is no re-usable report. Submitting the file %1..." ).arg( fileName );
					reportMode = ReportMode::NONE;
					reusingLastReport = false;
					uploadFile( fileName );
					return;
				}
				requestDelay = getSuitableDelay( replyResult );
				kDebug() << "Report not ready yet, waiting for" << requestDelay << "seconds...";
				emit( waitingForReport( requestDelay ) );
				if( timer == NULL ) {
					kDebug() << "A new timer will be used.";
					timer = new QTimer();
					connect( timer, SIGNAL( timeout() ), this, SLOT( reportChecker() ) );
					timer->start( requestDelay * 1000 );
				}
				else {
					timer->stop();
					timer->start( requestDelay * 1000 );
				}
				return;
			}
			else if( replyResult == ServiceReplyResult::REQUEST_LIMIT_REACHED ) {
				requestDelay = getSuitableDelay( replyResult );
				kDebug() << "Service limit reached! Waiting for" << requestDelay << "seconds...";
				emit( serviceLimitReached( requestDelay ) );
				if( timer == NULL ) {
					kDebug() << "A new timer will be used.";
					timer = new QTimer();
					connect( timer, SIGNAL( timeout() ), this, SLOT( reportChecker() ) );
					timer->start( requestDelay * 1000 );
				}
				else {
					timer->stop();
					timer->start( requestDelay * 1000 );
				}
				return;
			}
			else if( replyResult == ServiceReplyResult::INVALID_SERVICE_KEY ) {
				kDebug() << "Emitting signal invalidServiceKeyError()...";
				emit( invalidServiceKeyError() );
				return;
			}
		}

		// If we have used a timer, disconect it and delete it
		if( timer != NULL ) {
			kDebug() << "The current timer will be stopped and deleted";
			timer->disconnect();
			delete timer;
			timer = NULL;
		}

		// Make up a new FileReport object and emits a signal
		switch( reportMode ) {
			case ReportMode::FILE_MODE: {
				FileReport * const report = new FileReport( json, fileName, hasher ); // Share the FileHasher object
				emit( fileReportReady( report ) );
				break;
			}
			case ReportMode::URL_MODE: {
				UrlReport * const report = new UrlReport( json );
				emit( urlReportReady( report ) );
				break;
			}
		}
	}
	else {
		kDebug() << "ERROR: " << reply->errorString();
		emit( errorOccurred( reply->errorString() ) );
	}

	// Set the report mode to none
	reportMode = ReportMode::NONE;
}

void HttpConnector::uploadProgressRate( qint64 bytesSent, qint64 bytesTotal ){
	kDebug() << "Uploaded " << bytesSent << "/" << bytesTotal << QString( "(%1 %)").arg( ( double ) bytesSent / bytesTotal * 100 , -1, 'f', 1, '0' );
}

void HttpConnector::downloadProgressRate( qint64 bytesSent, qint64 bytesTotal ){
	kDebug() << "Downloaded " << bytesSent << "/" << bytesTotal;
}

void HttpConnector::submissionReplyError( QNetworkReply::NetworkError error ) {
	QString errorMsg( reply->errorString() );
	errorMsg.append( " - (%1) " ).arg( error );
	kDebug() << "ERROR:" << errorMsg;
	emit( errorOccurred( errorMsg ) );
	reportMode = ReportMode::NONE;
}

ReportMode::ReportModeEnum HttpConnector::getReportMode() {
	return this->reportMode;
}

void HttpConnector::abort() {
	this->abortRequested = true;
	abortCurrentTask();
}

void HttpConnector::abortCurrentTask() {

	kDebug() << "Aborting the current task...";
	// If we have used a timer, disconect it and delete it
	if( timer != NULL ) {
		timer->stop();
		timer->disconnect();
		delete timer;
		timer = NULL;
	}

	// If the file is open, it must be close
	if( file != NULL && file->isOpen() ) {
		file->close();
		file = NULL;
	}

	// Reset the report mode and abort the given reply (will be deleted when the connector dies)
	reportMode = ReportMode::NONE;
	if( reply != NULL ) {
		reply->abort();
	}
	abortRequested = false;
	kDebug() << "Abortion action complete.";
	emit( aborted() );
}

ServiceRequestDelay::ServiceRequestDelayEnum HttpConnector::getSuitableDelay( ServiceReplyResult::ServiceReplyResultEnum result ) {
	switch( result ) {
	case ServiceReplyResult::ITEM_NOT_PRESENT:
		switch( requestDelay ) {
		case ServiceRequestDelay::DELAY_LEVEL_1:
		case ServiceRequestDelay::DELAY_LEVEL_2:
			return ServiceRequestDelay::DELAY_LEVEL_1;
		case ServiceRequestDelay::DELAY_LEVEL_3:
		default:
			return ServiceRequestDelay::DELAY_LEVEL_2;
		case ServiceRequestDelay::DELAY_LEVEL_4:
			return ServiceRequestDelay::DELAY_LEVEL_3;
		case ServiceRequestDelay::DELAY_LEVEL_5:
			return ServiceRequestDelay::DELAY_LEVEL_4;
		case ServiceRequestDelay::DELAY_LEVEL_6:
			return ServiceRequestDelay::DELAY_LEVEL_5;
		case ServiceRequestDelay::DELAY_LEVEL_7:
			return ServiceRequestDelay::DELAY_LEVEL_6;
		}
	case ServiceReplyResult::REQUEST_LIMIT_REACHED:
		switch( requestDelay ) {
		case ServiceRequestDelay::DELAY_LEVEL_1:
			return ServiceRequestDelay::DELAY_LEVEL_2;
		case ServiceRequestDelay::DELAY_LEVEL_2:
		default:
			return ServiceRequestDelay::DELAY_LEVEL_3;
		case ServiceRequestDelay::DELAY_LEVEL_3:
			return ServiceRequestDelay::DELAY_LEVEL_4;
		case ServiceRequestDelay::DELAY_LEVEL_4:
			return ServiceRequestDelay::DELAY_LEVEL_5;
		case ServiceRequestDelay::DELAY_LEVEL_5:
			return ServiceRequestDelay::DELAY_LEVEL_6;
		case ServiceRequestDelay::DELAY_LEVEL_6:
		case ServiceRequestDelay::DELAY_LEVEL_7:
			return ServiceRequestDelay::DELAY_LEVEL_7;
		}
	default:
		return ServiceRequestDelay::DELAY_LEVEL_2;
	};
}

#include "httpconnector.moc"