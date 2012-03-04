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

#include "webhttpconnector.h"
#include "filereport.h"
#include "settings.h"
#include "servicereply.h"
#include "web/webservicereply.h"
#include "web/webfilereport.h"
#include "web/weburlreport.h"
#include "uploadservicereply.h"
#include <QFileInfo>
#include <QTimer>
#include "urlsubmissionreply.h"

/** Tags used to manage JSON objects. Extends ServiceBasicReply namespace. */
namespace JsonTag {
	static const QString ID 	 = "id";
	static const QString URL 	 = "url";
	static const QString FILE	 = "file";
	static const QString SCAN 	 = "scan";
	static const QString SCAN_ID = "scan_id";
}

/** VirusTotal service URLs */
namespace ServiceUrl {
	static QString GET_CSFR_TOKEN;
	static QString GET_FILE_ID;
	static QString GET_FILE_REPORT;
	static QString GET_URL_REPORT;
	static QString FORCE_FILE_SCAN;
	static QString SEND_URL_SCAN;
// 	static QString MAKE_COMMENT;
}

static QString PERMANENT_LINK_FILE_PATTERN = "https://www.virustotal.com/file/%1/analysis/";
static QString PERMANENT_LINK_URL_PATTERN  = "https://www.virustotal.com/url/%1/analysis/";

// Static method
void WebHttpConnector::loadSettings() {
	static const QString GET_CSFR_TOKEN			 = "%1://www.virustotal.com";
	static const QString GET_FILE_ID_PATTERN	 = "%1://www.virustotal.com/file/upload/";
	static const QString GET_FILE_REPORT_PATTERN = "%1://www.virustotal.com/file/%2/analysis/%3/info/"; //%2=sha256, %3=timestamp
	static const QString GET_URL_REPORT_PATTERN  = "%1://www.virustotal.com/url/%2/analysis/%3/info/"; //%2=sha256, %3=timestamp
	static const QString FORCE_FILE_SCAN_PATTERN = "%1://www.virustotal.com/file/%2/reanalyse/"; //%2=sha256
	static const QString SEND_URL_SCAN_PATTERN   = "%1://www.virustotal.com/url/submission/";

	const QString protocol( Settings::self()->secureProtocol() ? "https" : "http" );
	kDebug() << "Establishing the " << protocol << " protocol for any forthcomming connection...";
	ServiceUrl::GET_CSFR_TOKEN  = GET_CSFR_TOKEN.arg( protocol );
	ServiceUrl::GET_FILE_ID 	= GET_FILE_ID_PATTERN.arg( protocol );
	ServiceUrl::GET_FILE_REPORT = GET_FILE_REPORT_PATTERN.arg( protocol );
	ServiceUrl::GET_URL_REPORT  = GET_URL_REPORT_PATTERN.arg( protocol );
	ServiceUrl::FORCE_FILE_SCAN = FORCE_FILE_SCAN_PATTERN.arg( protocol );
	ServiceUrl::SEND_URL_SCAN   = SEND_URL_SCAN_PATTERN.arg( protocol );
	
	BaseHttpConnector::loadSettings();
}

WebHttpConnector::WebHttpConnector( QNetworkAccessManager*const manager ) : BaseHttpConnector( manager ){
	this->reuseLastReport = true;
	this->scanReportForced = false;
	this->itemExistInServer = true;
}

WebHttpConnector::~WebHttpConnector() {
}

