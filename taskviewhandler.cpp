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

#include <KMessageBox>
#include <KLocalizedString>
#include <KDebug>

#include "httpconnector.h"
#include "taskrowviewhandler.h"
#include "taskviewhandler.h"
#include <QHeaderView>
#include "mainwindow.h"
#include <settings.h>

static const int DEFAULT_ROW_COUNT = 10;
static const int DEFAULT_COL_COUNT = 5;
static const long SERVICE_MAX_FILE_SIZE = 20 * 1000 * 1000;; // 20 MB

TaskViewHandler::TaskViewHandler( MainWindow* mainwindow, QTableWidget* taskTableWidget, QTableWidget* reportTableWidget ) {
	this->mainwindow 	 = mainwindow;
	this->tableWidget 	 = taskTableWidget;

	setupObject( reportTableWidget );
}

TaskViewHandler::~TaskViewHandler() {
	reportViewHandler->deleteLater();
	timer->deleteLater();
}

void TaskViewHandler::setupObject( QTableWidget* reportTableWidget ){

	tableWidget->setRowCount( DEFAULT_ROW_COUNT );
	tableWidget->setColumnCount( DEFAULT_COL_COUNT  );
	tableWidget->setSelectionBehavior( QAbstractItemView::SelectRows );
	tableWidget->resizeRowsToContents();
	tableWidget->setAlternatingRowColors( true );

	// Set the headers
	QStringList headers;
	tableWidget->setHorizontalHeaderLabels( headers << i18n( "Type" ) << i18n( "Name" ) << i18n( "Size" ) << i18n( "Status" ) << i18n( "Elapsed Time" ) );

	// Set the columns' width done by the user, if any
	QList< int > colsWidths = Settings::self()->taskTableCols();
	kDebug() << "User-defined column widths=" << colsWidths;
	if( !colsWidths.isEmpty() && colsWidths.size() == tableWidget->columnCount() ) {
		for( int i = 0; i < colsWidths.size(); i++ ) {
			tableWidget->setColumnWidth( i, colsWidths.at( i ) );
		}
	}
	// Otherwise, distribute the columns' width based on own approximation algorithm
	else {
		int width = tableWidget->topLevelWidget()->width() - 45;
		for( int i = 0; i < tableWidget->columnCount(); i++ ) {
			tableWidget->setColumnWidth( i, TaskRowViewHandler::getHintColumnSize( i, width ) );
		}
	}

	// Create a shared timer for all rows
	timer = new QTimer( this );
	timer->start( 1000 );

	//Create a common action for all rows
	KAction* removeRowAction = new KAction( "Delete selected row", this ); // Since the msg is not shown, there is no need to use i18n
	removeRowAction->setShortcut( QKeySequence::Delete );
  	tableWidget->addAction( removeRowAction );
	connect( removeRowAction, SIGNAL( triggered( bool ) ), this, SLOT( removeSelectedRows( ) ) );

	// Prepare the report view tableWidget and connect the task row selection it's shown in the report view
	reportViewHandler = new ReportViewHandler( this->mainwindow, reportTableWidget );
	connect( tableWidget, SIGNAL( itemSelectionChanged() ),
			 this, SLOT( selectedRowChanged() ) );

	// Prepare our context menu
	setupContextMenu();
}

void TaskViewHandler::tableWidthChanged( int width ) {
	kDebug() << "TaskViewHandler::widthChanged() " << width;
	int localWidth = width - 41;
	for( int i = 0; i < tableWidget->columnCount(); i++ ) {
		tableWidget->setColumnWidth( i, TaskRowViewHandler::getHintColumnSize( i, localWidth ) );
	}
	reportViewHandler->widthChanged( width );
}

