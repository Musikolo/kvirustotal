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

#include "uploadservicereply.h"
#include <QMap>
#include <QVariant>
#include <jsonutil.h>
#include <KDebug>

UploadServiceReply::UploadServiceReply( const QString& jsonText ) {
	this->valid = false;
	processJsonReply( jsonText );
}

UploadServiceReply::~UploadServiceReply() { }

void UploadServiceReply::processJsonReply(const QString& jsonText ) {
	kDebug() << "Data:" << jsonText;
	const QMap< QString, QVariant > data = JsonUtil::getJsonMap( jsonText, this->valid );
	if( this->valid && !data.isEmpty() ) {
		this->fileExists = data[ "file_exists" ].toBool();
		this->uploadUrl = data[ "upload_url" ].toString();
		this->lastAnalysisDate = data[ "last_analysis_date" ].toString();
	}
}