void WebHttpConnector::fetchCsfrToken() {
	// Prepare the URL
	QUrl url = QUrl( ServiceUrl::GET_CSFR_TOKEN ) ;
	QNetworkRequest request( url );
	QNetworkReply* reply = createNetworkReply( request );
	
	// Submit data and establish all connections to this object from scratch
	connect( reply, SIGNAL( finished() ), this, SLOT( onFetchCsrfToken() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving a token, waiting for reply...";	
}

void WebHttpConnector::onFetchCsrfToken() {
	kDebug() << "Reply received. Processing token...";
	QNetworkReply* reply = getNetworkReply();
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		QByteArray setCookieHeader = reply->rawHeader( "Set-Cookie" );
		freeNetworkReply();
		if( !setCookieHeader.isEmpty() ) {
			this->csfrToken = QString( setCookieHeader ).split( ";" )[0].toAscii();
			kDebug() << "Set-Cookie=" << csfrToken;
			csfrToken = QString( csfrToken ).split("=")[1].toAscii();
			kDebug() << "X-CSRFToken=" << csfrToken;
			
			// Get the service Id
			setReportMode( ReportMode::URL_MODE );
			retrieveServiceId();
		}
		else {
			emit( errorOccurred( i18n( "The reply did not have a valid token. The operation must be aborted!" ) ) );
		}
	}
}

void WebHttpConnector::retrieveServiceId() {
	QNetworkReply* reply = NULL;
	// File mode
	if( getReportMode() == ReportMode::FILE_MODE ) {
		// Prepare the URL to submit
		QUrl url( ServiceUrl::GET_FILE_ID );
		url.addQueryItem( "sha256", getFileHasher()->getSha256Sum() );
#if QT_VERSION >= 0x040700
		url.addQueryItem( "_", QString::number( QDateTime::currentMSecsSinceEpoch() ) );
#else
		url.addQueryItem( "_", QString::number( QDateTime::currentDateTime().toTime_t() ) );
#endif
		// Submit data
		QNetworkRequest request( url );
		reply = createNetworkReply( request );
	}
	// Url mode
	else {
		// Prepare the URL to submit
		QUrl url( ServiceUrl::SEND_URL_SCAN );
		QByteArray params; 
		params.append( "url=" ).append( QUrl::toPercentEncoding( this->url2Scan.toString() ) );

		// Set also some required headers
		QNetworkRequest request( url );
		request.setRawHeader( "X-CSRFToken", csfrToken );
		request.setRawHeader( "Referer", ServiceUrl::GET_CSFR_TOKEN.toAscii() );
		// Submit data
		request.setRawHeader( "Cookie", "csrftoken=" + csfrToken );
		reply = createNetworkReply( request, params, true ); // true = use POST method
	}

	// Submit data and establish all connections to this object from scratch
	connect( reply, SIGNAL( finished() ), this, SLOT( onServiceIdComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving service id, waiting for reply...";	
}


void WebHttpConnector::onServiceIdComplete() {
	QNetworkReply*const reply = getNetworkReply();
	kDebug() << "Service ID received. Processing data...";
	
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		const QString jsonText = reply->readAll();
		freeNetworkReply();
		
		// File mode
		if( getReportMode() == ReportMode::FILE_MODE ) {
			// Convert the reply into a JSON string
			this->scanId = getFileHasher()->getSha256Sum();
			const UploadServiceReply serviceReply( jsonText );
			if( serviceReply.isValid() ){
				this->itemExistInServer = serviceReply.isFileExists();
				if( serviceReply.isFileExists() ) {
					if( this->reuseLastReport ) {
						retrieveLastReport();
					}
					else {
						forceScanReport();
					}
				}
				else {
					this->uploadUrl = serviceReply.getUploadUrl();
					uploadFile( getFileName() );
				}
			}
			else {
				QString msg( i18n( "ERROR: No valid response received!" ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
		// Url mode
		else {
			const UrlSubmissionReply serviceReply( jsonText );
			if( serviceReply.isValid() ) {
				this->itemExistInServer = serviceReply.isUrlExists();
				this->scanId = serviceReply.getScanId();
				emit( scanIdReady( this->scanId ) );
			}
			else {
				QString msg( i18n( "ERROR: No valid response received!" ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
	}
	else {
		freeNetworkReply();
	}
}

void WebHttpConnector::submitFile( const QString& fileName, const bool reuseLastReport ){
	kDebug() << "Submitting file " << fileName << "...";
	if( fileName.isEmpty() ) {
		kWarning() << "Nothing will be done as the file name is empty!";
		return;
	}
	setFileName( fileName );
	this->reuseLastReport = reuseLastReport;
	setReportMode( ReportMode::FILE_MODE );
	retrieveServiceId();
}

void WebHttpConnector::retrieveLastReport() {
	kDebug() << "Reusing existing report, if possible...";
	reuseLastReport = true;
	emit( retrieveFileReport( this->scanId ) );
}

void WebHttpConnector::uploadFile( const QString& fileName ) {

	// Set up the request
	kDebug() << "Uploading to" << this->uploadUrl;
	QUrl url( QString( this->uploadUrl ) );
	QNetworkRequest request( url );

	// Prepare the multipart form
	QMap< QString, QString > params;
	params[ "ajax" ] = "true";
	params[ "remote_addr" ] = "127.0.0.1";
	params[ "sha256" ] = getFileHasher()->getSha256Sum();
	QByteArray multipartform;
	if( !setupMultipartRequest( request, multipartform, fileName, params ) ) {
		QString msg( i18n( "An error has occurred while submitting the file %1...", fileName ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}

	// Submit data and establish all connections to this object from scratch
	reuseLastReport = false;
	scanReportForced = false; // Update flag
	createNetworkReply( request, multipartform, true );
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

void WebHttpConnector::onSubmissionReply() {
	QNetworkReply*const reply = getNetworkReply();
	kDebug()  << reply << " - File submission reply received. Processing data...";
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		const QString jsonText( reply->readAll() );
		kDebug() << "Data: " << jsonText;
		freeNetworkReply();// The reply as it's no longer needed

		const ReportMode::ReportModeEnum reportMode = getReportMode();
		if( reportMode == ReportMode::FILE_MODE ) {
			this->scanId = getFileHasher()->getSha256Sum();
			emit( scanIdReady( this->scanId ) );
		}
	}
	else {
		// This situation should be managed by submissionReplyError()
		kError() << "ERROR: " << reply->errorString();
		freeNetworkReply();// The reply as it's no longer needed
	}
}

void WebHttpConnector::forceScanReport() {
	// Set the init forced scan date
	this->scanForcedInitDate = QDateTime::currentDateTime().addSecs( -1 );
	const ReportMode:: ReportModeEnum reportMode = getReportMode();
	if( reportMode == ReportMode::FILE_MODE ) {
		QUrl url( ServiceUrl::FORCE_FILE_SCAN.arg( getFileHasher()->getSha256Sum() ) );
		url.addQueryItem( "filename", QFileInfo( this->getFileName() ).fileName() );
		QNetworkRequest request( url );
		QNetworkReply*const reply = createNetworkReply( request );
		connect( reply, SIGNAL( finished() ), this, SLOT( onScanForcedComplete() ) );
		connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
				this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
		kDebug() << "Forcing scan file report, waiting for reply...";
	}
	else {
		// Prepare the URL to submit
		QUrl url( ServiceUrl::SEND_URL_SCAN );
		
		// Set some required params
		url.addQueryItem( "force", "1" );
		url.addQueryItem( "url", this->url2Scan.toString() );
		
		// Submit data
		QNetworkRequest request( url );
		QNetworkReply*const reply = createNetworkReply( request ); 
		
		// Establish all connections to this object from scratch
		connect( reply, SIGNAL( finished() ), this, SLOT( onScanForcedComplete() ) );
		connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
				this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
		kDebug() << "Forcing scan url report, waiting for reply...";	
	}
}

void WebHttpConnector::onScanForcedComplete() {
	kDebug() << "Forced scan report request completed!";
	this->scanReportForced = true; // Update flag
	QNetworkReply*const reply = getNetworkReply();
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		freeNetworkReply();
		if( getReportMode() ==  ReportMode::FILE_MODE ) {
			emit( scanIdReady( getFileHasher()->getSha256Sum() ) );
		}
		else {
			emit( reportNotReady() );
		}
	}
	else {
		// This situation should have been managed by the submissionReplyError() method
		kError() << "ERROR: " << reply->errorString();
		freeNetworkReply();// The reply as it's no longer needed
	}
}

void WebHttpConnector::retrieveFileReport( const QString& scanId ){
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );

	// Prepare the data to submit
	#if QT_VERSION >= 0x040700
	const QString timestamp = QString::number( QDateTime::currentMSecsSinceEpoch() );
#else
	const QString timestamp = QString::number( QDateTime::currentDateTime().toTime_t() );
#endif
	QUrl url( ServiceUrl::GET_FILE_REPORT.arg( getFileHasher()->getSha256Sum() ).arg( timestamp ) );
	url.addQueryItem( "last-status", "analysing" );
	url.addQueryItem( "_", timestamp );
	QNetworkRequest request( url );

	// Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::FILE_MODE );
	QNetworkReply*const reply = createNetworkReply( request );
	connect( reply, SIGNAL( finished() ), this, SLOT( onReportComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving file report, waiting for reply...";
}

void WebHttpConnector::submitUrl( const QUrl& url2Scan, const bool reuseLastReport ) {
	if( !url2Scan.isValid() ) {
		QString msg;
		msg.append( i18n( "Invalid URL: %1", url2Scan.toString() ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}

	this->url2Scan = url2Scan;
	this->reuseLastReport = reuseLastReport;
	setReportMode( ReportMode::URL_MODE );
	fetchCsfrToken();
}

void WebHttpConnector::retrieveUrlReport( const QString& scanId ) {
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );

	// If the we want to force the scan, go ahead!
	if( !this->reuseLastReport && !this->scanReportForced ) {
		forceScanReport();
		return;
	}
	
	// Prepare the data to submit
#if QT_VERSION >= 0x040700
	const QString timestamp = QString::number( QDateTime::currentMSecsSinceEpoch() );
#else
	const QString timestamp = QString::number( QDateTime::currentDateTime().toTime_t() );
#endif
	QUrl url( ServiceUrl::GET_URL_REPORT.arg( this->scanId ).arg( timestamp ) );
	url.addQueryItem( "last-status", "analysing" );
	url.addQueryItem( "_", timestamp );
	QNetworkRequest request( url );

	// Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::URL_MODE );
	QNetworkReply*const reply = createNetworkReply( request );
	connect( reply, SIGNAL( finished() ), this, SLOT( onReportComplete() ) );
	// Exception 3: New URLs analysis return wrong reply sometimes
	if( this->itemExistInServer ) {
		connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
				this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	}
	kDebug() << "Retrieving URL report, waiting for reply...";
}

void WebHttpConnector::onReportComplete() {
	kDebug() << "Report reply received. Processing data...";
	QNetworkReply*const reply = getNetworkReply();
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		const QString json( reply->readAll() );
		kDebug() << "Data: " << json;
		freeNetworkReply();// The reply as it's no longer needed
		if( json.isEmpty() ) {
			kDebug() << "The reply is empty. Ignoring event... ";
			return;
		}
	
		const ReportMode:: ReportModeEnum reportMode = getReportMode();
		if( reportMode == ReportMode::FILE_MODE ) {
			// Extract the result from the JSON reply
			WebServiceReply jsonReply( json, WebServiceReplyType::FILE );
			if( jsonReply.isValid() ) {
				WebServiceReplyResult::WebServiceReplyResultEnum status = jsonReply.getStatus();
				// Exception 1: For rescans, the 'completed' returned status must be ignored till the scan date is changed.
				if( scanReportForced && status == WebServiceReplyResult::SCAN_COMPLETED ) {
					if( this->scanForcedInitDate.isValid() && this->scanForcedInitDate > jsonReply.getScanDate() ){
						kDebug() << "The report scan date has not been updated yet. Forcing status to SCAN_RUNNING...";
						status = WebServiceReplyResult::SCAN_RUNNING;
					}
				}
				// Exception 2: New submission, will return failed status till 'completed' is returned. Weird, but not pending or analysing values are returned!!
				else if( !this->itemExistInServer &&  status == WebServiceReplyResult::SCAN_ERROR ){
					kDebug() << "Ignoring error reply for newly submitted file. Forcing SCAN_RUNNING status...";
					status = WebServiceReplyResult::SCAN_RUNNING;	
				}
				
				// Evaluate the status accordingly
				switch( status ) {
					case WebServiceReplyResult::SCAN_STARTED:
						kDebug() << "Scan has been started...";
					case WebServiceReplyResult::SCAN_QUEUED:
					case WebServiceReplyResult::SCAN_RUNNING:{
						kDebug() << "Report not ready yet...";
						emit( reportNotReady() );
						return;
					}
					case WebServiceReplyResult::SCAN_COMPLETED: {
						// Make up a new AbstractReport object and emits a signal
						WebFileReport*const report = new WebFileReport( &jsonReply, getFileName(), getFileHasher() );
						report->setPermanentLink(  QUrl( PERMANENT_LINK_FILE_PATTERN.arg( getFileHasher()->getSha256Sum() ) ) );
						emit( reportReady( report ) );
						break;
					}
					default:
						kError() << "Unexpected service reply. Please, check this situation!";
					case WebServiceReplyResult::SCAN_ERROR: {
						QString msg( i18n( "ERROR: No valid status response received!" ) );
						kError() << msg;
						emit( errorOccurred( msg ) );
						break;
					}
				}
			}
			else {
				QString msg( i18n( "ERROR: No valid response received!" ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
		else {
			// Extract the result from the JSON reply
			WebServiceReply jsonReply( json, WebServiceReplyType::URL_SERVICE );
			if( jsonReply.isValid() ) {
				WebServiceReplyResult::WebServiceReplyResultEnum status = jsonReply.getStatus();
				// Exception 1: For rescans, the 'completed' returned status must be ignored till the scan date is changed.
				if( scanReportForced && status == WebServiceReplyResult::SCAN_COMPLETED ) {
					if( this->scanForcedInitDate.isValid() && this->scanForcedInitDate > jsonReply.getScanDate() ){
						kDebug() << "The report scan date has not been updated yet. Forcing status to SCAN_RUNNING...";
						status = WebServiceReplyResult::SCAN_RUNNING;
					}
				}

				switch( status ) {
					case WebServiceReplyResult::SCAN_STARTED:
						kDebug() << "Scan has been started...";
					case WebServiceReplyResult::SCAN_RUNNING:{
						kDebug() << "Report not ready yet...";
						emit( reportNotReady() );
						return;
					}
					case WebServiceReplyResult::SCAN_COMPLETED: {
						// Make up a new AbstractReport object and emits a signal
						WebUrlReport*const report = new WebUrlReport( &jsonReply );
						report->setPermanentLink(  QUrl( PERMANENT_LINK_URL_PATTERN.arg( this->scanId ) ) );
						emit( reportReady( report ) );
						break;
					}
					default: {
						kError() << "Unexpected service reply. Please, check this situation!";
						QString msg( i18n( "ERROR: No valid status response received!" ) );
						kError() << msg;
						emit( errorOccurred( msg ) );
						break;
					}
				}
			}
			else {
				QString msg( i18n( "ERROR: No valid response received!" ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
	}
	else {
		// Exception 3: New URLs analysis return wrong reply sometimes
		if( this->getReportMode() == ReportMode::URL_MODE && !itemExistInServer && reply->error() == QNetworkReply::UnknownContentError ) {
			kDebug() << "Wrong reply. Retrying once again...";
			emit( reportNotReady() );
		}
		// This situation should have been managed by the submissionReplyError() method
		else {
			kError() << "ERROR: " << reply->errorString();
		}
		freeNetworkReply();// The reply as it's no longer needed
	}
	// Set the report mode to none
	setReportMode( ReportMode::NONE );
}

HttpConnectorCfg WebHttpConnector::getFileHttpConnectorCfg() {
	static const int levels[] = { 5 };
	HttpConnectorCfg cfg = { (uchar)1, (uchar)0, levels, false, 32 * 1000 * 1000 }; // 32MB
	return cfg;
}

HttpConnectorCfg WebHttpConnector::getUrlHttpConnectorCfg() {
	static const int levels[] = { 5 };
	HttpConnectorCfg cfg = { (uchar)1, (uchar)0, levels, false, 32 * 1000 * 1000 }; // 32MB
	return cfg;
}

#include "webhttpconnector.moc"