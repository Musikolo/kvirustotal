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

#include <QFile>
#include <KDebug>
#include <QVariantMap>
#include <QCryptographicHash>
#include <KLocalizedString>
#include <QtCrypto/QtCrypto>

#include "filehasher.h"

static const QString NO_RESULT = "";
static const QCA::Initializer init; // Required to enable the QCA arquitecture

FileHasher::FileHasher( const QString& fileName ) {
	md5sum = NO_RESULT;
	sha1sum = NO_RESULT;
	sha256sum = NO_RESULT;
	setupObject( fileName );
}

FileHasher::FileHasher( FileHasher*const hasher ) {
	md5sum = QString( hasher->md5sum );
	sha1sum = QString( hasher->sha1sum );
	sha256sum = QString( hasher->sha256sum );
}

FileHasher::~FileHasher() {

}

void FileHasher::setupObject( const QString& fileName ) {
	// Create a file object and test whether it exists
	QFile file( fileName );
	if( !file.exists() ) {
		kWarning() << QString( "The file [%1] could not been found." ).arg( fileName );
		return;
	}

	// If it exits, open it and read all its content
	file.open( QIODevice::ReadOnly );
	if( !file.isOpen() ) {
		kWarning() << "Could not open file " << fileName;
		return;
	}
	QByteArray data = file.readAll();
	file.close();

	// Calculte all hashes
	calculateHashSum( data, "md5", md5sum );
	calculateHashSum( data, "sha1", sha1sum );
	calculateHashSum( data, "sha256", sha256sum  );
}

void FileHasher::calculateHashSum( const QByteArray& data, const QString& algorithm, QString& destination ) {
	kDebug() << QString( "Calculating %1Sum for the given block of data..." ).arg( algorithm.toUpper() );
	if( !QCA::isSupported( algorithm.toAscii() ) ) {
		kDebug() << QString( "Sorry, %1 algorithm is not supported!" ).arg( algorithm.toUpper() );
		destination = NO_RESULT;
		return;
	}

	// Caculate the hash of the given data and hash algorithm
	QCA::Hash qcaHash( algorithm );
	destination = qcaHash.hashToString( data );
	kDebug() << QString( "%1Sum is %2" ).arg( algorithm.toUpper() ).arg( destination );
}


const QString& FileHasher::getMd5Sum() {
	return md5sum;
}

const QString& FileHasher::getSha1Sum() {
	return sha1sum;
}

const QString& FileHasher::getSha256Sum() {
	return sha256sum;
}