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

#include <KLocalizedString>
#include <KMessageBox>
#include <KDebug>

#include "taskviewhandler.h"
#include "taskrowviewhandler.h"
#include "mainwindow.h"
#include "taskscheduler.h"
#include "httpconnectorfactory.h"
#include <settings.h>

TaskRowViewHandler::TaskRowViewHandler( TaskViewHandler* viewHandler, int rowIndex ) {
	this->viewHandler = viewHandler;
	this->rowIndex = rowIndex;
	this->downloader = NULL;
	this->report = NULL;
	this->remoteTmpFile = NULL;
	this->seconds = 0;
	this->transmissionSeconds = 0;
	this->finished = true;
	this->jobId = -1;
}

TaskRowViewHandler::~TaskRowViewHandler() {
	// Free the report, remote file and dowloader, as needed
	setReport( NULL );
	if( remoteTmpFile != NULL ) {
		remoteTmpFile->deleteLater();
		remoteTmpFile = NULL;
	}
	if( downloader != NULL ) {
		downloader->deleteLater();
		downloader = NULL;
	}
}

void TaskRowViewHandler::setupObject( const QString& type, const QString& name, int size ) {
	// Show the item in the table view
	setType( type );
	setName( name );
	setSize( size );
//	setStatus( i18n( "Queued" ) ); // The proper message will be set when invoking a submit*() method
	setTime( this->seconds );
}

void TaskRowViewHandler::submitFile( const QFile& file ) {
	// Enque the file and setup the table row fields
	this->finished = false;
	this->jobId = TaskScheduler::self()->enqueueJob( file.fileName(), JobType::FILE, this, Settings::reuseLastReport() );
	this->setupObject( i18nc( "Type of task", "File" ), file.fileName(), file.size() );
}

void TaskRowViewHandler::submitRemoteFile( QNetworkAccessManager*const networkManager, const QUrl& url ) {
	// Free th downloader and remote file, if exists any
	this->finished = false;
	if( downloader != NULL ) {
		downloader->deleteLater();
	}
	if( remoteTmpFile != NULL ) {
		remoteTmpFile->deleteLater();
		remoteTmpFile = NULL;
	}
	
	// Set the table row fields
	this->setupObject( i18nc( "Type of task", "Remote file" ), url.toString() );	
	setStatus( i18n( "Dowloading..." ) );

	// Create a new dowloader, connect it and start downloading
	this->downloader = new FileDownloader( networkManager );
	connect( downloader, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( onDownloadProgressRate( qint64, qint64 ) ) );
	connect( downloader, SIGNAL( downloadReady( QFile* ) ), this, SLOT( onDownloadReady( QFile* ) ) );
	connect( downloader, SIGNAL( maximunSizeExceeded( qint64, qint64 ) ), this, SLOT( onMaximunSizeExceeded( qint64, qint64 ) ) );
	connect( downloader, SIGNAL( errorOccurred( QString ) ), this, SLOT( onErrorOccurred( QString ) ) );
	connect( downloader, SIGNAL( aborted() ), this, SLOT( onAborted() ) );
	downloader->download( url, HttpConnectorFactory::getFileHttpConnectorCfg().maxServiceFileSize );
}

