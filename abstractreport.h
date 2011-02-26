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


#ifndef ABSTRACTREPORT_H
#define ABSTRACTREPORT_H

#include <QMap>
#include <QVariantMap>
#include <QString>
#include <QDateTime>

#include "servicebasicreply.h"

/** Tags used to manage JSON report objects. Extends ServiceBasicReply namespace. */
namespace JsonTag {
	const QString REPORT = "report";
}

struct RowResult {
	QString result;
	bool infected;
};

class AbstractReport : public virtual ServiceBasicReply
{
private :
	QDateTime reportDate;
//	QMap< QString, QString > resultMatrix;
	QMap< QString, RowResult > resultMatrix;
	int positives;

protected:
	AbstractReport();
	void commonSetup( const QString& json );
	void commonSetup( const QVariantMap& jsonMap );
	void virtual extendedSetup( const QVariantMap& jsonMap )=0;
	void setReportDate( const QString& date );
	void setReportDate( const QDateTime& date );

public:
    virtual ~AbstractReport();
	const QDateTime & getReportDate() const;
	const QMap< QString, RowResult > & getResultMatrix();
	bool isInfected() const { return this->positives > 0; }
	int getResultMatrixPositives() const { return this-> positives; }
};

#endif // ABSTRACTREPORT_H
