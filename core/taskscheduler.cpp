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

#include <QTimer>
#include <KDebug>

#include "taskscheduler.h"
#include <httpconnectorfactory.h>

TaskScheduler*const TaskScheduler::INSTANCE = new TaskScheduler();

TaskScheduler::TaskScheduler() {
	manager = new QNetworkAccessManager( this );
	nextJobId = 1;
	serviceLimit = false;
	initDelayLevels( false );// It must be false, otherwise, kDebug breaks!!
}

TaskScheduler::~TaskScheduler() {
	manager->deleteLater();
}

uint TaskScheduler::enqueueJob( const QString& resourceName, JobType::JobTypeEnum type,
								HttpConnectorListener*const listener, const bool reuseLastReport ) {
	// Create the job and set up all connections
	Job*const job = new Job( resourceName, type, manager, listener, reuseLastReport );
	connect( job, SIGNAL( scanIdReady( TaskSchedulerJob*const ) ), 
			 this,  SLOT( scanIdReady( TaskSchedulerJob*const ) ) );
	connect( job, SIGNAL( reportNotReady( TaskSchedulerJob*const ) ),
			 this,  SLOT( reportNotReady( TaskSchedulerJob*const ) ) );
	connect( job, SIGNAL( serviceLimitReached( TaskSchedulerJob*const ) ),
			 this,  SLOT( serviceLimitReached( TaskSchedulerJob*const ) ) );
	connect( job, SIGNAL( finished( TaskSchedulerJob*const ) ), 
			 this,  SLOT( finished( TaskSchedulerJob*const ) ) );
	
	// Reprocess the queues and return the jobId
	processNewJob( job );
	return job->jobId();
}
void TaskScheduler::processNewJob( Job*const job ) {
	QQueue<Job*>*const queue = queueByType( job->jobType() );
	if( queue->isEmpty() ) {
		const HttpConnectorCfg cfg = job->getJobHttpConnectorCfg(); 
		const uint retrySubmittionDelay = cfg.levels[ cfg.defaultLevel ];
		kDebug() << "Setting head job" << job->jobId() << "retry submition delay to" << retrySubmittionDelay << "seconds...";
		job->retrySubmittionEvery( retrySubmittionDelay );
	}
	queue->enqueue( job );
	job->submit();
}

void TaskScheduler::processUnsubmittedJobs() {
	static const JobType::JobTypeEnum types[] = { JobType::FILE, JobType::URL };
	static const int numTypes = sizeof( types ) / sizeof( types[ 0 ] );
	for( int i = 0; i < numTypes; i++ ) {
		const QQueue<Job*>*const queue = queueByType( types[ i ] );
		for( int j = 0; j < queue->size(); j++ ) {
			Job*const job = queue->at( j );
			if( !job->isRunning() && !serviceLimit ) {
				if( !job->isAborting() ) {
					kDebug() << "Submitting job id=" << job->jobId();
					job->submit();
				}
				else {
					kDebug() << "Job " << job->jobId() << "is being aborted. Doing nothing!";
				}
			}
		}
	}
}

void TaskScheduler::nextActiveJob( JobType::JobTypeEnum type ) {
	Job*const job = activeJob( type );
	if( job != NULL ){
		if( job->isRunning() ) {
			kDebug() << "Setting next active job id=" << job->jobId();
			reportNotReady( job );	
		}
		else {
			const HttpConnectorCfg cfg = job->getJobHttpConnectorCfg();
			const uint retrySubmittionDelay = cfg.levels[ cfg.defaultLevel ];
			kDebug() << "Setting next active job" << job->jobId()
					 << "retry submition delay to" << retrySubmittionDelay
					 << "seconds and submitting...";
			job->retrySubmittionEvery( retrySubmittionDelay );
			job->submit();
		}
	}
}

void TaskScheduler::scanIdReady( TaskSchedulerJob*const job ) {
	serviceLimit = false;
	Job*const aJob = activeJob( job->jobType() );
	if( !job->isAborting() ) {
		const int seconds = ( aJob == job ? firstSuitableDelay( job ) : -1 );
		job->checkReportReady( seconds );
	}
	else {
		kDebug() << "Job " << job->jobId() << "is being aborted. Doing nothing!";
	}
}

void TaskScheduler::reportNotReady( TaskSchedulerJob*const job ) {
	serviceLimit = false;
	if( !job->isAborting() ) {
		job->checkReportReady( getSuitableDelay( job ) );
	}
	else {
		kDebug() << "Job " << job->jobId() << "is being aborted. Doing nothing!";
	}
}

