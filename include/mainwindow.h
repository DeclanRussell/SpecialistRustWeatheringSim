#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <NGLScene.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_s_playToggleBtn_clicked();

    void on_s_resetBtn_clicked();

    void on_s_RDcheckBox_clicked();

    void on_checkBox_2_clicked();

    void on_checkBox_3_clicked();

    void on_checkBox_clicked();

    void on_spinBox_2_valueChanged(int arg1);

    void on_spinBox_valueChanged(int arg1);

    void on_s_RDNumParticles_valueChanged(int arg1);

    void on_s_startRedSB_valueChanged(int arg1);

    void on_s_startGreenSB_valueChanged(int arg1);

    void on_s_startBlueSB_valueChanged(int arg1);

    void on_s_endRedSB_valueChanged(int arg1);

    void on_s_endGreenSB_valueChanged(int arg1);

    void on_s_endBlueSB_valueChanged(int arg1);

    void on_s_toggleDrawLatticeCB_clicked(bool checked);

    void on_checkBox_4_clicked(bool checked);

    void on_spinBox_3_valueChanged(int arg1);

    void on_s_probFromPixCB_clicked(bool checked);

    void on_s_userDefProbSB_valueChanged(int arg1);

    void on_s_DPDSeedRB_clicked();

    void on_s_DPDRandomRB_clicked();

    void on_s_DPDSeedRB_clicked(bool checked);

    void on_s_DPDgenSeedsBtn_clicked();

    void on_s_DPDNumSeedsSB_valueChanged(int arg1);

    void on_s_DPDrandomSelectRB_clicked(bool checked);

    void on_s_DPDfromTexSelectRB_clicked(bool checked);

    void on_s_loadTexBtn_clicked();

    void on_s_genNoiseBtn_clicked();

    void on_s_randomNoiseRB_clicked();

    void on_s_PerlinNoiseRB_clicked();

    void on_s_loadMeshBtn_clicked();

    void on_s_loadBseTexture_clicked();

    void on_s_saveRustTextureBtn_clicked();

    void on_s_probFromPixCB_clicked();

private:
    Ui::MainWindow *ui;
    /// @brief Our open gl scene
    NGLScene *m_gl;
};

#endif // MAINWINDOW_H
