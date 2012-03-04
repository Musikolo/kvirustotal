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

#ifndef UPLOADSERVICEREPLY_H
#define UPLOADSERVICEREPLY_H

#include <QString>
class UploadServiceReply
{

private:
	bool valid;
	bool fileExists;
	QString uploadUrl;
	QString lastAnalysisDate;
	
	void processJsonReply( const QString& jsonReply );
	
public:
    UploadServiceReply( const QString& jsonReply );
    virtual ~UploadServiceReply();
	
	bool isValid() const { return this->valid; }
	bool isFileExists() const { return this->fileExists; }
	QString getUploadUrl() const { return this->uploadUrl; }
	QString getLastAnalysisDate() const { return this->lastAnalysisDate; }
};

#endif // UPLOADSERVICEREPLY_H
