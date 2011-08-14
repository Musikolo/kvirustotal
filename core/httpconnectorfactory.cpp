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
	switch( getHttpConnectorEngine() ) {
		case HttpConnectorEngine::API_HTTPCONNECTOR_ENGINE:
			return new ApiHttpConnector( networkManager, serviceKey );
		default:
			kWarning() << "Unknown HttpConnectorEngine " << getHttpConnectorEngine() << ". Asumming WebHttpConnector...";
		case HttpConnectorEngine::WEB_HTTPCONNECTOR_ENGINE:
			return new WebHttpConnector( networkManager );
	}
}

void HttpConnectorFactory::loadHttpConnectorSettings(){
	switch( getHttpConnectorEngine() ) {
		case HttpConnectorEngine::API_HTTPCONNECTOR_ENGINE:
			ApiHttpConnector::loadSettings();
			break;
		default:
			kWarning() << "Unknown HttpConnectorEngine " << getHttpConnectorEngine() << ". Asumming WebHttpConnector::loadSettings()...";
		case HttpConnectorEngine::WEB_HTTPCONNECTOR_ENGINE:
			WebHttpConnector::loadSettings();
			break;
	}
}

HttpConnectorEngine::HttpConnectorEngineEnum HttpConnectorFactory::getHttpConnectorEngine() {
	switch( Settings::self()->httpConnectorEngine() ) {
		case 0:
			return HttpConnectorEngine::API_HTTPCONNECTOR_ENGINE;
		default:
			kWarning() << "Unknown HttpConnectorEngine " << Settings::self()->httpConnectorEngine() << ". Asumming WEB_HTTPCONNECTOR_ENGINE...";
		case 1:
			return HttpConnectorEngine::WEB_HTTPCONNECTOR_ENGINE;
	}
}

HttpConnectorCfg HttpConnectorFactory::getFileHttpConnectorCfg() {
	if( HttpConnectorEngine::API_HTTPCONNECTOR_ENGINE == getHttpConnectorEngine() ) {
		return apiHttpConnector.getFileHttpConnectorCfg();
	}
	return webhttpconnector.getFileHttpConnectorCfg();
}

HttpConnectorCfg HttpConnectorFactory::getUrlHttpConnectorCfg() {
	if( HttpConnectorEngine::API_HTTPCONNECTOR_ENGINE == getHttpConnectorEngine() ) {
		return apiHttpConnector.getUrlHttpConnectorCfg();
	}
	return webhttpconnector.getUrlHttpConnectorCfg();
}