void TaskViewHandler::submitFile( const QString& fileName ) {
	kDebug() << "Submitting file" << fileName << "...";
	if( fileName.isEmpty() ) {
		kDebug() << "Nothing will be done as the file name is empty!";
		return;
	}
	// Check for the file size not being exceeded
	QFile file( fileName );
	if( file.size() > SERVICE_MAX_FILE_SIZE ) {
		KMessageBox::sorry( mainwindow->centralWidget(),
							i18n( "The given file is too big. The service does not accept files greater than %1 MiB (%2 MB).",
								  SERVICE_MAX_FILE_SIZE / ( 1024.0 * 1024 ), SERVICE_MAX_FILE_SIZE / ( 1000 * 1000 ) ),
							i18n( "File size exceeded!" ) );
		return;
	}
	// Check for duplicated submitted files
	if( hasDuplicateUnfinished( fileName ) &&
		KMessageBox::warningContinueCancel( mainwindow->centralWidget(),
											i18n( "The selected file is already being scanned. Do you really want to add another task to scan it again?" ),
										    i18n( "Duplicated scan detected" ),
											KStandardGuiItem::cont(),
											KStandardGuiItem::cancel(),
											QString::null,
											KMessageBox::Dangerous ) != KMessageBox::Continue ) {
		return;
	}
	// Create a new entry
	prepareTable();
	TaskRowViewHandler* rowViewHandler = new TaskRowViewHandler( this, rowViewHandlers.size(), file );
	connect( rowViewHandler, SIGNAL( reportCompleted( int ) ), this, SLOT( reportCompleted( int ) ) );
	addRow2Timer( rowViewHandler );
	rowViewHandlers.append( rowViewHandler );
}


void TaskViewHandler::submitUrl( const QString& urlAddress ) {
	kDebug() << "Submitting URL " << urlAddress << "...";
	if( urlAddress.isEmpty() ) {
		kDebug() << "Nothing will be done as the URL address is empty!";
		return;
	}
	if( hasDuplicateUnfinished( urlAddress ) &&
		KMessageBox::warningContinueCancel( mainwindow->centralWidget(),
											i18n( "The given URL is already being scanned. Do you really want to add another task to scan it again?" ),
										    i18n( "Duplicated scan detected" ),
											KStandardGuiItem::cont(),
											KStandardGuiItem::cancel(),
											QString::null,
											KMessageBox::Dangerous ) != KMessageBox::Continue ) {
		return;
	}
	prepareTable();
	QUrl url( urlAddress );
	TaskRowViewHandler* rowViewHandler = new TaskRowViewHandler( this, rowViewHandlers.size(), url );
	connect( rowViewHandler, SIGNAL( reportCompleted( int ) ), this, SLOT( reportCompleted( int ) ) );
	addRow2Timer( rowViewHandler );
	rowViewHandlers.append( rowViewHandler );
}

bool TaskViewHandler::hasDuplicateUnfinished( const QString& item ) {
	foreach( TaskRowViewHandler* const row, rowViewHandlers ) {
		if( !row->isFinished() && item == row->getName() ) {
			return true;
		}
	}
	return false;
}

void TaskViewHandler::prepareTable() {
	if( rowViewHandlers.size() >= tableWidget->rowCount() ) {
		tableWidget->setRowCount( rowViewHandlers.size() + 1 );
//		tableWidget->selectRow( rowViewHandlers.size() );
		tableWidget->scrollToBottom();
		tableWidget->resizeRowsToContents();
	}
}

void TaskViewHandler::selectedRowChanged() {
	QList< QTableWidgetItem* > items = tableWidget->selectedItems();
	int rowIndex = -1;
	if( items.size() > 0 ) {
		rowIndex = items.at( 0 )->row();
		kDebug() << "Selected row is " << rowIndex;
	}
	selectRow( rowIndex ); // When -1, it will clean the report view
}

void TaskViewHandler::reportCompleted( int rowIndex ) {
	QList< QTableWidgetItem* > items = tableWidget->selectedItems();
	for( int i = 0; i < items.size(); i++ ) {
		QTableWidgetItem* item = items.at( i );
		if( item->row() == rowIndex ) {
			selectRow( rowIndex );
			break;
		}
	}
}

void TaskViewHandler::selectRow( int rowIndex ) {
//	kDebug() << "selectRow() " << rowIndex;
	AbstractReport* report = NULL;
	if( rowIndex > -1 && rowViewHandlers.size() > rowIndex ) {
		TaskRowViewHandler* selectedRow = rowViewHandlers.at( rowIndex );
		report = selectedRow->getReport();
	}
	reportViewHandler->showReport( report );
}

void TaskViewHandler::removeSelectedRows() {
  QList< QTableWidgetSelectionRange > ranges = tableWidget->selectedRanges();
  for( int r = 0; r < ranges.count(); r++ ) {
	QTableWidgetSelectionRange range = ranges.at( r );
	int firstRow = range.topRow();
	int lastRow  = range.bottomRow();
	kDebug() << QString( "Deleting row range[ %1, %2 ]..." ).arg( firstRow ).arg( lastRow );
	// Invocations to removeRow() must be done in reverse-order
	for( int i = lastRow; i >= firstRow; i-- ) {
	  removeRow( i );
	}
  }
}

