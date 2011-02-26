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

#ifndef FILEREPORT_H
#define FILEREPORT_H

#include <QString>
#include <QUrl>
#include <QVariantMap>
#include "abstractreport.h"
#include "filehasher.h"

/** Tags used to manage JSON report objects. Extends AbstractReport namespace. */
namespace JsonTag {
	const QString PERMANENT_LINK = "permalink";
}

class FileReport : public virtual AbstractReport
{
private:
	QString fileName;
	QUrl permanentLink;
	bool localHasher;
	FileHasher* hasher;

	/** Sets up the current object from the given JSON string */
	void extendedSetup( const QVariantMap& jsonMap );

public:
	FileReport( const QString& json, const QString& fileName, FileHasher* const hasher = NULL );
	~FileReport();

	const QUrl& getPermanentLink();
	const QString& getMd5Sum();
	const QString& getSha1Sum();
	const QString& getSha256Sum();
};

#endif // FILEREPORT_H
