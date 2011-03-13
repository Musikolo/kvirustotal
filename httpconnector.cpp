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
#include <KLocalizedString>
#include <KDebug>

#include "filereport.h"
#include "httpconnector.h"
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
	static QString GET_SERVICE_WORKLOAD;
	static QString SEND_FILE_SCAN;
	static QString SEND_URL_SCAN;
	static QString MAKE_COMMENT;
}

// Static method
void HttpConnector::loadSettings() {
	static const QString GET_FILE_REPORT_PATTERN = "%1://www.virustotal.com/api/get_file_report.json";
	static const QString GET_URL_REPORT_PATTERN  = "%1://www.virustotal.com/api/get_url_report.json";
	static const QString GET_SERVICE_WORKLOAD	 = "%1://www.virustotal.com/get_workload.json";
	static const QString SEND_FILE_SCAN_PATTERN  = "%1://www.virustotal.com/api/scan_file.json";
	static const QString SEND_URL_SCAN_PATTERN   = "%1://www.virustotal.com/api/scan_url.json";
	static const QString MAKE_COMMENT_PATTERN    = "%1://www.virustotal.com/api/make_comment.json";
	
	const QString protocol( Settings::self()->secureProtocol() ? "https" : "http" );
	kDebug() << "Establishing the " << protocol << " protocol for any forthcomming connection...";
	ServiceUrl::GET_FILE_REPORT 	  = GET_FILE_REPORT_PATTERN.arg( protocol );
	ServiceUrl::GET_URL_REPORT  	  = GET_URL_REPORT_PATTERN.arg( protocol );
	ServiceUrl::GET_SERVICE_WORKLOAD  = GET_SERVICE_WORKLOAD.arg( protocol );
	ServiceUrl::SEND_FILE_SCAN  	  = SEND_FILE_SCAN_PATTERN.arg( protocol );
	ServiceUrl::SEND_URL_SCAN   	  = SEND_URL_SCAN_PATTERN.arg( protocol );
	ServiceUrl::MAKE_COMMENT    	  = MAKE_COMMENT_PATTERN.arg( protocol );
}

HttpConnector::HttpConnector( QNetworkAccessManager*const manager, const QString& key ) {
	this->manager = manager;
	this->key = key;
	this->reply = NULL;
	this->file = NULL;
	this->reportMode = ReportMode::NONE;
	this->abortRequested = false;
	this->hasher = NULL;
	this->reusingLastReport = false;
}

HttpConnector::~HttpConnector() {
	// Since we receive ownership of the reply object, we need to handle deletion.
	freeNetworkReply();
	if( hasher ) {
		delete hasher;
		hasher = NULL;
	}
}

bool HttpConnector::freeNetworkReply() {
	if( reply ) {
		reply->deleteLater();
		reply = NULL;
		return true;
	}
	return false;
}

void HttpConnector::createNetworkReply( const QNetworkRequest& request, const QByteArray& multipart, bool usePostMethod ) {
	// Since we receive ownership of the reply object, we need to handle deletion.
	if( freeNetworkReply() ) {
		kWarning() << "FREEING THE CURRENT REPLY. SINCE THIS MIGHT CAUSE PROBLEMS, IT'S RECOMMENDED TO CHECK THE ROOT OF THE CAUSE..!";
	}

	if( usePostMethod ) {
		this->reply = manager->post( request, multipart );
	}
	else {
		this->reply = manager->get( request );
	}
}

void HttpConnector::submitFile( const QString& fileName, const bool reuseLastReport ){
	kDebug() << "Submitting file " << fileName << "...";
	if( fileName.isEmpty() ) {
		kWarning() << "Nothing will be done as the file name is empty!";
		return;
	}
	this->fileName = fileName;

	if( reuseLastReport ) {
		retrieveLastReport( fileName );
	}
	else {
		uploadFile( fileName );
	}
}

void HttpConnector::retrieveLastReport( const QString& fileName ) {

	if( hasher ) {
		delete hasher;
	}
	kDebug() << "Generating hashes...";
	hasher = new FileHasher( fileName );
	kDebug() << "Reusing existing report, if possible...";
	reusingLastReport = true;
	emit( retrieveFileReport( hasher->getSha256Sum() ) );
}

