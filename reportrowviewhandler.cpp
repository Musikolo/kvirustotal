/*
    Copyright (c) 2011 Carlos López Sánchez <musikolo{AT}hotmail[DOT]com

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

#include <QDate>
#include <QTableWidget>
#include <KDebug>

#include "reportrowviewhandler.h"
#include "reportviewhandler.h"
#include <klocalizedstring.h>

namespace Column {
	enum ColumnEnum { ANTIVIRUS, VERSION, LAST_UPDATE_DATE, RESULT };
}

ReportRowViewHandler::ReportRowViewHandler( ReportViewHandler* viewHandler, int rowIndex, const QString& antivirus, const QString& antivirusDate, const RowResult& rowResult  ) {
	this->viewHandler = viewHandler;
	this->rowIndex = rowIndex;
	setupObject( antivirus, antivirusDate, rowResult );
}

ReportRowViewHandler::~ReportRowViewHandler() {

}

void ReportRowViewHandler::setupObject( const QString& antivirus, const QString& antivirusDate, const RowResult& rowResult ) {
	setResult( rowResult ); // Must be the first one!
	setAntivirus( antivirus );
	setVersion( i18n( "N/A" ) );
//	setLastUpdateDate( QDate::currentDate().toString( Qt::DefaultLocaleShortDate ) ); // For the moment being, we'll assume it's up-to-dated
	setLastUpdateDate( antivirusDate );
}

void ReportRowViewHandler::setAntivirus( const QString& antivirus ) {
	this->antivirus = antivirus;
	addRowItem( Column::ANTIVIRUS, antivirus );
}

void ReportRowViewHandler::setVersion( const QString& version ) {
	addRowItem( Column::VERSION, version );
}

void ReportRowViewHandler::setLastUpdateDate( const QString& date ) {
	addRowItem( Column::LAST_UPDATE_DATE, date );
}

void ReportRowViewHandler::setResult( const RowResult& rowResult ) {
//	this->result = result;
	this->rowResult = rowResult;
	addRowItem( Column::RESULT, !rowResult.infected ? i18n( "OK" ) : rowResult.result );
}

void ReportRowViewHandler::addRowItem( int column, const QString& text, const QString& toolTip ) {
	QTableWidget* table = viewHandler->getTableWidget();
	QTableWidgetItem* item = new QTableWidgetItem( text );
	item->setTextAlignment( Qt::AlignCenter );
	item->setToolTip( toolTip );
	table->setItem( rowIndex, column, item );
	if( rowResult.infected ) {
		QColor color( "red" );
		item->setTextColor( color );
	}
}