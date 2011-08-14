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

#ifndef BASEFILEREPORT_H
#define BASEFILEREPORT_H

#include "basereport.h"
#include "filereport.h"

class BaseFileReport : public BaseReport, public FileReport
{
private:
	QString fileName;

protected:
	void setFileName( const QString& fileName ) { this->fileName = fileName; }

public:
    BaseFileReport() : BaseReport( ReportType::FILE ) { }
    virtual ~BaseFileReport() { }
	
	const QString& getFileName() const { return this->fileName; }
	virtual QString getMd5Sum() const = 0;
	virtual QString getSha1Sum() const = 0;
	virtual QString getSha256Sum() const = 0;
};

#endif // BASEFILEREPORT_H