void HttpConnector::uploadFile( const QString& fileName ) {

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
	reusingLastReport = false;
	createNetworkReply( request, multipartform );
	connect( reply, SIGNAL( finished() ), this, SLOT( submissionReply() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
		     this, SLOT( submissionReplyError( QNetworkReply::NetworkError ) ) );
	connect( reply, SIGNAL( uploadProgress( qint64, qint64 ) ),
			 this, SLOT( uploadProgressRate( qint64, qint64 ) ) );
	connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ),
			 this, SLOT( downloadProgressRate( qint64, qint64 ) ) );

	// Retransmit the uploadProgress signal (signal-to-signal)
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
		kError() << "Could not open file " << fileName;
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

void HttpConnector::submissionReply() {
	kDebug()  << reply << " - File submission reply received. Processing data...";
	if( !this->abortRequested && reply->error() == QNetworkReply::NoError ) {
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
					kDebug() << "Received scan Id " << scanId;
					emit( scanIdReady( scanId ) );
				}
			}
			else if( resultReply == ServiceReplyResult::REQUEST_LIMIT_REACHED ) {
				// Reset the report mode and emit the service limit reached signal
				reportMode = ReportMode::NONE;
				emit( serviceLimitReached() );
			}
			else {
				QString msg;
				msg.append( i18n( "ERROR: Unexpected service result reply: %1", getServiceReplyResult() ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
		else {
			QString msg( "ERROR: No valid response received!" );
			kError() << msg;
			emit( errorOccurred( msg ) );
		}
	}
	else {
		// This situation should be managed by submissionReplyError()
		kError() << "ERROR: " << reply->errorString();
	}

	// If the the file is open, it must be close
	if( file != NULL && file->isOpen() ) {
		file->close();
		file = NULL;
	}
	// The reply as it's no longer needed
	freeNetworkReply();
}

void HttpConnector::retrieveFileReport( const QString& scanId ){
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );
	
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

void HttpConnector::submitUrl( const QUrl& url2Scan ) {
	if( !url2Scan.isValid() ) {
		QString msg;
		msg.append( i18n( "Invalid URL: %1", url2Scan.toString() ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}

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
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );

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

void HttpConnector::reportComplete() {
	kDebug() << "Report reply received. Processing data...";
	if( !this->abortRequested && reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		const QString json( reply->readAll() );
		kDebug() << "Data: " << json;
		if( json.isEmpty() ) {
			kDebug() << "The reply is empty. Ignoring event... ";
			freeNetworkReply(); // Free the reply as it's no longer needed
			return;
		}

		// Extract the result from the JSON reply
		bool ok;
		QJson::Parser parser;
		QMap< QString, QVariant > jsonMap = parser.parse( json.toAscii(), &ok ).toMap();
		if( ok && !jsonMap.isEmpty() ) {
			// Set and check the service reply result and check the result state.
			setServiceReplyResult( jsonMap[ JsonTag::RESULT ].toInt() );
			ServiceReplyResult::ServiceReplyResultEnum replyResult = getServiceReplyResult();
			if( replyResult == ServiceReplyResult::ITEM_NOT_PRESENT ) {
				if( reusingLastReport ) {
					kDebug() << QString( "There is no re-usable report. Submitting the file %1..." ).arg( fileName );
					reportMode = ReportMode::NONE;
					reusingLastReport = false;
					freeNetworkReply(); // Free the reply as it's no longer needed
					uploadFile( fileName );
					return;
				}
				kDebug() << "Report not ready yet...";
				emit( reportNotReady() );
				freeNetworkReply(); // Free the reply as it's no longer needed
				return;
			}
			else if( replyResult == ServiceReplyResult::REQUEST_LIMIT_REACHED ) {
				kDebug() << "Service limit reached!";
				emit( serviceLimitReached() );
				freeNetworkReply(); // Free the reply as it's no longer needed
				return;
			}
			else if( replyResult == ServiceReplyResult::INVALID_SERVICE_KEY ) {
				kDebug() << "Emitting signal invalidServiceKeyError()...";
				emit( invalidServiceKeyError() );
				freeNetworkReply(); // Free the reply as it's no longer needed
				return;
			}
		}

		// Make up a new AbstractReport object and emits a signal
		switch( reportMode ) {
			case ReportMode::FILE_MODE: {
				FileReport*const report = new FileReport( json, fileName, hasher ); // Share the FileHasher object
				emit( reportReady( report ) );
				break;
			}
			case ReportMode::URL_MODE: {
				UrlReport*const report = new UrlReport( json );
				emit( reportReady( report ) );
				break;
			}
			case ReportMode::NONE:
			default:
				kWarning() << "Unexpected report mode. Please, check this situation!";
				break;
		}
	}
	else {
		// This situation should have been managed by the submissionReplyError() method
		kError() << "ERROR: " << reply->errorString();
	}

	// Set the report mode to none and free the reply
	reportMode = ReportMode::NONE;
	freeNetworkReply();
}

void HttpConnector::retrieveServiceWorkload() {
	// Prepare the data to submit
	QNetworkRequest request( QUrl( ServiceUrl::GET_SERVICE_WORKLOAD ) );
	createNetworkReply( request, QByteArray(), false );
	connect( reply, SIGNAL( finished() ), this, SLOT( onServiceWorkloadComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( submissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving URL report, waiting for reply...";
}

void HttpConnector::onServiceWorkloadComplete() {
	if( reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		const QString json( reply->readAll() );
		kDebug() << "Data: " << json;
		if( json.isEmpty() ) {
			// The reply must be freed here
			freeNetworkReply();
			kDebug() << "The reply is empty. Ignoring event... ";
			return;
		}

		// Extract the data from the JSON string
		bool ok;
		QJson::Parser parser;
		QMap< QString, QVariant > jsonMap = parser.parse( json.toAscii(), &ok ).toMap();
		if( ok && !jsonMap.isEmpty() ) {

			ServiceWorkload workload = { ( uchar ) jsonMap[ "file" ].toInt()
									   , ( uchar ) jsonMap[ "url"  ].toInt() };
			emit( serviceWorkloadReady( workload ) );
		} 
		else {
			kError() << "ERROR: An unknown error ocurred while processing the data";
		}
	}
	else {
		// This code should never be reached, but left as a guard
		kError() << "ERROR: " << reply->errorString(); //
	}
	// The reply must be freed here
	freeNetworkReply();
}

void HttpConnector::uploadProgressRate( qint64 bytesSent, qint64 bytesTotal ){
	kDebug() << "Uploaded " << bytesSent << "/" << bytesTotal << QString( "(%1 %)").arg( ( double ) bytesSent / bytesTotal * 100 , -1, 'f', 1, '0' );
}

void HttpConnector::downloadProgressRate( qint64 bytesSent, qint64 bytesTotal ){
	kDebug() << "Downloaded " << bytesSent << "/" << bytesTotal;
}

void HttpConnector::submissionReplyError( QNetworkReply::NetworkError error ) {
	// Emit the signal for all errors, but when either close() or abort() methods are invoked
	if( error != QNetworkReply::OperationCanceledError ){
		QString errorMsg( reply->errorString().append( " - (Error code: %1) " ).arg( error ) );
		kError() << "ERROR:" << errorMsg;
		emit( errorOccurred( errorMsg ) );
	}
	reportMode = ReportMode::NONE;
}

void HttpConnector::abort() {
	this->abortRequested = true;
	abortCurrentTask();
}

void HttpConnector::abortCurrentTask() {

	kDebug() << "Aborting the current task...";
	//Abort the given reply (will be deleted before the abortion completes)
	if( reply != NULL ) {
		reply->abort(); // Will throw a call to submittionReplyError() and finished() methods
	}

	// If the file is open, it must be close
	if( file != NULL && file->isOpen() ) {
		file->close();
		file = NULL;
	}
	freeNetworkReply();// The reply should have already been freed here

	// Reset the report mode and the flag
	reportMode = ReportMode::NONE;
	abortRequested = false;
	kDebug() << "Abortion action complete.";
	emit( aborted() );
}

#include "httpconnector.moc"