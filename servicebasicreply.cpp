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

#include <KDebug>
#include "servicebasicreply.h"

const ServiceReplyResult::ServiceReplyResultEnum ServiceBasicReply::getServiceReplyResult() const {
	return this->result;
}

void ServiceBasicReply::setServiceReplyResult( int value ) {
	switch( value ){
		case ServiceReplyResult::INVALID_SERVICE_KEY:
			kDebug() << "INVALID_SERVICE_KEY";
			result = ServiceReplyResult::INVALID_SERVICE_KEY;
			break;
		case ServiceReplyResult::ITEM_PRESENT:
			result = ServiceReplyResult::ITEM_PRESENT;
			break;
		case ServiceReplyResult::REQUEST_LIMIT_REACHED:
			result = ServiceReplyResult::REQUEST_LIMIT_REACHED;
			break;
		default:
			kDebug() << "Invalid service result value" << value << ". The next value will assumed:" <<  ServiceReplyResult::ITEM_NOT_PRESENT;
		case ServiceReplyResult::ITEM_NOT_PRESENT:
			result = ServiceReplyResult::ITEM_NOT_PRESENT;
			break;
	}
}