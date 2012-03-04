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

#ifndef BASEHTTPCONNECTOR_H
#define BASEHTTPCONNECTOR_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>

#include "httpconnector.h"
#include "report.h"
#include "urlreport.h"
#include "filehasher.h"

namespace ReportMode {
	enum ReportModeEnum { NONE, FILE_MODE, URL_MODE };
}

class BaseHttpConnector : public HttpConnector
{
Q_OBJECT
private:
	QNetworkAccessManager* manager;
	QString fileName;
	ReportMode::ReportModeEnum reportMode;
	bool abortRequested;
	QNetworkReply* reply;
	FileHasher* hasher;

protected:
	static void loadSettings();
	bool setupMultipartRequest( QNetworkRequest& request, QByteArray& multipartform, const QString& fileName, const QMap< QString, QString > params = QMap<QString, QString>() );
	void abortCurrentTask();
	bool freeNetworkReply(); //Should be called as soon as the reply is no longer needed
	QNetworkReply* createNetworkReply( const QNetworkRequest& request, const QByteArray& multipart = QByteArray(), bool usePostMethod = false );
	QNetworkReply* getNetworkReply();
	QString getFileName();
	void setFileName( const QString& fileName );
	FileHasher* getFileHasher();
	ReportMode::ReportModeEnum getReportMode();
	void setReportMode( ReportMode::ReportModeEnum reportMode );
	bool isAbortRequested();

protected slots:
	/** Deals with the reply of a service workload request */
	void onServiceWorkloadComplete();
	
	/** Deals with error that might happen during a file or URL submission */
	void onSubmissionReplyError( QNetworkReply::NetworkError );

	/** Indicates the amount of sent data out of the total */
	void onUploadProgressRate( qint64 bytesSent, qint64 bytesTotal );

	/** Indicates the amount of dowloaded data out of the total */
	void onDownloadProgressRate( qint64 bytesSent, qint64 bytesTotal );

public:
	/** Creates a HTTP connector object that will use the given network access manager and access key */
	BaseHttpConnector( QNetworkAccessManager*const manager );

	/** Destroys and frees all resources bound to this object */
	virtual ~BaseHttpConnector();

	/** Tries to re-use an existant report, if possible. Otherwise, it calls uploadFile() method */
	void submitFile( const QString& fileName, const bool reuseLastReport )=0;

	/** Submits the given file */
	void uploadFile(const QString& fileName)=0;

	/** Retrieves the file report object corresponding to the given scan Id. */
	void retrieveFileReport( const QString& scanId )=0;

	/** Submits the given URL */
 	void submitUrl( const QUrl& url2Scan, const bool reuseLastReport )=0;

	/** Retrieves the URL report object corresponding to the given scan Id. */
	void retrieveUrlReport( const QString& scanId )=0;

	/** Request the current taks to be aborted */
	void abort();

public slots:
	/** Returns the service workload */
	void onRetrieveServiceWorkload();
	
//TODO: Implement this methods
/*	void makeComment( QFile file );
	void makeComment( QUrl url ); */

signals:
	void uploadingProgressRate( qint64 bytesSent, qint64 bytesTotal );
	void errorOccurred( const QString& message );
	void scanIdReady( const QString& scanId );
	void serviceWorkloadReady( ServiceWorkload workload );
	void retrievingReport();
	void reportNotReady();
	void serviceLimitReached();
	void aborted();
	void invalidServiceKeyError();
	void reportReady( Report*const report );
};

#endif // BASEHTTPCONNECTOR_H