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
#include <KDebug>

#include "reportviewhandler.h"
#include <QHeaderView>
#include "mainwindow.h"
#include <settings.h>

ReportViewHandler::ReportViewHandler( MainWindow* mainwindow, QTableWidget* tableWidget ) {
	this->mainWindow = mainwindow;
	this->tableWidget = tableWidget;
	this->rowViewHandlers = NULL;
	setupObject( tableWidget );
}

ReportViewHandler::~ReportViewHandler() {
	freeRowHandlers();
}

void ReportViewHandler::setupObject( QTableWidget* tableWidget ){

	tableWidget->setRowCount( 0 );
	tableWidget->setColumnCount( 4 );
	tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
	tableWidget->resizeRowsToContents();
	tableWidget->setAlternatingRowColors( true );

	QStringList headers;
	tableWidget->setHorizontalHeaderLabels( headers << i18n( "Antivirus" ) << i18n( "Version" ) << i18n( "Last update" ) << i18n( "Result" ) );

	// Set the columns' width done by the user, if any
	QList< int > colsWidths = Settings::self()->reportTableCols();
	kDebug() << "User-defined column widths=" << colsWidths;
	if( !colsWidths.isEmpty() && colsWidths.size() == tableWidget->columnCount() ) {
		for( int i = 0; i < colsWidths.size(); i++ ) {
			tableWidget->setColumnWidth( i, colsWidths.at( i ) );
		}
	}
	// Otherwise, distribute the columns' width based on own approximation algorithm
	else {
		int width = ( tableWidget->topLevelWidget()->width() - 60 ) / tableWidget->columnCount();
		for( int i = 0; i < tableWidget->columnCount(); i++ ) {
			tableWidget->setColumnWidth( i, width );
		}
	}
}

void ReportViewHandler::showReport( AbstractReport* const report ) {
	// It cannot be globally defined because the i18n function will not be ready
	static const QString NO_RESULT = i18n( "N/A" );

	// Get the result matrix and free the current row handlers, if any.
	freeRowHandlers();
	if( report != NULL ) {
		kDebug() << "Setting the given report...";
		QMap< QString, RowResult > matrix = report->getResultMatrix();
		tableWidget->setRowCount( matrix.size() );
		tableWidget->resizeRowsToContents();

		// Create brand-new row view handlers
		freeRowHandlers();
		rowViewHandlers = new QList< ReportRowViewHandler* >;
		int numInfected = 0;
		const QString antivirusDate = report->getReportDate().date().toString( Qt::DefaultLocaleShortDate );
		for( QMap< QString, RowResult >::const_iterator curRow = matrix.constBegin(); curRow != matrix.constEnd(); ++curRow ) {
			ReportRowViewHandler* rowViewHandler = new ReportRowViewHandler( this, rowViewHandlers->size(), curRow.key(), antivirusDate, curRow.value() );
			numInfected += rowViewHandler->isInfected() ? 1 : 0;
			rowViewHandlers->append( rowViewHandler );
		}

		// Set the report date
		QString reportDate = report->getReportDate().toString( Qt::DefaultLocaleShortDate );
		mainWindow->getScanAnaylisisDate()->setText( reportDate );

		QString permanentLink;
		FileReport* fileReport = dynamic_cast< FileReport * >( report );
		KUrlLabel* const link = mainWindow->getPermanentLink();
		if( fileReport != NULL ) {
			// Set the MD5, SHA1 and SHA256 hashes
			mainWindow->getMd5Label()->setText( fileReport->getMd5Sum() );
			mainWindow->getSha1Label()->setText( fileReport->getSha1Sum() );
			mainWindow->getSha256Label()->setText( fileReport->getSha256Sum() );

			// Set the permanent URL
			kDebug() << "Permanent URL: " << fileReport->getPermanentLink();
			permanentLink.append( fileReport->getPermanentLink().toString() );
			link->setTextInteractionFlags( Qt::TextBrowserInteraction );
			link->setEnabled( true );
		}
		else {
			// Set default the MD5, SHA1 and SHA256 hashes
			mainWindow->getMd5Label()->setText( NO_RESULT );
			mainWindow->getSha1Label()->setText( NO_RESULT );
			mainWindow->getSha256Label()->setText( NO_RESULT );

			// Set the default permanent URL
			permanentLink.append( NO_RESULT );
			link->setTextInteractionFlags( Qt::NoTextInteraction );
			link->setEnabled( false );
		}
		link->setText( permanentLink );

		// Set the result icon depending on the number of infections detected
		mainWindow->setResultIcon( getReportIconName( numInfected ) );
	}
	else {
		kDebug() << "The given report is null. Resetting the report view...";
		getTableWidget()->setRowCount( 0 ); // Free all rows
		mainWindow->getScanAnaylisisDate()->setText( NO_RESULT );
		mainWindow->getMd5Label()->setText( NO_RESULT );
		mainWindow->getSha1Label()->setText( NO_RESULT );
		mainWindow->getSha256Label()->setText( NO_RESULT );
		mainWindow->setResultIcon( "security-high", false );

		// Set the default permanent URL
		KUrlLabel* const link = mainWindow->getPermanentLink();
		link->setText( NO_RESULT );
		link->setTextInteractionFlags( Qt::NoTextInteraction );
		link->setEnabled( false );
	}
}

void ReportViewHandler::freeRowHandlers() {
	if( rowViewHandlers != NULL ) {
		for( QList< ReportRowViewHandler* >::const_iterator row = rowViewHandlers->constBegin(); row != rowViewHandlers->constEnd(); ++row ) {
			delete *row;
		}
		delete rowViewHandlers;
		rowViewHandlers = NULL;
	}
}

void ReportViewHandler::widthChanged( int width ) {
kDebug() << "ReportViewHandler::widthChanged() " << width;
	//int width = tableWidget->topLevelWidget()->width() - 45;
	width= ( width - 56 ) / tableWidget->columnCount();
	for( int i = 0; i < tableWidget->columnCount(); i++ ) {
		tableWidget->setColumnWidth( i, width );
	}
}

QString ReportViewHandler::getReportIconName( int numInfected ) {
	// Return icon name depending on the number of infections detected
	if( numInfected == 0 ) {
		return "security-high" ;
	}
	if( numInfected < 4 ) {
		return "security-medium";
	}
	return "security-low";
}

#include "reportviewhandler.moc"