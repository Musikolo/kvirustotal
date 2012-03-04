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

#include "urlsubmissionreply.h"
#include <jsonutil.h>
#include <KDebug>

UrlSubmissionReply::UrlSubmissionReply( const QString& jsonReply ) {
	this->valid = false;
	this->urlExists = false;
	processJsonReply( jsonReply );
}

UrlSubmissionReply::~UrlSubmissionReply() { }

void UrlSubmissionReply::processJsonReply(const QString& jsonReply) {
	if( jsonReply.isEmpty() ) {
		kWarning() << "The param jsonReply is empty!";
		return;
	}
	const QMap< QString, QVariant > json = JsonUtil::getJsonMap( jsonReply, this->valid );
	if( this->valid ) {
		QVariant aux = json[ "url_exists" ];
		if( aux.isValid() && aux.canConvert( QVariant::String ) ) {
			this->urlExists = ( aux.toString().trimmed().toLower() == "true" );
		}
		else {
			this->urlExists = false;
		}
		
		aux = json[ "sha256" ];
		if( aux.isValid() && aux.canConvert( QVariant::String ) ) {
			this->scanId = aux.toString();
		}
		else {
			this->valid = false;
			return;
		}
		
		aux = json[ "last_analysis_date" ];
		if( aux.isValid() && aux.canConvert( QVariant::String ) ) {
			this->lastScanDate = aux.toString();
		}
		else if( this->urlExists ){
			this->valid = false;
			return;
		}
	}
}