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

#ifndef APISERVICEREPLY_H
#define APISERVICEREPLY_H

#include <QString>

#include "base/baseservicereply.h"

namespace ApiServiceReplyResult {
	enum ApiServiceReplyResultEnum { REQUEST_LIMIT_REACHED = -2, INVALID_SERVICE_KEY = -1, ITEM_NOT_PRESENT = 0, ITEM_PRESENT = 1 };
}

class ApiServiceReply : public BaseServiceReply
{
public:
    ApiServiceReply( QString jsonText );
	ApiServiceReplyResult::ApiServiceReplyResultEnum getStatus();
	
protected:
	void processJson( const QMap< QString, QVariant >& json );
};

#endif // APISERVICEREPLY_H