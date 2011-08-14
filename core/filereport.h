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

#ifndef FILEREPORT_H
#define FILEREPORT_H

#include <QUrl>

#include "report.h"

/** File report interface */
class FileReport : public virtual Report
{
protected:
	virtual void setFileName( const QString& fileName ) = 0;
	
public:
    virtual ~FileReport() { }
	
	virtual const QString& getFileName() const = 0;
	virtual QString getMd5Sum() const = 0;
	virtual QString getSha1Sum() const = 0;
	virtual QString getSha256Sum() const = 0;
};

#endif // FILEREPORT_H