void TaskViewHandler::removeRow( int rowIndex ) {
  kDebug() << "The selected row is " << rowIndex;
  if( rowViewHandlers.size() > rowIndex ) {
	TaskRowViewHandler* rowViewHandler = rowViewHandlers.at( rowIndex );
	if( rowViewHandler->isFinished() ) {
	  kDebug() << "Deleting row " << rowIndex;
	  rowViewHandlers.removeAt( rowIndex );
	  rowViewHandler->deleteLater();

	  // Move all rows below rowIndex, upwards
	  tableWidget->removeRow( rowIndex );
	  for( int i = rowIndex; i < rowViewHandlers.size(); i++ ) {
		rowViewHandlers.at( i )->setRowIndex( i );
	  }

	  // Adjust the number of rows to its minium
	  QTableWidget*const table = getTableWidget();
	  if( table->rowCount() < DEFAULT_ROW_COUNT ){
		  table->setRowCount( DEFAULT_ROW_COUNT );
		  table->resizeRowsToContents();
	  }
	}
	else {
	  KMessageBox::error( tableWidget,
						  i18n( "The task number %1 is still in course. Please, either wait for it to finished or abort it." ).
								arg( rowIndex + 1 ),
						  i18n( "Remove selected row" ) );
	}
  }
}

void TaskViewHandler::writeSelectedRowIndexes( QSet< int >& rowIds, RowSelection::RowSelectionEnum selection ) {
	QList< QTableWidgetSelectionRange > ranges = tableWidget->selectedRanges();
	const int rowNum = rowViewHandlers.size();
	for( int r = 0; r < ranges.count(); r++ ) {
		QTableWidgetSelectionRange range = ranges.at( r );
		int firstRow = range.topRow();
		int lastRow  = range.bottomRow();
		kDebug() << QString( "Processing range[ %1, %2 ]..." ).arg( firstRow ).arg( lastRow );
		for( int i = firstRow; i <= lastRow && i < rowNum; i++ ) {
			TaskRowViewHandler* const row = rowViewHandlers.at( i );
			switch( selection ) {
				case RowSelection::FINISHED:
					if( row->isFinished() ) {
						rowIds.insert( i );
					}
					break;

				case RowSelection::UNFINISHED:
					if( !row->isFinished() ) {
						rowIds.insert( i );
					}
					break;

				case RowSelection::ALL:
				default:
					rowIds.insert( i );;
					break;
			}
		}
	}
}

void TaskViewHandler::selectAll() {
	const int size = rowViewHandlers.size();
	if( size < 1 ) {
		KMessageBox::sorry( mainwindow->centralWidget(),
							i18n( "There are no task that can be selected!" ),
							i18n( "Action aborted" ) );
		return;
	}

	QTableWidget* const table = getTableWidget();
	const int colNum = table->columnCount();
	for( int i = 0; i < size; i++ ) {
			for( int j = 0; j < colNum; j++ ) {
				table->item( i, j )->setSelected( true );
			}
	}
}

void TaskViewHandler::invertSelection() {
	// If there are rows...
	const int size = rowViewHandlers.size();
	if( size < 1 ) {
		KMessageBox::sorry( mainwindow->centralWidget(),
							i18n( "There are no task that can be selected!" ),
							i18n( "Action aborted" ) );
		return;
	}

	// Get all selected rows
	QSet< int > selectedRowIds;
	writeSelectedRowIndexes( selectedRowIds );

	// Invert the selection
	QTableWidget* const table = getTableWidget();
	const int colNum = table->columnCount();
	for( int i = 0; i < size; i++ ) {
		bool value = !selectedRowIds.contains( i );
		for( int j = 0; j < colNum; j++ ) {
			table->item( i, j )->setSelected( value );
		}
	}

}

