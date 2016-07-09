#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected slots:
    void openBmp();

    void openPix();

    void save();

protected:

private:
    Ui::MainWindow *ui;

    std::vector<uint8_t> _currentPix;
};

#endif // MAINWINDOW_H
