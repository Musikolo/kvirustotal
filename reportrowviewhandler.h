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

#ifndef REPORTROWVIEWHANDLER_H
#define REPORTROWVIEWHANDLER_H

#include <QString>
#include <report.h>
//#include <reportviewhandler.h>
class ReportViewHandler;

class ReportRowViewHandler
{
private:
    ReportViewHandler* viewHandler;
    int rowIndex;
    QString antivirus;
	ResultItem resultItem;

    void setupObject( const ResultItem& resultItem );
	void setAntivirus( const QString& antivirus );
	void setVersion( const QString& version );
	void setLastUpdateDate( const QString& date );
	void setResultItem( const ResultItem& resultItem );
	void addRowItem(int column, const QString& text, const QString& toolTip = QString() );

public:
    ReportRowViewHandler( ReportViewHandler* viewHandler, int rowIndex, const ResultItem& resultItem );
    virtual ~ReportRowViewHandler();
	inline bool isInfected(){ return this->resultItem.infected == InfectionType::YES; }
};

#endif // REPORTROWVIEWHANDLER_H