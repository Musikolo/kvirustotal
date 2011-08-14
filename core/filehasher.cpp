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

static const QCA::Initializer init; // Required to enable the QCA arquitecture

FileHasher::FileHasher( const QString& fileName ) {
	setupObject( fileName );
}

FileHasher::FileHasher( FileHasher*const hasher ) {
	md5sum    = hasher->md5sum;
	sha1sum   = hasher->sha1sum;
	sha256sum = hasher->sha256sum;
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
	md5sum = calculateHashSum( data, "md5" );
	sha1sum = calculateHashSum( data, "sha1"  );
	sha256sum = calculateHashSum( data, "sha256" );
}

QString FileHasher::calculateHashSum( const QByteArray& data, const QString& algorithm ) {
	kDebug() << QString( "Calculating %1Sum for the given block of data..." ).arg( algorithm.toUpper() );
	if( !QCA::isSupported( algorithm.toAscii() ) ) {
		kDebug() << QString( "Sorry, %1 algorithm is not supported!" ).arg( algorithm.toUpper() );
		return QString();
	}

	// Caculate the hash of the given data and hash algorithm
	QCA::Hash qcaHash( algorithm );
	QString hash = qcaHash.hashToString( data ); 
	kDebug() << QString( "%1Sum is %2" ).arg( algorithm.toUpper() ).arg( hash );
	return hash;
}

QString FileHasher::getMd5Sum() const {
 	return md5sum;
}

QString FileHasher::getSha1Sum() const {
	return sha1sum;
}

QString FileHasher::getSha256Sum() const {
	return sha256sum;
}