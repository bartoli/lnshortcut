#include <AnalysisThread.hpp>
#include <MainWindow.hpp>

MainWindow::MainWindow()
{
    main_ui.setupUi(this);
    main_ui.node_pubkey->setText("02c521e5e73e40bad13fb589635755f674d6a159fd9f7b248d286e38c3a46f8683");
    QObject::connect(main_ui.analyzeBtn, &QPushButton::clicked, this, &MainWindow::analyze_request);
    QObject::connect(this, &MainWindow::newWorkRequest, AnalysisThread::getInstance(), &AnalysisThread::newWork);

}


void MainWindow::analyze_request()
{
    const QString& node_pubkey = main_ui.node_pubkey->text();
    emit newWorkRequest(node_pubkey);

}
