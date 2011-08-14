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
#ifndef SERVICEREPLY_H
#define SERVICEREPLY_H

#include <QString>
#include <QMap>
#include <QVariant>
#include <QDateTime>
#include <KDebug>
#include <qjson/parser.h>
#include <QUrl>

/** Service reply interface */
class ServiceReply
{
public:
	virtual ~ServiceReply(){ }
	
	virtual bool isValid() const = 0;
	virtual QString getScanId() const = 0;
	virtual QDateTime getScanDate() const = 0;
	virtual QUrl getPermanentLink() const = 0;
	virtual QString getFileReportId() const = 0;
	virtual QMap< QString, QVariant > getScanResult() const = 0;
};

#endif // SERVICEREPLY_H