void TaskScheduler::serviceLimitReached( TaskSchedulerJob*const job ) {
	serviceLimit = true;
	if( job->isRunning() ){
		if( !job->isAborting() ) {
			job->checkReportReady( getSuitableDelay( job, true ), true );
		}
		else {
			kDebug() << "Job " << job->jobId() << "is being aborted. Doing nothing!";
		}
	}
}

void TaskScheduler::finished( TaskSchedulerJob*const job ) {
	QQueue< Job* >*const queue = queueByType( job->jobType() );
	if( queue->removeOne( job ) ) {
		processUnsubmittedJobs();
		nextActiveJob( job->jobType() );
		job->deleteLater();
		if( fileQueue.isEmpty() && urlQueue.isEmpty() ) {
			initDelayLevels();
		}
	}
	else {
		kError() << "The job id=" << job->jobId() << "was not dequeued!";
	}
}

bool TaskScheduler::abort( const uint jobId ) {
	Job*const job = findJob( jobId );
	if( job ) {
		job->abort();
		return true;
	}
	kWarning() << "No job with id=" << jobId << "found!";
	return false;
}

Job* TaskScheduler::findJob( const uint jobId ) {
	if( !fileQueue.isEmpty() ) {
		for( int i = 0; i < fileQueue.size(); i++ ) {
			Job*const job = fileQueue[ i ];
			if( job->jobId() == jobId ) {
				return job;
			}
		}
	}
	if( !urlQueue.isEmpty() ) {
		for( int i = 0; i < urlQueue.size(); i++ ) {
			Job*const job = urlQueue[ i ];
			if( job->jobId() == jobId ) {
				return job;
			}
		}
	}
	
	return NULL;
}

QQueue< Job* >* TaskScheduler::queueByType( JobType::JobTypeEnum type ) {
	return type == JobType::FILE ? &fileQueue : &urlQueue;
}

Job* TaskScheduler::activeJob( JobType::JobTypeEnum type ) {
	QQueue< Job* >*const queue = queueByType( type );
	kDebug() << "Queue size" << queue->size();
	if( !queue->isEmpty() ){
		return queue->head();
	}
	kDebug() << "No active job found of type " << type;
	return NULL;
}

void TaskScheduler::initDelayLevels( bool showKDebug ) {
	// showKDebug must be false when calling this method from the constructor. Otherwise, kDebug breaks!!
	if( showKDebug ) {
		kDebug() << "Initializing delay levels...";	
	}
	fileDelayLevel = HttpConnectorFactory::getFileHttpConnectorCfg().defaultLevel;
	urlDelayLevel  = HttpConnectorFactory::getUrlHttpConnectorCfg().defaultLevel;
}

int TaskScheduler::firstSuitableDelay( Job*const job ) {
	const HttpConnectorCfg cfg = job->getJobHttpConnectorCfg();
	return cfg.levels[ cfg.defaultLevel ];
}

int TaskScheduler::getSuitableDelay( Job*const job, bool serviceLimitReached ) {

	// Get the right configuration object and apply the suitable delay
	const HttpConnectorCfg cfg = job->getJobHttpConnectorCfg();
	switch( job->jobType() ) {
		default:
			kWarning() << "Type unknown" << job->jobType();
		case JobType::FILE:
			if( !serviceLimitReached ) {
				int minLevel = 0;
				if( cfg.numLevels > 1 ) {
					const bool singleQeuueRunning = queueByType( job->jobType() )->isEmpty();
					if( singleQeuueRunning ) {
						minLevel = 1;
					}
				} 
				fileDelayLevel = ( fileDelayLevel > minLevel ? fileDelayLevel - 1 : minLevel );
			}
			else if( fileDelayLevel < cfg.numLevels - 1 ) {
				fileDelayLevel++;
			}
			return cfg.levels[ fileDelayLevel ];
			
		case JobType::URL:
			if( !serviceLimitReached ) {
				int minLevel = 0;
				if( cfg.numLevels > 1 ) {
					const bool singleQeuueRunning = queueByType( job->jobType() )->isEmpty();
					if( singleQeuueRunning ) {
						minLevel = 1;
					}
				} 
				urlDelayLevel = ( urlDelayLevel > minLevel ? urlDelayLevel - 1 : minLevel );
			}
			else if( urlDelayLevel < cfg.numLevels - 1 ) {
				urlDelayLevel++;
			}
			return cfg.levels[ urlDelayLevel ];
	}
}