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

#include <settings.h>
#include "taskschedulerjob.h"

uint TaskSchedulerJob::nextJobId = 1;

TaskSchedulerJob::TaskSchedulerJob( const QString& resourceName, JobType::JobTypeEnum type,
									QNetworkAccessManager*const manager, HttpConnectorListener*const listener,
								    const bool reuseLastReport ) {
	this->id = nextJobId++;
	this->connector = new HttpConnector( manager, Settings::self()->serviceKey() );
	this->listener = listener;
	this->type = type;
	this->resourceName = resourceName;
	this->reuseLastReport = reuseLastReport;
	this->scanId = QString::null;
	this->retrySubmitionDelay = 0;
	
	// Establish all connections between the connector and job
	connect( connector, SIGNAL( scanIdReady( QString ) ), this, SLOT( onScanIdReady( QString ) ) );
	connect( connector, SIGNAL( reportNotReady() ), this, SLOT( onReportNotReady() ) );
	connect( connector, SIGNAL( reportReady( AbstractReport*const ) ), this, SLOT( onReportReady() ) );
	connect( connector, SIGNAL( serviceLimitReached() ), this, SLOT( onServiceLimitReached() ) );
	connect( connector, SIGNAL( invalidServiceKeyError()), this, SLOT( onInvalidServiceKeyError() ) );
	connect( connector, SIGNAL( aborted() ), this, SLOT( onAbort() ) );
	connect( connector, SIGNAL( errorOccurred( QString ) ), this, SLOT( onErrorOccurred( QString ) ) );

	// Set up all connections between this job and the listener
	connect( this, SIGNAL( queued() ), listener, SLOT( queued() ) );
	connect( this, SIGNAL( startScanning() ), listener, SLOT( scanningStarted() ) );
	connect( this, SIGNAL( waitingForReport( int ) ), listener, SLOT( waitingForReport( int ) ) );
	connect( this, SIGNAL( serviceLimitReached( int ) ), listener, SLOT( serviceLimitReached( int ) ) );
	
	// Set up all connections between the connector and the listener
	connect( connector, SIGNAL( uploadingProgressRate( qint64, qint64 ) ), 
			 listener, SLOT( uploadProgressRate( qint64, qint64 ) ) );
	connect( connector, SIGNAL( retrievingReport() ), listener, SLOT( retrievingReport() ) );
	connect( connector, SIGNAL( reportReady( AbstractReport*const ) ), listener, SLOT( reportReady( AbstractReport*const) ) );
	connect( connector, SIGNAL( aborted() ), listener, SLOT( aborted() ) );
	connect( connector, SIGNAL( errorOccurred( QString ) ), listener, SLOT( errorOccurred( QString ) ) );
}

TaskSchedulerJob::~TaskSchedulerJob() {
	this->connector->deleteLater();
}

void TaskSchedulerJob::submit() {
	emit( startScanning() );
	switch( type ) {
	case JobType::FILE:
		connector->submitFile( resourceName, reuseLastReport );
		break;
	case JobType::URL:
		connector->submitUrl( QUrl( resourceName ) );
		break;
	}
}
void TaskSchedulerJob::retrySubmittionEvery( int seconds ) {
	this->retrySubmitionDelay = ( seconds > 0 ? seconds : 0 );
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
	connector->abort();
}

void TaskSchedulerJob::onScanIdReady( const QString& scanId ) {
	this->scanId = scanId;
//	emit( queued() ); // Signal to the listener
	emit( scanIdReady( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onReportNotReady() {
	emit( reportNotReady( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onServiceLimitReached() {
	if( retrySubmitionDelay > 0 ) {
		kDebug() << "Setting retry submittion in" << retrySubmitionDelay << "seconds...";
		QTimer::singleShot( retrySubmitionDelay * 1000, this, SLOT( submit() ) );
		emit( serviceLimitReached( retrySubmitionDelay ) ); // Signal to the listener
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
	emit( finished( this ) ); // Signal to the scheduler
}

void TaskSchedulerJob::onErrorOccurred( const QString& message ) {
	Q_UNUSED( message );
	emit( finished( this ) ); // Signal to the scheduler
}