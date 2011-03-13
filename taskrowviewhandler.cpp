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

#include <QTableWidget>
#include <KLocalizedString>
#include <KDebug>

#include "taskviewhandler.h"
#include "taskrowviewhandler.h"
#include <KAction>
#include <qcoreevent.h>
#include "mainwindow.h"
#include "taskscheduler.h"
#include <settings.h>

//TODO: Better use QIODevice than QFile
TaskRowViewHandler::TaskRowViewHandler( TaskViewHandler* viewHandler, int rowIndex, const QFile& file ) {
	setupObject( viewHandler, rowIndex, i18nc( "Type of task", "File" ), file.fileName(), file.size(), JobType::FILE );
}

TaskRowViewHandler::TaskRowViewHandler( TaskViewHandler* viewHandler, int rowIndex, const QUrl& url ) {
	setupObject( viewHandler, rowIndex, i18n( "URL" ), url.toString(), 0, JobType::URL );
}

TaskRowViewHandler::~TaskRowViewHandler() {
	setReport( NULL ); // Free the report
}

void TaskRowViewHandler::setupObject( TaskViewHandler* viewHandler, int rowIndex, const QString& type, const QString& name, int size, JobType::JobTypeEnum jobType ) {
	// Assing inner objects
	this->viewHandler = viewHandler;
	this->setRowIndex( rowIndex );
	this->report = NULL;
	this->finished = false;
	this->jobId = TaskScheduler::self()->enqueueJob( name, jobType, this, Settings::reuseLastReport() );
	this->startUploadSeconds = 0;

	// Show the item in the table view
	setType( type );
	setName( name );
	setSize( size );
//	setStatus( i18n( "Queued" ) ); // The "Submitting" message is already shown due to the enqueueJob() call above
	setTime( this->seconds = 0 );
}

void TaskRowViewHandler::setRowIndex( int index ){
		kDebug() << "Using table index" << index;
		this->rowIndex = index;
}

void TaskRowViewHandler::setType( const QString& type ){
	addRowItem( Column::TYPE, type );
}

// const QString& will fail
QString TaskRowViewHandler::getName() const {
	return getItemText( Column::NAME );
}

void TaskRowViewHandler::setName( const QString& name ) {
	addRowItem( Column::NAME, name, name );
}

void TaskRowViewHandler::setSize( qint64 size ) {
	QString text( i18n( "N/A" ) );
	if( size > 0 ) {
		if( size < 1024 ) {
			text = QString( "%L1 B").arg( size );
		}
		else {
			double tmp = size / 1024.0;
			if( tmp < 1024 ) {
				text = "%L1 KiB";
			}
			else {
				tmp = size / ( 1024 * 1024.0 );
				text = "%L1 MiB";
			}
			text = text.arg( tmp, 0, 'g', 3 );
		}
	}
	addRowItem( Column::SIZE, text );
}

void TaskRowViewHandler::setStatus( const QString& status, const QString& toolTip ) {
kDebug() << "Writting status " << status;
	addRowItem( Column::STATUS, status, toolTip );
}

void TaskRowViewHandler::setTime( int seconds ) {
	int hour = seconds / 3600 ;
	seconds -= hour * 3600;

	int minute = seconds / 60;
	seconds -= minute * 60;

	static const QLatin1Char zeroPadding( '0' );
	QString text = QString( "%1:%2:%3" ).arg( hour, 2, 10, zeroPadding ).
										 arg( minute, 2, 10, zeroPadding ).
										 arg( seconds, 2, 10, zeroPadding );
	addRowItem( Column::TIME, text );
}

void TaskRowViewHandler::nextSecond() {
	setTime( ++seconds );
}

QString TaskRowViewHandler::getItemText( Column::ColumnEnum column ) const {
	QTableWidget*const table = viewHandler->getTableWidget();
	QTableWidgetItem* item = table->item( rowIndex, column );
	if( item != NULL ) {
		return item->text();
	}
	kDebug() << "The item is null. Returning QString::null...";
	return QString::null;
}

void TaskRowViewHandler::addRowItem( int column, const QString& text, const QString& toolTip ) {
	QTableWidget*const table = viewHandler->getTableWidget();
	QTableWidgetItem* item = new QTableWidgetItem( text );
	switch( column ) {
		case Column::TYPE:
		case Column::STATUS:
		case Column::TIME:
			item->setTextAlignment( Qt::AlignCenter );
			break;
		case Column::SIZE:
			item->setTextAlignment( Qt::AlignRight );
			break;
	}
	item->setToolTip( toolTip );
	table->setItem( rowIndex, column, item );
}

AbstractReport* TaskRowViewHandler::getReport() const {
	return report;
}

void TaskRowViewHandler::setReport( AbstractReport*const report ) {
	if( this->report != NULL ) {
		delete this->report;
	}
	this->report = report;
}

