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

#include "taskschedulerjob.h"
#include <httpconnectorfactory.h>

uint TaskSchedulerJob::nextJobId = 1;

TaskSchedulerJob::TaskSchedulerJob( const QString& resourceName, JobType::JobTypeEnum type,
									QNetworkAccessManager*const manager, HttpConnectorListener*const listener,
								    const bool reuseLastReport ) {
	this->id = nextJobId++;
 	this->connector = HttpConnectorFactory::getHttpConnector( manager );
	this->listener = listener;
	this->type = type;
	this->resourceName = resourceName;
	this->reuseLastReport = reuseLastReport;
	this->scanId = QString::null;
	this->retrySubmitionDelay = 0;
	this->submitting = false;
	this->waitingRetransmittion = false;
	this->aborting = false;
	
	// Establish all connections between the connector and job
	connect( connector, SIGNAL( scanIdReady( QString ) ), this, SLOT( onScanIdReady( QString ) ) );
	connect( connector, SIGNAL( reportNotReady() ), this, SLOT( onReportNotReady() ) );
	connect( connector, SIGNAL( reportReady( Report*const ) ), this, SLOT( onReportReady() ) );
	connect( connector, SIGNAL( serviceLimitReached() ), this, SLOT( onServiceLimitReached() ) );
	connect( connector, SIGNAL( invalidServiceKeyError()), this, SLOT( onInvalidServiceKeyError() ) );
	connect( connector, SIGNAL( aborted() ), this, SLOT( onAbort() ) );
	connect( connector, SIGNAL( errorOccured( QString ) ), this, SLOT( onErrorOccured( QString ) ) );

	// Set up all connections between this job and the listener
	connect( this, SIGNAL( queued() ), listener, SLOT( onQueued() ) );
	connect( this, SIGNAL( startScanning() ), listener, SLOT( onScanningStarted() ) );
	connect( this, SIGNAL( waitingForReport( int ) ), listener, SLOT( onWaitingForReport( int ) ) );
	connect( this, SIGNAL( serviceLimitReached( int ) ), listener, SLOT( onServiceLimitReached( int ) ) );
	
	// Set up all connections between the connector and the listener
	connect( connector, SIGNAL( uploadingProgressRate( qint64, qint64 ) ), 
			 listener, SLOT( onUploadProgressRate( qint64, qint64 ) ) );
	connect( connector, SIGNAL( retrievingReport() ), listener, SLOT( onRetrievingReport() ) );
	connect( connector, SIGNAL( reportReady( Report*const ) ), listener, SLOT( onReportReady( Report*const) ) );
	connect( connector, SIGNAL( aborted() ), listener, SLOT( onAborted() ) );
	connect( connector, SIGNAL( errorOccured( QString ) ), listener, SLOT( onErrorOccured( QString ) ) );
}

TaskSchedulerJob::~TaskSchedulerJob() {
	this->connector->deleteLater();
}

void TaskSchedulerJob::submitJob() {
	this->submitting = true;
	emit( startScanning() );
	switch( type ) {
	case JobType::FILE:
		connector->submitFile( resourceName, reuseLastReport );
		break;
	case JobType::URL:
		connector->submitUrl( QUrl( resourceName ) );
		break;
	}
	// Update the retransmition flag
	this->waitingRetransmittion = false;
}

void TaskSchedulerJob::submit() {
	if( submitting ) {
		kWarning() << "This job(" << id << ") is now already being submitted. Thus, nothing will be done!";
	}
	else if( !waitingRetransmittion ) {
		submitJob();
	}
	else {
		kWarning() << "There is a programmed submit() retransmittion. Thus, nothing will be done!";
	}
}

void TaskSchedulerJob::retrySubmittionEvery( int seconds ) {
	this->retrySubmitionDelay = ( seconds > 0 ? seconds : 0 );
}

HttpConnectorCfg TaskSchedulerJob::getJobHttpConnectorCfg() {
	switch( jobType() ) {
		case JobType::URL:
			return connector->getUrlHttpConnectorCfg();
		default:
			kWarning() << "Unknown job type. Assuming JobType::FILE...";
		case JobType::FILE:
			return connector->getFileHttpConnectorCfg();
	}
}

void TaskSchedulerJob::checkReportReady( int seconds, bool limitReached ) {
	if( seconds > 0 ) {
		// Signal the right signal depending on the limitReaced flag
		if( !limitReached ) {
			emit( waitingForReport( seconds ) ); // Signal to the listener
		}
		else {
			emit( serviceLimitReached( seconds ) ); // Signal to the listener
		}
		kDebug() << "Setting" << seconds << "seconds singleShot for job" << id;
		QTimer::singleShot( seconds * 1000, this, SLOT( onCheckReportReady() ) );
	}
	else {
		emit( waitingForReport( seconds ) ); // Signal to the listener
	}
}

void TaskSchedulerJob::onCheckReportReady() {
	kDebug() << "Receiving singleShot for job" << id;
	if( scanId.isEmpty() ) {
		kError() << "The job id=" << id << " does not have scanId! NOTHING WILL BE DONE!!";
		return;
	}
	switch( type ) {
	case JobType::FILE:
		connector->retrieveFileReport( scanId );
		break;
	case JobType::URL:
		connector->retrieveUrlReport( scanId );
		break;
	}
}

void TaskSchedulerJob::abort() {
	this->aborting = true;
	connector->abort();
}

void TaskSchedulerJob::onScanIdReady( const QString& scanId ) {
	this->scanId = scanId;
	this->submitting = false;
	emit( scanIdReady( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onReportNotReady() {
	emit( reportNotReady( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onServiceLimitReached() {
	this->submitting = false;
	if( retrySubmitionDelay > 0 ) {
		if( !this->isRunning() ) {
			kDebug() << "Setting retry submittion in" << retrySubmitionDelay << "seconds...";
			this->waitingRetransmittion = true;
			QTimer::singleShot( retrySubmitionDelay * 1000, this, SLOT( submitJob() ) );
			emit( serviceLimitReached( retrySubmitionDelay ) ); // Signal to the listener
		}
		else {
			emit( serviceLimitReached( this ) ); // Signal to the scheduler
		}
	}
	else {
		emit( serviceLimitReached( this ) ); // Signal to the scheduler
		if( !this->isRunning() ) {
			emit( queued() ); // Signal to the listener
		}
	}
}

void TaskSchedulerJob::onReportReady() {
	emit( finished( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onInvalidServiceKeyError() {
	emit( finished( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onAbort() {
	this->aborting = false;
	emit( finished( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onErrorOccured( const QString& message ) {
	Q_UNUSED( message );
	this->submitting = false;
	emit( finished( this ) ); // Signal to the scheduler
}