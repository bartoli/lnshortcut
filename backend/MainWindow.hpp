#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <WorkLauncher.hpp>
#include <ui_main.h>

class MainWindow : public QMainWindow//, public WorkLauncher
{
    Q_OBJECT
  public:
    MainWindow();

public slots:
    void analyze_request();
signals:
    void newWorkRequest(const QString& node_pubkey);

private:


    Ui_MainWindow main_ui;
};

#endif // MAINWINDOW_H
