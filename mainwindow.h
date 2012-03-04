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

#ifndef MainWindow_H
#define MainWindow_H

#include <KMainWindow>
#include <QResizeEvent>
#include <KNotification>

#include "ui_mainwindow.h"
#include "taskviewhandler.h"
#include "reportviewhandler.h"
#include "welcomewizard.h"

class MainWindow : public KMainWindow, private Ui::MainWindow
{
Q_OBJECT
private:
	QNetworkAccessManager* networkManager;
	HttpConnector* workloadConnector;
    TaskViewHandler* taskViewHandler;
	QLabel* resultIconLabel;
	WelcomeWizard* wizard;
	FileDownloader* downloader;
	bool manualVersionCheckRequested;

	void dragEnterEvent( QDragEnterEvent* event );
	void dropEvent( QDropEvent* event );
	void setupWorkloadProgressBars();
	void updateWorkloadProgressBars( ServiceWorkload workload );
	void setProgressBarSecurityIcon();
	void validateCurrentVersion();
	bool isPreviousVersion();
	bool isPreviousVersionTo( uchar major, uchar minor, uchar bugfix );
	QUrl promptUrl(  const QString& title, const QString& message  );
	void downloadVersionFile();
	bool validateHttpConnectorProtocol();
	
private slots:
	bool closeRequested();
	void openFile();
	void openRemoteFile();
	void openUrl();
	void openPermanentLink();
	void resizeEvent ( QResizeEvent* event );
	void showWelcomeWizard();
	void wizardFinished( int result );
	void showSettingsDialog();
	void settingsChanged();
	void delayedConnections();
	void onWorkloadReady( ServiceWorkload workload );
	void checkForUpdates();
	void onVersionFileReady( QFile* file );
	
public:
    MainWindow( );
    virtual ~MainWindow();
	inline QLabel* getScanAnaylisisDate(){ return this->scanAnalysisDateValue; }
	inline QLabel* getMd5Label(){ return this->md5Value; }
	inline QLabel* getSha1Label(){ return this->sha1Value; }
	inline QLabel* getSha256Label(){ return this->sha256Value; }
	inline KUrlLabel* getPermanentLink(){ return this->permanentLinkValue; }
	void setResultIcon( const QString& iconName, bool enabled = true );
	void submitFile( const QString& filename );
	void submitRemoteFile( const QUrl& url );
	void submitUrl( const QUrl& url );
	
	static void showInfoNotificaton( const QString& msg );
	static void showWarningNotificaton( const QString& msg );
	static void showErrorNotificaton( const QString& msg );
	static void showCompleteTaskNotificaton( const QString& msg, const QString& iconName );

signals:
	void resizeWidth( int width );
};

#endif // MainWindow_H