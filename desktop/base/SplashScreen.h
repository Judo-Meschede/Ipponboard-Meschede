// Copyright 2018 Florian Muecke. Original Ipponboard source under BSD-style license.
// Modifications: Ipponboard-Meschede.
#ifndef WIDGETS__SPLASHSCREEN_H_
#define WIDGETS__SPLASHSCREEN_H_
#include <QDialog>
#include <QDate>
namespace Ui { class SplashScreen; }
class QResizeEvent;
class SplashScreen : public QDialog
{
    Q_OBJECT
public:
    struct Data { QDate date; QString text; QString info; };
    SplashScreen(Data const& data, QWidget* parent = 0);
    ~SplashScreen();
    void SetImageStyleSheet(QString const& text);
protected:
    void changeEvent(QEvent* e);
    bool eventFilter(QObject* watched, QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    Ui::SplashScreen* ui;
    void SyncMasterDataCache();
    void MakeCardChildrenMouseTransparent(QWidget* card);
    void ApplyReferenceGeometry();
private slots:
    void on_commandLinkButton_startSingleVersion_pressed();
    void on_commandLinkButton_startTeamVersion_pressed();
    void on_commandLinkButton_admin_pressed();
    void on_btnSync_clicked();
};
#endif
