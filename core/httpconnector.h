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
#include <QUrl>

#include "httpconnectoremitter.h"

/** HttpConnector config as per service type */
struct HttpConnectorCfg {
	const uchar numLevels;
	const uchar defaultLevel;
	const int* levels;
	const bool serviceKeyRequired;
	const qint64 maxServiceFileSize;
};

/** HttpConnector interface */
class HttpConnector : public HttpConnectorEmitter
{
public:
	/** Destroys and frees all resources bound to this object */
 	virtual ~HttpConnector(){ }

	/** Tries to re-use an existant report, if possible. Otherwise, it calls uploadFile() method */
	virtual void submitFile( const QString& fileName, const bool reuseLastReport )=0;

	/** Submits the given file */
	virtual void uploadFile(const QString& fileName)=0;

	/** Retrieves the file report object corresponding to the given scan Id. */
	virtual void retrieveFileReport( const QString& scanId )=0;

	/** Submits the given URL */
 	virtual void submitUrl( const QUrl& url2Scan, const bool reuseLastReport )=0;

	/** Retrieves the URL report object corresponding to the given scan Id. */
	virtual void retrieveUrlReport( const QString& scanId )=0;

	/** Request the current taks to be aborted */
	virtual void abort()=0;

	/** Returns the service workload */
	virtual void onRetrieveServiceWorkload()=0;
	
	virtual HttpConnectorCfg getFileHttpConnectorCfg()=0;
	virtual HttpConnectorCfg getUrlHttpConnectorCfg()=0;
	
//TODO: Implement this methods some day
/*	virtual void makeComment( QFile file )=0;
	virtual void makeComment( QUrl url )=0; */
};

#endif // HTTPCONNECTOR_H