void TaskViewHandler::abortSelectedTask() {
	// Collect all real candicates from the user selection
	QSet< int > rowIds2Delete;
	writeSelectedRowIndexes( rowIds2Delete, RowSelection::UNFINISHED );

	// If there are rows to abort, ask the user
	if( !rowIds2Delete.isEmpty() ) {
		if( KMessageBox::questionYesNo( mainwindow->centralWidget(),
										i18np( "Do you really want to abort the selected task?",
											   "Do you really want to abort the %1 selected tasks?", rowIds2Delete.size() ),
										i18n( "Abort selected tasks" ),
										KStandardGuiItem::yes(), KStandardGuiItem::no(),
										QString(), KMessageBox::Dangerous ) == KMessageBox::Yes ) {
			// Abort all the collected items
			foreach( int rowId, rowIds2Delete ) {
				TaskRowViewHandler* const row = rowViewHandlers.at( rowId );
				if( !row->isFinished() ) {
					kDebug() << QString( "Aborting row %1..." ).arg( rowId );
					row->abort();
				}
			}
		}
	}
	// Show a message indication that no row can be aborted
	else {
		KMessageBox::sorry( mainwindow->centralWidget(), i18n( "There is no selected task in course that can be aborted!" ), i18n( "Abort selected tasks" ) );
	}
}

void TaskViewHandler::clearFinishedRows() {
	for( int i = rowViewHandlers.size() - 1; i >=0; i-- ) {
		TaskRowViewHandler* const row = rowViewHandlers.at( i );
		if( row->isFinished() ) {
			removeRow( i );
		}
	}
}

void TaskViewHandler::setupContextMenu() {
	// Create a menu
	ctxMenu = new KMenu( tableWidget );

	// Add "Re-scan" action
	QAction* action = ctxMenu->addAction( i18n( "Re-scan" ), this, SLOT( rescanTasks() ) );
	action->setIcon( KIcon( "view-refresh" ) );

	// Add "Clean finished tasks" action
	action = ctxMenu->addAction( i18n( "Clean finished tasks" ), this, SLOT( clearFinishedRows() ) );
	action->setIcon( KIcon( "run-build-clean" ) );

	// Connect the menu to the task view table
	tableWidget->setContextMenuPolicy( Qt::CustomContextMenu );
	connect( tableWidget, SIGNAL( customContextMenuRequested( const QPoint& ) ),
			 this, SLOT( showContextMenu( const QPoint& ) ) );

}

void TaskViewHandler::showContextMenu( const QPoint& point ) {
	// Re-scan: enable/disable as appropriate
	QSet< int > finishedRowIds;
	writeSelectedRowIndexes( finishedRowIds, RowSelection::FINISHED );
	ctxMenu->actions().at( 0 )->setEnabled( !finishedRowIds.isEmpty() );

	// Clear finished tasks: enable/disable as appropriate
	bool enable = false;
	for( int i = rowViewHandlers.size() - 1; i >=0; i-- ) {
		TaskRowViewHandler* const row = rowViewHandlers.at( i );
		if( row->isFinished() ) {
			enable = true;
			break;
		}
	}
	ctxMenu->actions().at( 1 )->setEnabled( enable );

	// Show the menu
	ctxMenu->exec( tableWidget->mapToGlobal( point ) );
}

void TaskViewHandler::rescanTasks() {
	QSet< int > finishedRowIds;
	writeSelectedRowIndexes( finishedRowIds, RowSelection::FINISHED );
	if( finishedRowIds.isEmpty() ) {
		KMessageBox::sorry( mainwindow, i18n( "There are no selected tasks that can be re-scanned." ) ,i18n( "Re-scan operation error" ) );
		return;
	}
	foreach( int rowIdx, finishedRowIds ) {
		TaskRowViewHandler* const row = rowViewHandlers.at( rowIdx );
		addRow2Timer( row );
		row->rescan();
	}
}

void TaskViewHandler::addRow2Timer( TaskRowViewHandler * rowViewHandler ) {
	if ( !timer->isActive() ) {
		timer->start();
	}
	connect( timer, SIGNAL( timeout() ), rowViewHandler, SLOT( nextSecond() ) );
	connect( rowViewHandler, SIGNAL( unsubscribeNextSecond( TaskRowViewHandler* ) ),
			 this, SLOT( removeRow2Timer( TaskRowViewHandler * ) ) );
}

void TaskViewHandler::removeRow2Timer( TaskRowViewHandler* rowViewHandler ) {
	timer->disconnect( rowViewHandler );
}

bool TaskViewHandler::isUnfinishedTasks() {
	for( int i = 0; i < rowViewHandlers.size(); i++ ) {
		if( !rowViewHandlers.at( i )->isFinished() ) {
			return true;
		}
	}
	return false;
}

#include "taskviewhandler.moc"