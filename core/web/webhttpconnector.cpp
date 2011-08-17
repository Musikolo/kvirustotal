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
	static QString GET_FILE_ID;
	static QString GET_FILE_REPORT;
	static QString GET_URL_REPORT;
	static QString FORCE_FILE_SCAN;
	static QString SEND_FILE_SCAN;
	static QString SEND_URL_SCAN;
// 	static QString MAKE_COMMENT;
}

static QString PERMANENT_LINK_FILE_PATTERN = "http://www.virustotal.com/file-scan/report.html?id=%1";
static QString PERMANENT_LINK_URL_PATTERN  = "http://www.virustotal.com/url-scan/report.html?id=%1";

// Static method
void WebHttpConnector::loadSettings() {
	static const QString GET_FILE_ID_PATTERN	 = "%1://www.virustotal.com/file-scan/get_identifier.json";
	static const QString GET_FILE_REPORT_PATTERN = "%1://www.virustotal.com/file-scan/get_scan.json";
	static const QString GET_URL_REPORT_PATTERN  = "%1://www.virustotal.com/url-scan/get_url_analysis.json";
	static const QString FORCE_FILE_SCAN_PATTERN = "%1://www.virustotal.com/file-scan/reanalysis.html";
	static const QString SEND_FILE_SCAN_PATTERN  = "%1://www.virustotal.com/file-upload/file_upload";
	static const QString SEND_URL_SCAN_PATTERN   = "%1://www.virustotal.com/url-scan/was_url_analysed.json";
// 	static const QString MAKE_COMMENT_PATTERN    = "%1://www.virustotal.com/api/make_comment.json";

	const QString protocol( Settings::self()->secureProtocol() ? "https" : "http" );
	kDebug() << "Establishing the " << protocol << " protocol for any forthcomming connection...";
	ServiceUrl::GET_FILE_ID 	= GET_FILE_ID_PATTERN.arg( protocol );
	ServiceUrl::GET_FILE_REPORT = GET_FILE_REPORT_PATTERN.arg( protocol );
	ServiceUrl::GET_URL_REPORT  = GET_URL_REPORT_PATTERN.arg( protocol );
	ServiceUrl::FORCE_FILE_SCAN = FORCE_FILE_SCAN_PATTERN.arg( protocol );
	ServiceUrl::SEND_FILE_SCAN  = SEND_FILE_SCAN_PATTERN.arg( protocol );
	ServiceUrl::SEND_URL_SCAN   = SEND_URL_SCAN_PATTERN.arg( protocol );
// 	ServiceUrl::MAKE_COMMENT    = MAKE_COMMENT_PATTERN.arg( protocol );
	
	BaseHttpConnector::loadSettings();
}

WebHttpConnector::WebHttpConnector( QNetworkAccessManager*const manager ) : BaseHttpConnector( manager ){
	this->reusingLastReport = false;
	this->scanReportForced = false;
}

WebHttpConnector::~WebHttpConnector() {
}

