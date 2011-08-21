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

#ifndef APIHTTPCONNECTOR_H
#define APIHTTPCONNECTOR_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QUrl>

#include "base/basehttpconnector.h"
#include "report.h"
#include "urlreport.h"
#include "filehasher.h"

class ApiHttpConnector : public BaseHttpConnector
{
Q_OBJECT
private:
	QString key;
	QString scanId; // Only used to build permanent link for URL scans
	bool reusingLastReport;

    void retrieveLastReport();

private slots:
	/** Process the reply of a previous file submission and emits retrieveFileReport() signal */
	void onSubmissionReply();

	/** Deals with the reply of a previous file report request */
	void onReportComplete();

public:
	/** Does some initialization tasks common to all instances. Thus, it should be called before invoking the constructor. */
	static void loadSettings();
	
	/** Creates a HTTP connector object that will use the given network access manager and access key */
	ApiHttpConnector( QNetworkAccessManager*const manager, const QString& key );

	/** Destroys and frees all resources bound to this object */
	virtual ~ApiHttpConnector();

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
	
	HttpConnectorCfg getFileHttpConnectorCfg();
	HttpConnectorCfg getUrlHttpConnectorCfg();

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

#endif // APIHTTPCONNECTOR_H