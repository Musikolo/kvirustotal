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


#ifndef HTTPCONNECTOR_H
#define HTTPCONNECTOR_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>
#include <QTimer>

#include "filereport.h"
#include "urlreport.h"

namespace ReportMode {
	enum ReportModeEnum { NONE, FILE_MODE, URL_MODE };
}

/** Delays used to wait for the service to have reports ready */
namespace ServiceRequestDelay {
	enum ServiceRequestDelayEnum {
		DELAY_LEVEL_1 = 30,
		DELAY_LEVEL_2 = 45,
		DELAY_LEVEL_3 = 60,
		DELAY_LEVEL_4 = 90,
		DELAY_LEVEL_5 = 120,
		DELAY_LEVEL_6 = 180,
		DELAY_LEVEL_7 = 300
	};
}

class HttpConnector : public QObject, public virtual ServiceBasicReply
{
Q_OBJECT
private:
	QNetworkAccessManager * manager;
	QString key;
	QFile* file;
	QString fileName;
	QTimer* timer;
    QString scanId;
	ReportMode::ReportModeEnum reportMode;
	bool abortRequested;
	QNetworkReply * reply;
	FileHasher* hasher;
	bool reusingLastReport;
	ServiceRequestDelay::ServiceRequestDelayEnum requestDelay;

    void retrieveLastReport(const QString& fileName);
	bool setupMultipartRequest( QNetworkRequest& request, QByteArray& multipartform, const QString& fileName );
	void abortCurrentTask();
	ServiceRequestDelay::ServiceRequestDelayEnum getSuitableDelay( ServiceReplyResult::ServiceReplyResultEnum result );

private slots:
	/** Process the reply of a previous file submission and emits retrieveFileReport() signal */
	void submissionReply();

	/** Deals with the reply of a previous file report request */
	void reportComplete();

	/** Deals with error that might happen during a file or URL submission */
	void submissionReplyError( QNetworkReply::NetworkError );

	/** Indicates the amount of sent data out of the total */
	void uploadProgressRate( qint64 bytesSent, qint64 bytesTotal );

	/** Indicates the amount of dowloaded data out of the total */
	void downloadProgressRate( qint64 bytesSent, qint64 bytesTotal );

	void createNetworkReply( const QNetworkRequest& request, const QByteArray& multipart );
	void reportChecker();

public:
	/** Does some initialization tasks common to all instances. Thus, it should be called before invoking the constructor. */
	static void loadSettings();
	
	/** Creates a HTTP connector object that will use the given network access manager and access key */
	HttpConnector( QNetworkAccessManager * manager, const QString & key );

	/** Destroys and frees all resources bound to this object */
	~HttpConnector();

	/** Tries to re-use an existant report, if possible. Otherwise, it calls uploadFile() method */
	void submitFile( const QString& fileName );

	/** Submits the given file */
	void uploadFile(const QString& fileName);

	/** Retrieves the file report object corresponding to the given scan Id. */
	void retrieveFileReport( const QString& scanId );

	/** Submits the given URL */
 	void submitUrl( const QUrl & url );

	/** Retrieves the URL report object corresponding to the given scan Id. */
	void retrieveUrlReport( const QString & scanId );

	/** Returns the current report mode being carried out */
	ReportMode::ReportModeEnum getReportMode();

	/** Request the current taks to be aborted */
	void abort();

//TODO: Implement this methods
/*	void makeComment( QFile file );
	void makeComment( QUrl url ); */

signals:
	void startScanning();
	void errorOccurred( const QString& message );
	void uploadingProgressRate( qint64 bytesSent, qint64 bytesTotal );
	void scanIdReady();
	void retrievingReport();
	void waitingForReport( int seconds );
	void serviceLimitReached( int seconds );
	void aborted();
	void invalidServiceKeyError();

	/** Emits when a file report is ready. The given object must be deleted when unneeded.  */
	void fileReportReady( FileReport* const report );

	/** Emits when a URL report is ready. The given object must be deleted when unneeded.  */
	void urlReportReady( UrlReport * const report );
};

#endif // HTTPCONNECTOR_H
