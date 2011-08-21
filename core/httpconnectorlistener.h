/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2011  <copyright holder> <email>

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

#ifndef HTTPCONNECTORLISTENER_H
#define HTTPCONNECTORLISTENER_H

#include <QObject>

class Report;

/** HttpConnector interface for listener objects */
class HttpConnectorListener : public QObject
{
public:
	virtual ~HttpConnectorListener(){ }
	virtual void onQueued()=0;
	virtual void onScanningStarted()=0;
	virtual void onErrorOccured( const QString& message )=0;
	virtual void onUploadProgressRate( qint64 bytesSent, qint64 bytesTotal )=0;
	virtual void onRetrievingReport()=0;
	virtual void onWaitingForReport( int seconds )=0;
	virtual void onServiceLimitReached( int seconds )=0;
	virtual void onAborted()=0;
	virtual void onReportReady( Report* const report )=0;
};

#endif // HTTPCONNECTORLISTENER_H