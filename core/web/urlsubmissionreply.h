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

#ifndef URLSUBMISSIONREPLY_H
#define URLSUBMISSIONREPLY_H
#include <QString>

class UrlSubmissionReply
{
private:
	bool valid;
	QString scanId;
	bool urlExists;
	QString lastScanDate;
	
	void processJsonReply( const QString& jsonReply );

public:
    UrlSubmissionReply( const QString& jsonReply );
    virtual ~UrlSubmissionReply();
	
	bool isValid() const { return this->valid; }
	QString getScanId() const { return this->scanId; }
	bool isUrlExists() const { return this->urlExists; }
	QString getLastScanDate() const { return this->lastScanDate; }
};

#endif // URLSUBMISSIONREPLY_H