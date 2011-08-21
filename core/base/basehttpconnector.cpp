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
#include <QFileInfo>
#include <KDebug>

#include "basehttpconnector.h"
#include "filereport.h"
#include "servicereply.h"
#include "settings.h"

/** VirusTotal service URLs */
namespace ServiceUrl {
	static QString GET_SERVICE_WORKLOAD;
}

// Static method
void BaseHttpConnector::loadSettings() {
	static const QString GET_SERVICE_WORKLOAD	 = "%1://www.virustotal.com/get_workload.json";

	const QString protocol( Settings::self()->secureProtocol() ? "https" : "http" );
	kDebug() << "Establishing the " << protocol << " protocol for any forthcomming connection...";
	ServiceUrl::GET_SERVICE_WORKLOAD  = GET_SERVICE_WORKLOAD.arg( protocol );
}

BaseHttpConnector::BaseHttpConnector( QNetworkAccessManager*const manager ) {
	this->manager = manager;
	this->reply = NULL;
	this->reportMode = ReportMode::NONE;
	this->abortRequested = false;
	this->hasher = NULL;
}

BaseHttpConnector::~BaseHttpConnector() {
	// Since we receive ownership of the reply object, we need to handle deletion.
	freeNetworkReply();
	if( hasher ) {
		delete hasher;
		hasher = NULL;
	}
}

bool BaseHttpConnector::freeNetworkReply() {
	if( reply ) {
		reply->deleteLater();
		reply = NULL;
		return true;
	}
	return false;
}

QNetworkReply* BaseHttpConnector::createNetworkReply( const QNetworkRequest& request, const QByteArray& multipart, bool usePostMethod ) {
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
	return this->reply;
}

QNetworkReply* BaseHttpConnector::getNetworkReply() {
	return reply;
}

QString BaseHttpConnector::getFileName() {
	return fileName;
}

void BaseHttpConnector::setFileName( const QString& fileName ) {
	if( hasher != NULL ) {
		delete hasher;
	}
	kDebug() << "Generating hashes...";
	this->fileName = fileName;
	hasher = new FileHasher( fileName );
}

FileHasher* BaseHttpConnector::getFileHasher() {
	return hasher;
}

ReportMode::ReportModeEnum BaseHttpConnector::getReportMode() {
	return reportMode;
}

void BaseHttpConnector::setReportMode( ReportMode::ReportModeEnum reportMode ) {
	this->reportMode = reportMode;
}

bool BaseHttpConnector::isAbortRequested() {
	return abortRequested;
}


bool BaseHttpConnector::setupMultipartRequest( QNetworkRequest& request, QByteArray& multipartform, const QString& fileName ) {
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
	multipartform.append(QString("Content-Disposition: form-data; name=\"file\"; filename=\"" + QFileInfo( file ).fileName() + "\"" + crlf).toAscii());
	multipartform.append(QString("Content-Type: application/octet-stream" + crlf + crlf).toAscii());
	multipartform.append(file.readAll());
	file.close();
	multipartform.append(QString(crlf + "--" + boundaryStr + "--" + crlf).toAscii());

	// Set the right multipart and boundary headers
	request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + boundaryStr);
	return true;
}

void BaseHttpConnector::onRetrieveServiceWorkload() {
	// Prepare the data to submit
	QNetworkRequest request( QUrl( ServiceUrl::GET_SERVICE_WORKLOAD ) );
	createNetworkReply( request, QByteArray(), false );
	connect( reply, SIGNAL( finished() ), this, SLOT( onServiceWorkloadComplete() ) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
			 this, SLOT( submissionReplyError( QNetworkReply::NetworkError ) ) );
	kDebug() << "Retrieving URL report, waiting for reply...";
}

void BaseHttpConnector::onServiceWorkloadComplete() {
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
			kError() << "ERROR: An unknown error occured while processing the data";
		}
	}
	else {
		// This code should never be reached, but left as a guard
		kError() << "ERROR: " << reply->errorString(); //
	}
	// The reply must be freed here
	freeNetworkReply();
}

void BaseHttpConnector::onUploadProgressRate( qint64 bytesSent, qint64 bytesTotal ){
	kDebug() << "Uploaded " << bytesSent << "/" << bytesTotal << QString( "(%1 %)").arg( ( double ) bytesSent / bytesTotal * 100 , -1, 'f', 1, '0' );
}

void BaseHttpConnector::onDownloadProgressRate( qint64 bytesSent, qint64 bytesTotal ){
	kDebug() << "Downloaded " << bytesSent << "/" << bytesTotal;
}

void BaseHttpConnector::onSubmissionReplyError( QNetworkReply::NetworkError error ) {
	// Emit the signal for all errors, but when either close() or abort() methods are invoked
	if( error != QNetworkReply::OperationCanceledError ){
		QString errorMsg( reply->errorString().append( " - (Error code: %1) " ).arg( error ) );
		kError() << "ERROR:" << errorMsg;
		emit( errorOccured( errorMsg ) );
	}
	reportMode = ReportMode::NONE;
}

void BaseHttpConnector::abort() {
	this->abortRequested = true;
	abortCurrentTask();
}

void BaseHttpConnector::abortCurrentTask() {

	kDebug() << "Aborting the current task...";
	//Abort the given reply (will be deleted before the abortion completes)
	if( reply != NULL ) {
		reply->abort(); // Will throw a call to submittionReplyError() and finished() methods
	}

	freeNetworkReply();// The reply should have already been freed here

	// Reset the report mode and the flag
	reportMode = ReportMode::NONE;
	abortRequested = false;
	kDebug() << "Abortion action complete.";
	emit( aborted() );
}

#include "basehttpconnector.moc"