void WebHttpConnector::retrieveServiceId() {
	// Prepare the URL to submit
	QUrl url( ServiceUrl::GET_FILE_ID );
	url.addQueryItem( "_", QString::number( QDateTime::currentMSecsSinceEpoch() ) ) ;
 	QNetworkRequest request( url );

	// Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::FILE_MODE );
	QNetworkReply*const reply = createNetworkReply( request, "", false ); // false = user GET method
	connect( reply, SIGNAL( finished() ), this, SLOT( onServiceIdComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving service id, waiting for reply...";	
}


void WebHttpConnector::onServiceIdComplete() {
	QNetworkReply*const reply = getNetworkReply();
	kDebug() << "Service ID received. Processing data...";
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		// Convert the reply into a JSON string
		QString serviceId( reply->readAll() );
		serviceId.remove( "\"" );
		if( !serviceId.isEmpty() ) {
			freeNetworkReply();
			kDebug() << "Service identifier" << serviceId << "received!";
			this->serviceId = serviceId;
//FIXME: Find out the way to pass through this variable from submitFile() method!
bool reuseLastReport = false;
			if( reuseLastReport ) {
				retrieveLastReport();
			}
			else {
				uploadFile( getFileName() );
			}
		}
		else {
			QString msg( i18n( "ERROR: No valid response received!" ) );
			kError() << msg;
			emit( errorOccurred( msg ) );
			freeNetworkReply();
		}
	}
	else {
		freeNetworkReply();
	}
}

void WebHttpConnector::submitFile( const QString& fileName, const bool reuseLastReport ){
	Q_UNUSED( reuseLastReport );
	kDebug() << "Submitting file " << fileName << "...";
	if( fileName.isEmpty() ) {
		kWarning() << "Nothing will be done as the file name is empty!";
		return;
	}
	setFileName( fileName );
	retrieveServiceId();
}

void WebHttpConnector::retrieveLastReport() {
	kDebug() << "Reusing existing report, if possible...";
	reusingLastReport = true;
	emit( retrieveFileReport( getFileHasher()->getSha256Sum() ) );
}

void WebHttpConnector::uploadFile( const QString& fileName ) {

	// Set up the request
	QUrl url( QString( ServiceUrl::SEND_FILE_SCAN ).append( "?" ).append( this->serviceId ) );
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
	scanReportForced = false; // Update flag
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
			// Extract the scan Id from the JSON reply
			WebServiceReply jsonReply( jsonText, WebServiceReplyType::FILE, true ); // true = htmlReply
			if( jsonReply.isValid() ) {
				// Emit the signal that informs about the scan Id is ready
				this->scanId = jsonReply.getScanId(); // Set the scanId in this object
				forceScanReport();
			}
			else {
				QString msg( i18n( "ERROR: No valid response received!" ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
		else if( reportMode == ReportMode::URL_MODE ) {
			WebServiceReply jsonReply( jsonText, WebServiceReplyType::URL_SERVICE_1 );
			if( jsonReply.isValid() && jsonReply.getStatus() != WebServiceReplyResult::SCAN_ERROR ) {
				this->scanId = jsonReply.getScanId(); // Set the scanId in this object. Used to build the permanentLink
				emit( scanIdReady( jsonReply.getScanId() ) );
			}
			else {
				QString msg( i18n( "ERROR: No valid response received!" ) );
				kError() << msg;
				emit( errorOccurred( msg ) );
			}
		}
	}
	else {
		// This situation should be managed by submissionReplyError()
		kError() << "ERROR: " << reply->errorString();
		freeNetworkReply();// The reply as it's no longer needed
	}
}

void WebHttpConnector::forceScanReport() {
	QUrl url( ServiceUrl::FORCE_FILE_SCAN );
	url.addQueryItem( "id", this->scanId );
	url.addQueryItem( "force", "1" );
	QNetworkRequest request( url );
	QNetworkReply*const reply = createNetworkReply( request, "", false ); // false = use GET method
	connect( reply, SIGNAL( finished() ), this, SLOT( onScanForcedComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Forcing scan file report, waiting for reply...";
}

void WebHttpConnector::onScanForcedComplete() {
	kDebug() << "Forced scan report request completed!";
	QNetworkReply*const reply = getNetworkReply();
	if( !isAbortRequested() && reply->error() == QNetworkReply::NoError ) {
		scanReportForced = true; // Update flag
		freeNetworkReply();
		if( !scanId.isEmpty() ){
			kDebug() << "Received scan Id " << scanId;
			emit( scanIdReady( scanId ) );
		}
		else {
			kError() << "ERROR: No scanId ready. Cannot continue!";
		}
	}
	else {
		// This situation should have been managed by the submissionReplyError() method
		kError() << "ERROR: " << reply->errorString();
		freeNetworkReply();// The reply as it's no longer needed
	}
}

void WebHttpConnector::commonRetrieveFileReport() {
	// Prepare the data to submit
	QUrl url( ServiceUrl::GET_FILE_REPORT );
	url.addQueryItem( "_", QString::number( QDateTime::currentMSecsSinceEpoch() ) ) ;
	url.addQueryItem( JsonTag::ID, scanId );
	QNetworkRequest request( url );

	// Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::FILE_MODE );
	QNetworkReply*const reply = createNetworkReply( request, "", false ); // false = use GET method
	connect( reply, SIGNAL( finished() ), this, SLOT( onReportComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving file report, waiting for reply...";
}


void WebHttpConnector::retrieveFileReport( const QString& scanId ){
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );
	
	if( !scanReportForced ) {
		forceScanReport();
	}
	else {
		commonRetrieveFileReport();
	}	
}

void WebHttpConnector::submitUrl( const QUrl& url2Scan ) {
	if( !url2Scan.isValid() ) {
		QString msg;
		msg.append( i18n( "Invalid URL: %1", url2Scan.toString() ) );
		kDebug() << msg;
		emit( errorOccurred( msg ) );
		return;
	}

//TODO: See how to pass the reuseLastReport variable
const bool reuseLastReport = false;

	// Prepare the request
	QNetworkRequest request( ServiceUrl::SEND_URL_SCAN );
	QByteArray params;
	params.append( "url=" ).append( url2Scan.toString() ).
		   append( "&force=" ).append( reuseLastReport ? "0" : "1" ) ;

	 // Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::URL_MODE );
	QNetworkReply*const reply = createNetworkReply( request, params );
	connect( reply, SIGNAL( finished() ), this, SLOT( onSubmissionReply() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
		     this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "URL sent, waiting for reply...";
}

void WebHttpConnector::retrieveUrlReport( const QString& scanId ) {
	kDebug() << "Using scan Id " << scanId;
	emit( retrievingReport() );

	// Prepare the data to submit
	QUrl url( ServiceUrl::GET_URL_REPORT );
	url.addQueryItem( "_", QString::number( QDateTime::currentMSecsSinceEpoch() ) );
	url.addQueryItem( "id", scanId );
	QNetworkRequest request( url );

	// Submit data and establish all connections to this object from scratch
	setReportMode( ReportMode::URL_MODE );
	QNetworkReply*const reply = createNetworkReply( request, "", false );
	connect( reply, SIGNAL( finished() ), this, SLOT( onReportComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( onSubmissionReplyError( QNetworkReply::NetworkError ) ) );
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
				
				switch( jsonReply.getStatus() ) {
					case WebServiceReplyResult::SCAN_STARTED:
						kDebug() << "Scan has been started...";
					case WebServiceReplyResult::SCAN_QUEUED:
					case WebServiceReplyResult::SCAN_RUNNING:{
						if( reusingLastReport ) {
							kDebug() << QString( "There is no re-usable report. Submitting the file %1..." ).arg( getFileName() );
							setReportMode( ReportMode::NONE );
							reusingLastReport = false;
							uploadFile( getFileName() );
							return;
						}
						kDebug() << "Report not ready yet...";
						emit( reportNotReady() );
						return;
					}
					case WebServiceReplyResult::SCAN_COMPLETED: {
						// Make up a new AbstractReport object and emits a signal
						WebFileReport*const report = new WebFileReport( &jsonReply, getFileName(), getFileHasher() );
						report->setPermanentLink(  QUrl( PERMANENT_LINK_FILE_PATTERN.arg( this->scanId ) ) );
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
			WebServiceReply jsonReply( json, WebServiceReplyType::URL_SERVICE_2 );
			if( jsonReply.isValid() ) {
				
				switch( jsonReply.getStatus() ) {
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
//						report->setPermanentLink(  QUrl( PERMANENT_LINK_URL_PATTERN.arg( jsonReply.getFileReportId() ) ) );
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
		// This situation should have been managed by the submissionReplyError() method
		kError() << "ERROR: " << reply->errorString();
		freeNetworkReply();// The reply as it's no longer needed
	}
	// Set the report mode to none
	setReportMode( ReportMode::NONE );
}

HttpConnectorCfg WebHttpConnector::getFileHttpConnectorCfg() {
	static const int levels[] = { 5 };
	HttpConnectorCfg cfg = { (uchar)1, (uchar)0, levels, false };
	return cfg;
}

HttpConnectorCfg WebHttpConnector::getUrlHttpConnectorCfg() {
	return getFileHttpConnectorCfg();
}

#include "webhttpconnector.moc"