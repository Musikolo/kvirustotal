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

class AbstractReport;

/** HttpConnector interface for listener objects */
class HttpConnectorListener : public QObject
{
public:
	virtual ~HttpConnectorListener(){ }
	virtual void queued()=0;
	virtual void scanningStarted()=0;
	virtual void errorOccurred( const QString& message )=0;
	virtual void uploadProgressRate( qint64 bytesSent, qint64 bytesTotal )=0;
	virtual void retrievingReport()=0;
	virtual void waitingForReport( int seconds )=0;
	virtual void serviceLimitReached( int seconds )=0;
	virtual void aborted()=0;
	virtual void reportReady( AbstractReport* const report )=0;
};

#endif // HTTPCONNECTORLISTENER_H