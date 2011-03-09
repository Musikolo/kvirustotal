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

#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <QString>
#include <QUrl>
#include <QQueue>
#include <QNetworkAccessManager>

#include "httpconnectorlistener.h"
#include "taskschedulerjob.h"

/** Delays used to wait for the service to have reports ready */
namespace ServiceRequestDelay {
	enum ServiceRequestDelayEnum {
		DELAY_LEVEL_1 = 15,
		DELAY_LEVEL_2 = 30,
		DELAY_LEVEL_3 = 45,
		DELAY_LEVEL_4 = 60,
		DELAY_LEVEL_5 = 90,
		DELAY_LEVEL_6 = 150,
		DELAY_LEVEL_7 = 300
	};
}

typedef TaskSchedulerJob Job;

class TaskScheduler : public QObject
{
Q_OBJECT
private:
	/** Singleton instance */
	static TaskScheduler* const INSTANCE;
	
	QNetworkAccessManager* manager;
	QQueue< Job* > fileQueue;
	QQueue< Job* > urlQueue;
	int fileDelayLevel;
	int urlDelayLevel;
	int nextJobId;
	bool serviceLimit;
	
	TaskScheduler();

	Job* const findJob( const uint jobId );
	Job* const activeJob( JobType::JobTypeEnum type );
	QQueue< Job* >*const queueByType( JobType::JobTypeEnum type );
	void processNewJob( Job*const job );
	void nextActiveJob( JobType::JobTypeEnum type );
	void processUnsubmittedJobs();
	ServiceRequestDelay::ServiceRequestDelayEnum getSuitableDelay( Job*const job, bool serviceLimitReached = false );
	ServiceRequestDelay::ServiceRequestDelayEnum firstSuitableDelay( Job*const job );
	void initDelayLevels( bool showKDebug = true );

private slots:
	void scanIdReady( TaskSchedulerJob*const job );
	void reportNotReady( TaskSchedulerJob*const job );
	void serviceLimitReached( TaskSchedulerJob*const job );
	
	/** Emitted whenever a job's final state is reached: report ready, error, abortion, ...etc.*/
	void finished( TaskSchedulerJob*const job );
public:
	/** Job id that should be used for initializations or reset operations */
	static const uint INVALID_JOB_ID = 0;
	
	/** Gets the singleton instance */
	static TaskScheduler* self(){ return TaskScheduler::INSTANCE; }
	~TaskScheduler();
	
	/** Enqueue the given resource and returns the jobId. 0 will be return in case of failure. */
	uint enqueueJob( const QString& resourceName, JobType::JobTypeEnum type, HttpConnectorListener* const listener, const bool reuseLastReport = true );
	bool abort( const uint jobId );
};

#endif // TASKSCHEDULER_H