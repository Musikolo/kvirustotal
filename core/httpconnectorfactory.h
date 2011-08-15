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

#ifndef HTTPCONECTORFACTORY_H
#define HTTPCONECTORFACTORY_H
#include "httpconnector.h"

namespace HttpConnectorType {
	enum HttpConnectorTypeEnum { API_HTTPCONNECTOR = 0, WEB_HTTPCONNECTOR = 1 };
}

/** HttpConnector factory class that will return the currently active implementation */
class HttpConnectorFactory
{
private:
    HttpConnectorFactory();
	static HttpConnectorType::HttpConnectorTypeEnum getHttpConnectorType();
	
public:
    virtual ~HttpConnectorFactory();
	static HttpConnector* getHttpConnector( QNetworkAccessManager*const networkManager );
	static HttpConnector* getHttpConnector( QNetworkAccessManager*const networkManager, const QString serviceKey );
	static void loadHttpConnectorSettings();
	static HttpConnectorCfg getFileHttpConnectorCfg();
	static HttpConnectorCfg getUrlHttpConnectorCfg();
};

#endif // HTTPCONECTORFACTORY_H