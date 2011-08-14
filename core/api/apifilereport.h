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

#ifndef APIFILEREPORT_H
#define APIFILEREPORT_H

#include "base/basefilereport.h"
#include "servicereply.h"
#include "filehasher.h"

class ApiFileReport : public BaseFileReport
{
private:
	FileHasher* hasher;

protected:
	void processReply( const ServiceReply*const reply );
	
public:
    ApiFileReport( const ServiceReply*const reply, const QString& fileName, FileHasher*const hasher = NULL );
    virtual ~ApiFileReport();
	
	QString getMd5Sum() const;
	QString getSha1Sum() const;
	QString getSha256Sum() const;
};

#endif // APIFILEREPORT_H