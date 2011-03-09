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

#ifndef TASKSCHEDULERJOB_H
#define TASKSCHEDULERJOB_H

#include <QString>

#include "httpconnector.h"
#include "httpconnectorlistener.h"

namespace JobType {
	enum JobTypeEnum { FILE, URL };
};

class TaskSchedulerJob : public QObject
{
Q_OBJECT
private:
	static uint nextJobId;
	
	uint id;
	JobType::JobTypeEnum type;
	HttpConnector* connector;
	QString resourceName;
	bool reuseLastReport;
	QString scanId;
	HttpConnectorListener* listener;
	int retrySubmitionDelay;
	
private slots:
	void onCheckReportReady();
	void onScanIdReady( const QString& scanId );
//	void retrievingReport();
	void onReportNotReady();
	void onReportReady();
	void onServiceLimitReached();
	void onInvalidServiceKeyError();
	void onAbort();
	void onErrorOccurred( const QString& message );
	
public:
    TaskSchedulerJob( const QString& resourceName, JobType::JobTypeEnum type, 
					  QNetworkAccessManager* const manager, HttpConnectorListener* const listener,
					  const bool reuseLastReport );
    virtual ~TaskSchedulerJob();
	
	uint jobId() { return this->id; }
	JobType::JobTypeEnum jobType() { return this->type; }
	bool isRunning() { return !scanId.isEmpty(); }
	
	/** Retries submittions every given seconds when the service limit is reached */
	void retrySubmittionEvery( int seconds );
	
public slots:
	void submit();
	void checkReportReady( int seconds, bool limitReached = false );
	void abort();
	
signals:
	void queued();
	void startScanning();
	void scanIdReady( TaskSchedulerJob*const job );
	void waitingForReport( int );
	void reportNotReady( TaskSchedulerJob*const job );
	void serviceLimitReached( int );
	void serviceLimitReached( TaskSchedulerJob*const job );
	
	/** Emitted whenever a final state is reached: report ready, error, abortion, ...etc.*/
	void finished( TaskSchedulerJob* const job );
};

#endif // TASKSCHEDULERJOB_H