int TaskRowViewHandler::getHintColumnSize(int column, int availableWidth ) {
	switch( column ) {
		case Column::NAME:
			return availableWidth * 0.5;
		case Column::STATUS:
			return availableWidth * 0.2;
		case Column::SIZE:
			return availableWidth * 0.08;
		case Column::TIME:
			return availableWidth * 0.12;
		default:
			return availableWidth * 0.1;
	}
}

bool TaskRowViewHandler::abort() {
	if( !isFinished() ) {
		setStatus( i18n( "Aborting..." ) );
		return TaskScheduler::self()->abort( this->jobId );
	}
	return false;
}

bool TaskRowViewHandler::rescan() {
	bool actionDone = false;
	if( isFinished() ) {
		QString itemName = getName();
		QFile file( itemName );
		if( file.exists() ) {
			this->finished = false;
			this->seconds = 0;
			this->jobId = TaskScheduler::self()->enqueueJob( itemName, JobType::FILE, this, false );
			actionDone = true;
		}
		else {
			QUrl url( itemName );
			if( url.isValid() ) {
				this->finished = false;
				this->seconds = 0;
				this->jobId = TaskScheduler::self()->enqueueJob( itemName, JobType::URL, this, true );
				actionDone = true;
			}
		}
		
		// Reset the current report
		if( report != NULL ) {
			kDebug() << "Resetting the current report...";
			this->setReport( NULL );
			emit( reportCompleted( rowIndex ) );
		}
	}
	return actionDone;
}

void TaskRowViewHandler::queued() {
	setStatus( i18n( "Queued" ) );
}

void TaskRowViewHandler::scanningStarted() {
	this-> startUploadSeconds = seconds;
	setStatus( i18n( "Submitting..." ) );
}

void TaskRowViewHandler::errorOccurred( const QString& message ) {
	this->finished = true;
	this->jobId = TaskScheduler::INVALID_JOB_ID;
	setStatus( i18n( "Error" ), message );
	MainWindow::showErrorNotificaton( i18n( "The next error ocurred while processing the task %1: %2 ", rowIndex, message ) );
	emit( unsubscribeNextSecond( this ) );
}

void TaskRowViewHandler::uploadProgressRate( qint64 bytesSent, qint64 bytesTotal ) {
	// The task will be finished when an error has ocurred. In such a case, do nothing...
	if( finished ) {
		kWarning() << "The task is finished. Thus, the uploadProgressRate() event will be ignored!";
		return;
	}
	int seconds = this->seconds - this->startUploadSeconds; // Current time - start time
	double rate = 100.0 * bytesSent / bytesTotal;
	int speed = ( int )( 0.5 + bytesSent / ( ( seconds ? seconds : 1 ) << 10 ) ); // seconds << 10 <==> seconds * 2^10 <==> seconds * 1024
	QString text( i18n( "Uploading (%L1%/%2 KiB/s)..." ).arg( rate, 3, 'g', 3 ).arg( speed ) );
	setStatus( text );
}

void TaskRowViewHandler::retrievingReport() {
	setStatus( i18n( "Retrieving report..." ) );
}

void TaskRowViewHandler::waitingForReport( int seconds ) {
	setStatus( seconds > 0 ? i18n( "Waiting for %1 seconds...", seconds ) : i18n( "Waiting..." ) );
}

void TaskRowViewHandler::serviceLimitReached( int seconds ) {
	setStatus( i18n( "Service limit reached! (wait %1)", seconds ) );
}

void TaskRowViewHandler::aborted() {
	this->finished = true;
	this->jobId = TaskScheduler::INVALID_JOB_ID;
	kDebug() << "Aborted!";
	setStatus( i18n( "Aborted" ) );
	emit( unsubscribeNextSecond( this ) );
}

void TaskRowViewHandler::reportReady( AbstractReport*const report ) {
	setReport( report ); // Free the current report and set the new one
	this->finished = true;
	this->jobId = TaskScheduler::INVALID_JOB_ID;
	setStatus( i18n( "Finished (%1/%2)", report->getResultMatrixPositives(), report->getResultMatrix().size() ) );
	
	// Show a notification only when the conditions are met
	const int notificationTime = Settings::self()->taskNotificationTime();
	bool showNotification = ( notificationTime > 0 && seconds >= notificationTime ) ||
							( report->isInfected() && Settings::self()->infectedTaskNotification() );
	if( showNotification ) {
		MainWindow::showCompleteTaskNotificaton( i18n( "Task %1 finished", rowIndex + 1 ), 
												 ReportViewHandler::getReportIconName( report->getResultMatrixPositives() ) );
	}
	emit( unsubscribeNextSecond( this ) );
	emit( reportCompleted( rowIndex ) );
}

#include "taskrowviewhandler.moc"