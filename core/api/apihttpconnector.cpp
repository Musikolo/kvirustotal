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
#include <KLocalizedString>
#include <KDebug>

#include "apihttpconnector.h"
#include "filereport.h"
#include "settings.h"
#include "servicereply.h"
#include "api/apiservicereply.h"
#include "api/apifilereport.h"
#include "api/apiurlreport.h"

static const int DELAY_LEVELS[] = { 15, 30, 45, 60, 90, 150, 300 };
static const qint64 SERVICE_MAX_FILE_SIZE = 20 * 1000 * 1000; // 20 MB
static QString PERMANENT_LINK_URL_PATTERN  = "http://www.virustotal.com/url-scan/report.html?id=%1";

/** Tags used to manage JSON objects. Extends ServiceBasicReply namespace. */
namespace JsonTag {
	static const QString KEY	  = "key";
	static const QString ID = "resource";
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
void ApiHttpConnector::loadSettings() {
	static const QString GET_FILE_REPORT_PATTERN = "%1://www.virustotal.com/api/get_file_report.json";
	static const QString GET_URL_REPORT_PATTERN  = "%1://www.virustotal.com/api/get_url_report.json";
	static const QString SEND_FILE_SCAN_PATTERN  = "%1://www.virustotal.com/api/scan_file.json";
	static const QString SEND_URL_SCAN_PATTERN   = "%1://www.virustotal.com/api/scan_url.json";
	static const QString MAKE_COMMENT_PATTERN    = "%1://www.virustotal.com/api/make_comment.json";

	const QString protocol( Settings::self()->secureProtocol() ? "https" : "http" );
	kDebug() << "Establishing the " << protocol << " protocol for any forthcomming connection...";
	ServiceUrl::GET_FILE_REPORT 	  = GET_FILE_REPORT_PATTERN.arg( protocol );
	ServiceUrl::GET_URL_REPORT  	  = GET_URL_REPORT_PATTERN.arg( protocol );
	ServiceUrl::SEND_FILE_SCAN  	  = SEND_FILE_SCAN_PATTERN.arg( protocol );
	ServiceUrl::SEND_URL_SCAN   	  = SEND_URL_SCAN_PATTERN.arg( protocol );
	ServiceUrl::MAKE_COMMENT    	  = MAKE_COMMENT_PATTERN.arg( protocol );
	
	BaseHttpConnector::loadSettings();
}

ApiHttpConnector::ApiHttpConnector( QNetworkAccessManager*const manager, const QString& key ) : BaseHttpConnector( manager ){
	this->key = key;
	this->reusingLastReport = false;
}

ApiHttpConnector::~ApiHttpConnector() {
}



void ApiHttpConnector::submitFile( const QString& fileName, const bool reuseLastReport ){
	kDebug() << "Submitting file " << fileName << "...";
	if( fileName.isEmpty() ) {
		kWarning() << "Nothing will be done as the file name is empty!";
		return;
	}
	setFileName( fileName );;

	if( reuseLastReport ) {
		retrieveLastReport();
	}
	else {
		uploadFile( fileName );
	}
}

void ApiHttpConnector::retrieveLastReport() {
	kDebug() << "Reusing existing report, if possible...";
	reusingLastReport = true;
	emit( retrieveFileReport( getFileHasher()->getSha256Sum() ) );
}

void ApiHttpConnector::uploadFile( const QString& fileName ) {

	// Set up the request
	QUrl url( ServiceUrl::SEND_FILE_SCAN );
	url.addQueryItem( JsonTag::KEY, key );
	QNetworkRequest request( url );

	// Prepare the multipart form
	QByteArray multipartform;
	if( !setupMultipartRequest( request, multipartform, fileName ) ) {
		QString msg( i18n( "An error has occurred while submitting the file %1...", fileName ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}

	// Submit data and establish all connections to this object from scratch
	reusingLastReport = false;
	createNetworkReply( request, multipartform );
	QNetworkReply*const reply = getNetworkReply();
	connect( reply, SIGNAL( finished() ), this, SLOT( onSubmissionReply() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
		     this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	connect( reply, SIGNAL( uploadProgress( qint64, qint64 ) ),
			 this, SLOT( onUploadProgressRate( qint64, qint64 ) ) );
	connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ),
			 this, SLOT( onDownloadProgressRate( qint64, qint64 ) ) );

	// Retransmit the uploadProgress signal (signal-to-signal)
	connect( reply, SIGNAL( uploadProgress( qint64, qint64 ) ),
			 this, SIGNAL( uploadingProgressRate( qint64, qint64 ) ) );

	kDebug() << "File sent, waiting for reply...";
}

void ApiHttpConnector::onSubmissionReply() {
	QNetworkReply*const reply = getNetworkReply();
	kDebug()  << reply << " - File submission reply received. Processing data...";
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		const QString jsonText( reply->readAll() );
		kDebug() << "Data: " << jsonText;

		// Extract the scan Id from the JSON reply
		ApiServiceReply jsonReply( jsonText );
		if( jsonReply.isValid() ) {
			// Check the service reply result
			if( jsonReply.getStatus() == ApiServiceReplyResult::ITEM_PRESENT ) {
				// Emit the signal that informs about the scan Id
				const QString scanId = jsonReply.getScanId();
				if( !scanId.isEmpty() ){
					kDebug() << "Received scan Id " << scanId;
					emit( scanIdReady( scanId ) );
				}
			}
			else if( jsonReply.getStatus() == ApiServiceReplyResult::REQUEST_LIMIT_REACHED ) {
				// Reset the report mode and emit the service limit reached signal
				setReportMode( ReportMode::NONE );
				emit( serviceLimitReached() );
			}
			else {
				QString msg;
				msg.append( i18n( "ERROR: Unexpected service result reply: %1", jsonReply.getStatus() ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
		else {
			QString msg( i18n( "ERROR: No valid response received!" ) );
			kError() << msg;
			emit( errorOccurred( msg ) );
		}
	}
	else {
		// This situation should be managed by submissionReplyError()
		kError() << "ERROR: " << reply->errorString();
	}

	// The reply as it's no longer needed
	freeNetworkReply();
}

void ApiHttpConnector::retrieveFileReport( const QString& scanId ){
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );
	
	// Prepare the data to submit
	QNetworkRequest request( QUrl( ServiceUrl::GET_FILE_REPORT ) );
	request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
	QByteArray params;
	params.append( JsonTag::KEY ).append( "=" ).append( key ).append( "&" ).
		   append( JsonTag::ID ).append( "=" ).append( scanId );

	// Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::FILE_MODE );
	QNetworkReply*const reply = createNetworkReply( request, params );
	connect( reply, SIGNAL( finished() ), this, SLOT( onReportComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving file report, waiting for reply...";
}

void ApiHttpConnector::submitUrl( const QUrl& url2Scan ) {
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
	QNetworkReply*const reply = createNetworkReply( request, params );
	connect( reply, SIGNAL( finished() ), this, SLOT( onSubmissionReply() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
		     this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "URL sent, waiting for reply...";
}

void ApiHttpConnector::retrieveUrlReport( const QString& scanId ) {
	this->scanId = scanId;
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );

	// Prepare the data to submit
	QNetworkRequest request( QUrl( ServiceUrl::GET_URL_REPORT ) );
	request.setHeader( QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded" );
	QByteArray params;
	params.append( JsonTag::KEY ).append( "=" ).append( key ).append( "&" ).
		   append( JsonTag::SCAN ).append( "=1&").
		   append( JsonTag::ID ).append( "=" ).append( scanId );

	// Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::URL_MODE );
	QNetworkReply*const reply = createNetworkReply( request, params );
	connect( reply, SIGNAL( finished() ), this, SLOT( onReportComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving URL report, waiting for reply...";
}

void ApiHttpConnector::onReportComplete() {
	kDebug() << "Report reply received. Processing data...";
	QNetworkReply*const reply = getNetworkReply();
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		const QString json( reply->readAll() );
		kDebug() << "Data: " << json;
		if( json.isEmpty() ) {
			kDebug() << "The reply is empty. Ignoring event... ";
			freeNetworkReply(); // Free the reply as it's no longer needed
			return;
		}

		// Extract the result from the JSON reply
		ApiServiceReply jsonReply( json );
		if( jsonReply.isValid() ) {
			// Do depending on the status value
			if( jsonReply.getStatus() == ApiServiceReplyResult::ITEM_NOT_PRESENT ) {
				if( reusingLastReport ) {
					kDebug() << QString( "There is no re-usable report. Submitting the file %1..." ).arg( getFileName() );
					setReportMode( ReportMode::NONE );
					reusingLastReport = false;
					freeNetworkReply(); // Free the reply as it's no longer needed
					uploadFile( getFileName() );
					return;
				}
				kDebug() << "Report not ready yet...";
				emit( reportNotReady() );
				freeNetworkReply(); // Free the reply as it's no longer needed
				return;
			}
			else if( jsonReply.getStatus() == ApiServiceReplyResult::REQUEST_LIMIT_REACHED ) {
				kDebug() << "Service limit reached!";
				emit( serviceLimitReached() );
				freeNetworkReply(); // Free the reply as it's no longer needed
				return;
			}
			else if( jsonReply.getStatus() == ApiServiceReplyResult::INVALID_SERVICE_KEY ) {
				kDebug() << "Emitting signal invalidServiceKeyError()...";
				emit( invalidServiceKeyError() );
				freeNetworkReply(); // Free the reply as it's no longer needed
				return;
			}
		}

		// Make up a new AbstractReport object and emits a signal
		switch( getReportMode() ) {
			case ReportMode::FILE_MODE: {
 				Report*const report = ( Report* ) new ApiFileReport( &jsonReply, getFileName(), getFileHasher() );
				emit( reportReady( report ) );
				break;
			}
			case ReportMode::URL_MODE: {
 				ApiUrlReport*const report = new ApiUrlReport( &jsonReply );
				report->setPermanentLink( PERMANENT_LINK_URL_PATTERN.arg( this->scanId ) );
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
	setReportMode( ReportMode::NONE );
	freeNetworkReply();
}


HttpConnectorCfg ApiHttpConnector::getFileHttpConnectorCfg() {
	const HttpConnectorCfg cfg = { (uchar)7, (uchar)4, DELAY_LEVELS, true, SERVICE_MAX_FILE_SIZE };
	return cfg;
}

HttpConnectorCfg ApiHttpConnector::getUrlHttpConnectorCfg() {
	const HttpConnectorCfg cfg = { (uchar)7, (uchar)2, DELAY_LEVELS, true, SERVICE_MAX_FILE_SIZE };
	return cfg;
}

#include "apihttpconnector.moc"