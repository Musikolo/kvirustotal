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

#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>

class FileDownloader : public QObject
{
Q_OBJECT
private:
	QNetworkAccessManager* networkManager;
	QNetworkReply* reply;
	bool running;
	qint64 maxSizeAllowed;
	
private slots:
	void onDownloadProgress( qint64 bytesDownloaded, qint64 bytesTotal );
	void onDownloadReady();
	void onErrorOccurred( QNetworkReply::NetworkError );
	
public:
	FileDownloader( QNetworkAccessManager*const networkManager );
	virtual ~FileDownloader();
	
	void download( const QUrl& url, qint64 maxSizeAllowed = 0 );
	bool abort();

signals:
	void downloadProgress( qint64 bytesDownloaded, qint64 bytesTotal );
	void downloadReady( QFile* file ); // The given file should be deleted by the receiver
	void maximunSizeExceeded( qint64 sizeAllowed, qint64 sizeTotal );
	void errorOccurred( const QString& errorMsg );
	void aborted();
};

#endif // FILEDOWNLOADER_H