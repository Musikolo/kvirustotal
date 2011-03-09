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

#include "filereport.h"
#include "urlreport.h"

namespace ReportMode {
	enum ReportModeEnum { NONE, FILE_MODE, URL_MODE };
}

class HttpConnector : public QObject, public virtual ServiceBasicReply
{
Q_OBJECT
private:
	QNetworkAccessManager* manager;
	QString key;
	QFile* file;
	QString fileName;
	ReportMode::ReportModeEnum reportMode;
	bool abortRequested;
	QNetworkReply* reply;
	FileHasher* hasher;
	bool reusingLastReport;

    void retrieveLastReport(const QString& fileName);
	bool setupMultipartRequest( QNetworkRequest& request, QByteArray& multipartform, const QString& fileName );
	void abortCurrentTask();

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

public:
	/** Does some initialization tasks common to all instances. Thus, it should be called before invoking the constructor. */
	static void loadSettings();
	
	/** Creates a HTTP connector object that will use the given network access manager and access key */
	HttpConnector( QNetworkAccessManager*const manager, const QString& key );

	/** Destroys and frees all resources bound to this object */
	~HttpConnector();

	/** Tries to re-use an existant report, if possible. Otherwise, it calls uploadFile() method */
	void submitFile( const QString& fileName, const bool reuseLastReport );

	/** Submits the given file */
	void uploadFile(const QString& fileName);

	/** Retrieves the file report object corresponding to the given scan Id. */
	void retrieveFileReport( const QString& scanId );

	/** Submits the given URL */
 	void submitUrl( const QUrl& url2Scan );

	/** Retrieves the URL report object corresponding to the given scan Id. */
	void retrieveUrlReport( const QString& scanId );

	/** Request the current taks to be aborted */
	void abort();

//TODO: Implement this methods
/*	void makeComment( QFile file );
	void makeComment( QUrl url ); */

signals:
	void uploadingProgressRate( qint64 bytesSent, qint64 bytesTotal );
	void errorOccurred( const QString& message );
	void scanIdReady( const QString& scanId );
	void retrievingReport();
	void reportNotReady();
	void serviceLimitReached();
	void aborted();
	void invalidServiceKeyError();
	void reportReady( AbstractReport*const report );
};

#endif // HTTPCONNECTOR_H