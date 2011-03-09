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


#ifndef FILEHASHER_H
#define FILEHASHER_H

#include <QString>

class FileHasher
{
private:
	QString md5sum;
	QString sha1sum;
	QString sha256sum;

	void setupObject( const QString& fileName );
	void calculateHashSum( const QByteArray& data, const QString& algorithm, QString& destination );

public:
    FileHasher( const QString& fileName );
	FileHasher( FileHasher*const hasher );
    virtual ~FileHasher();
	const QString& getMd5Sum();
	const QString& getSha1Sum();
	const QString& getSha256Sum();
};

#endif // FILEHASHER_H
