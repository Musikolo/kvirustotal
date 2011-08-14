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

#ifndef BASEREPORT_H
#define BASEREPORT_H

#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QMap>
#include <QVariantMap>

#include "report.h"

/** Report base class */
class BaseReport : public virtual Report
{
private :
	ReportType::ReportTypeEnum type;
	QUrl permanentLink;
	QDateTime reportDate;
	QList< ResultItem > resultList;
	int positives;

protected:
	BaseReport( const ReportType::ReportTypeEnum& type ) { this->type = type; this->positives = 0; }
	virtual void setPermanentLink( const QUrl& permanentLink ){ this->permanentLink = permanentLink; }
	virtual void setScanDate( const QDateTime& date ){ this-> reportDate = date; };
	virtual void setNumPositives( int positives ) { this->positives = positives; }
	virtual void setResultList( const QList< ResultItem >& resultList ){ this->resultList = resultList; }
	
public:
	virtual ~BaseReport(){ }
	ReportType::ReportTypeEnum getReportType() const { return this->type; }
	const QUrl& getPermanentLink() const { return this->permanentLink; }
	const QDateTime& getReportDate() const { return this->reportDate; }
	const QList< ResultItem >& getResultList() const { return this->resultList; }
	bool isInfected() const { return this->positives > 0; }
	int getNumPositives() const { return this-> positives; }
};

#endif // BASEREPORT_H