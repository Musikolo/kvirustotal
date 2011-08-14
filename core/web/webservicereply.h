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

#ifndef WEBSERVICEREPLY_H
#define WEBSERVICEREPLY_H

#include "base/baseservicereply.h"

namespace WebServiceReplyResult {
	enum WebServiceReplyResultEnum { SCAN_REPORT_REUSABLE, SCAN_REPORT_NOT_REUSABLE, SCAN_STARTED, SCAN_QUEUED, SCAN_RUNNING, SCAN_COMPLETED, SCAN_ERROR };
}

namespace WebServiceReplyType {
	enum WebServiceReplyTypeEnum { FILE, URL_SERVICE_1, URL_SERVICE_2 };
}

class WebServiceReply : public BaseServiceReply
{
private:
	WebServiceReplyType::WebServiceReplyTypeEnum replyType;
	int positives;

protected:
	void processHtml( const QString& htmlReply );
	void processJsonMap( const QMap< QString, QVariant >& json );
	void processJsonList( const QList< QVariant >& json );

public:
    WebServiceReply( const QString& textReply, WebServiceReplyType::WebServiceReplyTypeEnum replyType, bool htmlReply = false );
    virtual ~WebServiceReply();

	WebServiceReplyResult::WebServiceReplyResultEnum getStatus();
	int getNumPositives() const { return this->positives; }
	void setNumPositives( int positives ) { this->positives = positives; }
};

#endif // WEBSERVICEREPLY_H