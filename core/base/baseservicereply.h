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
#ifndef BASESERVICEREPLY_H
#define BASESERVICEREPLY_H

#include <QString>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <KDebug>
#include <qjson/parser.h>
#include <QUrl>

#include <servicereply.h>

/** Base service reply */
class BaseServiceReply : public ServiceReply
{
private:
	bool valid;
	int status;
	QString scanId;
	QDateTime scanDate;
	QUrl permanentLink;
	QString fileReportId;
	QMap< QString, QVariant > matrix;

protected:
	BaseServiceReply(){ this->valid = true; } // Optimistic approach
	
	void setValid( bool valid ) { this->valid = valid; }
	void setStatus( int result ) { this->status = result; }
	void setScanId( const QString& scanId ) { this->scanId = scanId; }
	void setScanDate( QDateTime scanDate ) { this->scanDate = scanDate; }
	void setPermanentLink( QUrl url ) { this->permanentLink = url; }
	void setFileReportId( const QString& fileReportId ) { this->fileReportId = fileReportId; }
	void setMatrix( QMap< QString, QVariant > matrix ) { this->matrix = matrix; }
	int getInnerStatus(){ return status; }
	QMap< QString, QVariant > getJsonMap( const QString& jsonText );
	QList< QVariant > getJsonList( const QString& jsonText );
	
public:
	virtual ~BaseServiceReply() { }
	
	bool isValid() const { return valid; }
	QString getScanId() const { return scanId; }
	QDateTime getScanDate() const { return this->scanDate; }
	QUrl getPermanentLink() const { return this->permanentLink; }
	QString getFileReportId() const { return this->fileReportId; }
	QMap< QString, QVariant > getScanResult() const { return this->matrix; }
};

#endif // BASESERVICEREPLY_H