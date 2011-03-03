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

//TODO: Better use QIODevice than QFile
TaskRowViewHandler::TaskRowViewHandler( TaskViewHandler* viewHandler, int rowIndex, HttpConnector* connector, const QFile& file ) {
	setupObject( viewHandler, rowIndex, connector, i18nc( "Type of task", "File" ), file.fileName(), file.size() );
	connector->submitFile( file.fileName() );
}

TaskRowViewHandler::TaskRowViewHandler( TaskViewHandler* viewHandler, int rowIndex, HttpConnector* connector, const QUrl& url ) {
	setupObject( viewHandler, rowIndex, connector, i18n( "URL" ), url.toString(), 0 );
	connector->submitUrl( url );
}

TaskRowViewHandler::~TaskRowViewHandler() {
	connector->deleteLater();
	setReport( NULL ); // Free the report
}

void TaskRowViewHandler::setupObject( TaskViewHandler* viewHandler, int rowIndex, HttpConnector* connector, const QString& type, const QString& name, int size ) {
	// Assing inner objects
	this->viewHandler = viewHandler;
	this->connector = connector;
	this->setRowIndex( rowIndex );
	this->report = NULL;
	this->finished = false;

	// Show the item in the table view
	setType( type );
	setName( name );
	setSize( size );
	setStatus( QString() );
	setTime( this->seconds = 0 );

	// Establish all connections
	connect( connector, SIGNAL( startScanning() ), this, SLOT( scanningStarted() ) );
	connect( connector, SIGNAL( uploadingProgressRate( qint64, qint64 ) ), this, SLOT( uploadProgressRate( qint64, qint64 ) ) );
	connect( connector, SIGNAL( scanIdReady() ), this, SLOT( scanIdReady() ) );
	connect( connector, SIGNAL( retrievingReport() ), this, SLOT( retrievingReport() ) );
	connect( connector, SIGNAL( waitingForReport( int ) ), this, SLOT( waitingForReport( int ) ) );
	connect( connector, SIGNAL( serviceLimitReached( int ) ), this, SLOT( serviceLimitReached( int ) ) );
	connect( connector, SIGNAL( fileReportReady( FileReport* const ) ), this, SLOT( fileReportReady( FileReport* const ) ) );
	connect( connector, SIGNAL( urlReportReady( UrlReport* const ) ), this, SLOT( urlReportReady( UrlReport* const) ) );
	connect( connector, SIGNAL( aborted() ), this, SLOT( aborted() ) );
	connect( connector, SIGNAL( errorOccurred( QString ) ), this, SLOT( errorOccurred( QString ) ) );
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

	QLatin1Char zeroPadding = QLatin1Char( '0' );
	QString text;
	text = QString( "%1:%2:%3" ).arg( hour, 2, 10, zeroPadding ).
								 arg( minute, 2, 10, zeroPadding ).
								 arg( seconds, 2, 10, zeroPadding );
	addRowItem( Column::TIME, text );
}

void TaskRowViewHandler::nextSecond() {
	setTime( ++seconds );
}

const QString& TaskRowViewHandler::getItemText( Column::ColumnEnum column ) const {
	QTableWidget* table = viewHandler->getTableWidget();
	QTableWidgetItem* item = table->item( rowIndex, column );
	if( item != NULL ) {
		return item->text();
	}
	kDebug() << "The item is null. Returning QString::null...";
	return QString::null;
}

void TaskRowViewHandler::addRowItem( int column, const QString& text, const QString& toolTip ) {
	QTableWidget* table = viewHandler->getTableWidget();
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
		connector->abort();
		return true;
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
			connector->uploadFile( itemName );
			actionDone = true;
		}
		else {
			QUrl url( itemName );
			if( url.isValid() ) {
				this->finished = false;
				this->seconds = 0;
				connector->submitUrl( url );
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

void TaskRowViewHandler::scanningStarted() {
	setStatus( i18n( "Submitting..." ) );
}

void TaskRowViewHandler::errorOccurred( const QString& message ) {
	this->finished = true;
	setStatus( i18n( "Error" ), message );
	MainWindow::showErrorNotificaton( i18n( "The next error ocurred while processing the task %1: %2 ", rowIndex, message ) );
	emit( unsubscribeNextSecond( this ) );
}

void TaskRowViewHandler::uploadProgressRate( qint64 bytesSent, qint64 bytesTotal ) {
	double rate = 100.0 * bytesSent / bytesTotal;
	int speed = ( int )( 0.5 + bytesSent / ( ( seconds ? seconds : 1 ) << 10 ) ); // seconds << 10 <==> seconds * 2^10 <==> seconds * 1024
	QString text( i18n( "Uploading (%L1%/%2 KiB/s)..." ).arg( rate, 3, 'g', 3 ).arg( speed ) );
	setStatus( text );
}

void TaskRowViewHandler::scanIdReady() {
	setStatus( i18n( "Queued" ) );
}

void TaskRowViewHandler::retrievingReport() {
	setStatus( i18n( "Retrieving report..." ) );
}

void TaskRowViewHandler::waitingForReport( int seconds ) {
	setStatus( i18n( "Waiting for %1 seconds...", seconds ) );
}

void TaskRowViewHandler::serviceLimitReached( int seconds ) {
	if( seconds > 0 ) {
		setStatus( i18n( "Service limit reached! (wait %1)", seconds ) );
	}
	else {
		setStatus( i18n( "Service limit reached! Please, try later." ) );
		emit( unsubscribeNextSecond( this ) );
		emit( reportCompleted( rowIndex ) );
		this->finished = true;
	}
}

void TaskRowViewHandler::aborted() {
	this->finished = true;
	kDebug() << "Aborted!";
	setStatus( i18n( "Aborted" ) );
	emit( unsubscribeNextSecond( this ) );
}

void TaskRowViewHandler::fileReportReady( FileReport* const report ) {
	reportReady( report );
}

void TaskRowViewHandler::urlReportReady( UrlReport * const report ) {
	reportReady( report );
}

void TaskRowViewHandler::reportReady( AbstractReport*const report ) {
	setReport( report ); // Free the current report and set the new one
	this->finished = true;
	setStatus( i18n( "Finished (%1/%2)", report->getResultMatrixPositives(), report->getResultMatrix().size() ) );
	// Only when the report takes more a a minute, will show a notification
	if( seconds > 60 ) {
		MainWindow::showCompleteTaskNotificaton( i18n( "Task %1 finished", rowIndex + 1 ), 
												 ReportViewHandler::getReportIconName( report->getResultMatrixPositives() ) );
	}
	emit( unsubscribeNextSecond( this ) );
	emit( reportCompleted( rowIndex ) );
}

#include "taskrowviewhandler.moc";