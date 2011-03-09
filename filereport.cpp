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

#include <QVariantMap>
#include <KLocalizedString>
#include <QFile>
#include <KDebug>

#include "filereport.h"

FileReport::FileReport( const QString& json, const QString& fileName, FileHasher* const hasher ) {
	this->fileName = fileName;
	this->hasher = ( hasher != NULL ? new FileHasher( hasher ) : new FileHasher( fileName ) );
	commonSetup( json ); // It will call local extendedSetup() method
}

FileReport::~FileReport() {
	// If the FileHasher object is alive, free its memory
	if( hasher != NULL ) {
		delete hasher;
		hasher = NULL;
	}
}

void FileReport::extendedSetup( const QVariantMap & jsonMap ) {
	this->permanentLink = jsonMap[ JsonTag::PERMANENT_LINK ].toUrl();
}

const QUrl & FileReport::getPermanentLink(){
	return this->permanentLink;
}

const QString& FileReport::getMd5Sum() {
	return hasher->getMd5Sum();
}

const QString& FileReport::getSha1Sum() {
	return hasher->getSha1Sum();
}

const QString& FileReport::getSha256Sum() {
	return hasher->getSha256Sum();
}