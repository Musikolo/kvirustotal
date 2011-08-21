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

#include "filedownloader.h"

#include <KDebug>
#include <QNetworkRequest>
#include <KLocalizedString>
#include <QTemporaryFile>
#include <QFileInfo>

FileDownloader::FileDownloader( QNetworkAccessManager*const networkManager ) {
	this->networkManager = networkManager;
	this->reply = NULL;
	this->running = false;
}

FileDownloader::~FileDownloader() {
	if( reply != NULL ) {
		reply->deleteLater();
		reply = NULL;
	}
}

void FileDownloader::download( const QUrl& url, qint64 maxSizeAllowed ) {
	// Validate that only one file at the same time can be downloaded
	if( running ) {
		kError() << "A file is already being downloaded. Please, try later!";
		emit( errorOccured( "Error" ) );
		return;
	}
	
	// Set the flag, max allowed size and request the file
	running = true;
	this->maxSizeAllowed = maxSizeAllowed;
	QNetworkRequest request( url );
	reply = networkManager->get( request );
	
	// Set up all connections
	connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ), this, SLOT( onDownloadProgress( qint64, qint64 ) ) );
	connect( reply, SIGNAL( finished() ), this, SLOT( onDownloadReady()) );
	connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ), this, SLOT( onErrorOccured( QNetworkReply::NetworkError ) ) );
}

void FileDownloader::onDownloadProgress( qint64 bytesDownloaded, qint64 bytesTotal ) {
	// Validate the maximun size allowed. Abort, if exceeded!
	if( maxSizeAllowed > 0 &&  bytesTotal > maxSizeAllowed ) {
		kDebug() << "The remote file is too big:" << maxSizeAllowed << ">" << bytesTotal << ".Aborting..." ;
		reply->abort();
		emit( maximunSizeExceeded( maxSizeAllowed, bytesTotal ) );
		return;
	}
	
	// Emit signal to inform about the download progress rate
	kDebug() << "Downloaded" << bytesDownloaded << "/" << bytesTotal;
	emit( downloadProgress( bytesDownloaded, bytesTotal ) );
}

void FileDownloader::onDownloadReady() {
	// Process the reply
	kDebug() << "onDownloadReady() event received...";
	if( reply->error() == QNetworkReply::NoError ) {
		QFileInfo info( reply->url().path() );
		QTemporaryFile*const  file = new QTemporaryFile( info.fileName() );
		if( file->open() ) {
			file->write( reply->readAll() );
			emit( downloadReady( file ) );
		}
	}
	else if( reply->error() != QNetworkReply::OperationCanceledError ){
		kError() << "ERROR: onDownloadReady() event error:" << reply->error();
	}
	this->running = false;
	
	// Free the reply
	reply->deleteLater();
	reply = NULL;
}

void FileDownloader::onErrorOccured( QNetworkReply::NetworkError error ) {
	// Unless the download has been aborted, inform about the error
	if( error != QNetworkReply::OperationCanceledError ){
		QString errorMsg( reply->errorString().append( " - (Error code: %1) " ).arg( error ) );
		kError() << "ERROR: " << errorMsg;
		emit( errorOccured( errorMsg ) );
	}
}

bool FileDownloader::abort() {
	if( running && reply != NULL ) {
		reply->abort();
		emit( aborted() );
		return true;
	}
	return false;
}