void TaskRowViewHandler::submitUrl( const QUrl& url ) {
	// Enque the file and setup the table row fields
	this->finished = false;
	this->jobId = TaskScheduler::self()->enqueueJob( url.toString(), JobType::URL, this, Settings::reuseLastReport() );
	this->setupObject( i18n( "URL" ), url.toString() );
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
// 	kDebug() << "Writting status " << status;
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

void TaskRowViewHandler::onNextSecond() {
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

Report* TaskRowViewHandler::getReport() const {
	return report;
}

void TaskRowViewHandler::setReport( Report*const report ) {
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
		if( downloader != NULL && downloader->abort() ) {
			return true;
		}
		return TaskScheduler::self()->abort( this->jobId );
	}
	return false;
}

bool TaskRowViewHandler::rescan() {
	bool actionDone = false;
	if( isFinished() ) {
		if( downloader != NULL && remoteTmpFile == NULL ) {
			this->finished = false;
			setStatus( i18n( "Dowloading..." ) );
			downloader->download( QUrl( getName() ), HttpConnectorFactory::getFileHttpConnectorCfg().maxServiceFileSize );
		}
		else {
			QString itemName = remoteTmpFile != NULL ? remoteTmpFile->fileName() : getName();
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

void TaskRowViewHandler::onQueued() {
	setStatus( i18n( "Queued" ) );
}

void TaskRowViewHandler::onScanningStarted() {
	setStatus( i18n( "Submitting..." ) );
}

void TaskRowViewHandler::onErrorOccurred( const QString& message ) {
	this->finished = true;
	this->jobId = TaskScheduler::INVALID_JOB_ID;
	setStatus( i18n( "Error" ), message );
	MainWindow::showErrorNotificaton( i18n( "The next error occurred while processing the task %1: %2 ", rowIndex, message ) );
	emit( unsubscribeNextSecond( this ) );
}

void TaskRowViewHandler::onDownloadProgressRate( qint64 bytesSent, qint64 bytesTotal ) {
	// The task will be finished when an error has occurred. In such a case, do nothing...
	if( finished ) {
		kWarning() << "The task is finished. Thus, the downloadProgressRate() event will be ignored!";
		return;
	}
	int seconds = this->seconds - this->transmissionSeconds; // Current time - start time
	double rate = 100.0 * bytesSent / bytesTotal;
	int speed = ( int )( 0.5 + bytesSent / ( ( seconds ? seconds : 1 ) << 10 ) ); // seconds << 10 <==> seconds * 2^10 <==> seconds * 1024
	QString text( i18n( "Downloading (%L1%/%2 KiB/s)..." ).arg( rate, 3, 'g', 3 ).arg( speed ) );
	setSize( bytesTotal );
	setStatus( text );
}

void TaskRowViewHandler::onUploadProgressRate( qint64 bytesSent, qint64 bytesTotal ) {
	// The task will be finished when an error has occurred. In such a case, do nothing...
	if( finished ) {
		kWarning() << "The task is finished. Thus, the uploadProgressRate() event will be ignored!";
		return;
	}
	int seconds = this->seconds - this->transmissionSeconds; // Current time - start time
	double rate = 100.0 * bytesSent / bytesTotal;
	int speed = ( int )( 0.5 + bytesSent / ( ( seconds ? seconds : 1 ) << 10 ) ); // seconds << 10 <==> seconds * 2^10 <==> seconds * 1024
	QString text( i18n( "Uploading (%L1%/%2 KiB/s)..." ).arg( rate, 3, 'g', 3 ).arg( speed ) );
	setStatus( text );
}

void TaskRowViewHandler::onRetrievingReport() {
	setStatus( i18n( "Retrieving report..." ) );
}

void TaskRowViewHandler::onWaitingForReport( int seconds ) {
	setStatus( seconds > 0 ? i18n( "Waiting for %1 seconds...", seconds ) : i18n( "Waiting..." ) );
}

void TaskRowViewHandler::onServiceLimitReached( int seconds ) {
	setStatus( i18n( "Service limit reached! (wait %1)", seconds ) );
}

void TaskRowViewHandler::onAborted() {
	this->finished = true;
	this->jobId = TaskScheduler::INVALID_JOB_ID;
	kDebug() << "Aborted!";
	setStatus( i18n( "Aborted" ) );
	emit( unsubscribeNextSecond( this ) );
}

void TaskRowViewHandler::onReportReady( Report*const report ) {
	setReport( report ); // Free the current report and set the new one
	this->finished = true;
	this->jobId = TaskScheduler::INVALID_JOB_ID;
	setStatus( i18n( "Finished (%1/%2)", report->getNumPositives(), report->getResultList().size() ) );
	
	// Show a notification only when the conditions are met
	const int notificationTime = Settings::self()->taskNotificationTime();
	bool showNotification = ( notificationTime > 0 && seconds >= notificationTime ) ||
							( report->isInfected() && Settings::self()->infectedTaskNotification() );
	if( showNotification ) {
		MainWindow::showCompleteTaskNotificaton( i18n( "Task %1 finished", rowIndex + 1 ), 
												 ReportViewHandler::getReportIconName( report->getNumPositives() ) );
	}
	emit( unsubscribeNextSecond( this ) );
	emit( reportCompleted( rowIndex ) );
}

void TaskRowViewHandler::onDownloadReady( QFile* file ) {
	setSize( file->size() );
	this->remoteTmpFile = file;
	this->transmissionSeconds = 0; // Reset the counter
	this->jobId = TaskScheduler::self()->enqueueJob( file->fileName(), JobType::FILE, this, Settings::reuseLastReport() );
}

void TaskRowViewHandler::onMaximunSizeExceeded( qint64 sizeAllowed, qint64 sizeTotal ) {
	setSize( sizeTotal );
	const qint64 maxSize = HttpConnectorFactory::getFileHttpConnectorCfg().maxServiceFileSize;
	viewHandler->showFileTooBigMsg( i18n( "The remote file is too big. The service does not accept files greater than %1 MiB (%2 MB).",
									maxSize / ( 1024.0 * 1024 ), maxSize/ ( 1000 * 1000 ) ) );
	this->finished = true;
	emit( unsubscribeNextSecond( this ) );
	emit( maximunSizeExceeded( rowIndex ) );
}

#include "taskrowviewhandler.moc"