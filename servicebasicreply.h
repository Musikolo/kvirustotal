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


#ifndef BASICJSONOBJECT_H
#define BASICJSONOBJECT_H

/** Tags used to manage JSON report objects. Intented to be extended by child classes */
namespace JsonTag {
	const QString RESULT = "result";
}

namespace ServiceReplyResult {
	enum ServiceReplyResultEnum { REQUEST_LIMIT_REACHED = -2, INVALID_SERVICE_KEY = -1, ITEM_NOT_PRESENT = 0, ITEM_PRESENT = 1 };
}

class ServiceBasicReply {
private:
	enum ServiceReplyResult::ServiceReplyResultEnum result;

protected:
    inline ServiceBasicReply(){};
	void setServiceReplyResult( int value );

public:
    inline ~ServiceBasicReply(){};
	ServiceReplyResult::ServiceReplyResultEnum getServiceReplyResult() const;
};

#endif // BASICJSONOBJECT_H
