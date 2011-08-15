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

#include "httpconnectorfactory.h"
#include "settings.h"
#include "api/apihttpconnector.h"
#include "web/webhttpconnector.h"

static ApiHttpConnector apiHttpConnector( NULL, "" );
static WebHttpConnector webhttpconnector( NULL );

HttpConnectorFactory::HttpConnectorFactory() { }

HttpConnectorFactory::~HttpConnectorFactory() { }

HttpConnector* HttpConnectorFactory::getHttpConnector( QNetworkAccessManager*const networkManager ) {
	return getHttpConnector( networkManager, Settings::self()->serviceKey() );
}

HttpConnector* HttpConnectorFactory::getHttpConnector( QNetworkAccessManager*const networkManager, const QString serviceKey ) {
	switch( getHttpConnectorType() ) {
		case HttpConnectorType::API_HTTPCONNECTOR:
			return new ApiHttpConnector( networkManager, serviceKey );
		default:
			kWarning() << "Unknown HttpConnectorType " << getHttpConnectorType() << ". Asumming WEB_HTTPCONNECTOR...";
		case HttpConnectorType::WEB_HTTPCONNECTOR:
			return new WebHttpConnector( networkManager );
	}
}

void HttpConnectorFactory::loadHttpConnectorSettings(){
	switch( getHttpConnectorType() ) {
		case HttpConnectorType::API_HTTPCONNECTOR:
			ApiHttpConnector::loadSettings();
			break;
		default:
			kWarning() << "Unknown HttpConnectorType " << getHttpConnectorType() << ". Asumming WebHttpConnector::loadSettings()...";
		case HttpConnectorType::WEB_HTTPCONNECTOR:
			WebHttpConnector::loadSettings();
			break;
	}
}

HttpConnectorType::HttpConnectorTypeEnum HttpConnectorFactory::getHttpConnectorType() {
	switch( Settings::self()->httpConnectorType() ) {
		case 0:
			return HttpConnectorType::API_HTTPCONNECTOR;
		default:
			kWarning() << "Unknown HttpConnectorType " << Settings::self()->httpConnectorType() << ". Asumming WEB_HTTPCONNECTOR_ENGINE...";
		case 1:
			return HttpConnectorType::WEB_HTTPCONNECTOR;
	}
}

HttpConnectorCfg HttpConnectorFactory::getFileHttpConnectorCfg() {
	if( HttpConnectorType::API_HTTPCONNECTOR == getHttpConnectorType() ) {
		return apiHttpConnector.getFileHttpConnectorCfg();
	}
	return webhttpconnector.getFileHttpConnectorCfg();
}

HttpConnectorCfg HttpConnectorFactory::getUrlHttpConnectorCfg() {
	if( HttpConnectorType::API_HTTPCONNECTOR == getHttpConnectorType() ) {
		return apiHttpConnector.getUrlHttpConnectorCfg();
	}
	return webhttpconnector.getUrlHttpConnectorCfg();
}