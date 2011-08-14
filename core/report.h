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

#ifndef REPORT_H
#define REPORT_H

#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QMap>
#include <QVariantMap>

namespace ReportType {
	enum ReportTypeEnum { FILE, URL, UNKNOWN };
}

namespace InfectionType {
	enum InfectionTypeEnum { YES, NO, UNKNOWN };
}

struct ResultItem {
	QString antivirus;
	QString version;
	QDateTime lastUpdate;
	QString result;
	InfectionType::InfectionTypeEnum infected;
};

/** Report interface */
class Report
{
protected:
	virtual void setPermanentLink( const QUrl& permanentLink )=0;
	virtual void setScanDate( const QDateTime& date )=0;
	virtual void setNumPositives( int positives )=0;
	virtual void setResultList( const QList< ResultItem >& resultList )=0;
	
public:
	virtual ~Report(){ }
	virtual ReportType::ReportTypeEnum getReportType() const = 0;
	virtual const QUrl& getPermanentLink() const=0;
	virtual const QDateTime& getReportDate() const = 0;
	virtual const QList< ResultItem >& getResultList() const = 0;
	virtual bool isInfected() const = 0;
	virtual int getNumPositives() const = 0;
};

#endif // REPORT_H