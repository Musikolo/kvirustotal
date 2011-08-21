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

#ifndef HTTPCONNECTOREMIITER_H
#define HTTPCONNECTOREMIITER_H

#include "report.h"

/** Service workload structure */
struct ServiceWorkload {
	static const uchar MIN_VALUE = 1;
	static const uchar MAX_VALUE = 7;
	uchar file;
	uchar url;
};

/** HttpConnector interface for emittig signals */
class HttpConnectorEmitter : public QObject
{
public:
	/** Destroys and frees all resources bound to this object */
	virtual ~HttpConnectorEmitter(){ }

	virtual void uploadingProgressRate( qint64 bytesSent, qint64 bytesTotal )=0;
	virtual void errorOccurred( const QString& message )=0;
	virtual void scanIdReady( const QString& scanId )=0;
	virtual void serviceWorkloadReady( ServiceWorkload workload )=0;
	virtual void retrievingReport()=0;
	virtual void reportNotReady()=0;
	virtual void serviceLimitReached()=0;
	virtual void aborted()=0;
	virtual void invalidServiceKeyError()=0;
	virtual void reportReady( Report*const report )=0;
};

#endif // HTTPCONNECTOREMIITER_H