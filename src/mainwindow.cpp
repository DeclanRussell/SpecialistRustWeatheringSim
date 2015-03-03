#include "include/mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
  // create an OpenGL format specifier
  QGLFormat format;
  // set the number of samples for multisampling
  // will need to enable glEnable(GL_MULTISAMPLE); once we have a context
  format.setSamples(4);
  #if defined( DARWIN)
    // at present mac osx Mountain Lion only supports GL3.2
    // the new mavericks will have GL 4.x so can change
    format.setVersion(3,2);
  #else
    // with luck we have the latest GL version so set to this
    format.setVersion(4,3);
  #endif
  // now we are going to set to CoreProfile OpenGL so we can't use and old Immediate mode GL
  format.setProfile(QGLFormat::CoreProfile);
  // now set the depth buffer to 24 bits
  format.setDepthBufferSize(24);
  // now we are going to create our scene window
  m_gl = new NGLScene(format,this);
  ui->s_mainWindowGridLayout->addWidget(m_gl,0,0,1,1);
}

MainWindow::~MainWindow()
{
    delete m_gl;
    delete ui;
}

void MainWindow::on_s_playToggleBtn_clicked()
{
    m_gl->togglePlay();
}

void MainWindow::on_s_resetBtn_clicked()
{
    m_gl->clearRust();
}

void MainWindow::on_s_RDcheckBox_clicked()
{
    m_gl->toggleRandDepModel();
}

void MainWindow::on_checkBox_2_clicked()
{
    m_gl->toggleRandDepVarModel();
}

void MainWindow::on_checkBox_3_clicked()
{
    m_gl->toggleBallisticModel();
}

void MainWindow::on_checkBox_clicked()
{
    m_gl->toggleWireFrameView();
}

void MainWindow::on_spinBox_2_valueChanged(int arg1)
{
    m_gl->setNumBDParticles(arg1);
}

void MainWindow::on_spinBox_valueChanged(int arg1)
{
    m_gl->setNumRDVParticles(arg1);
}

void MainWindow::on_s_RDNumParticles_valueChanged(int arg1)
{
    m_gl->setNumRDParticles(arg1);
}

void MainWindow::on_s_startRedSB_valueChanged(int arg1)
{
    m_gl->setRustStartColour((float)ui->s_startRedSB->value()/255.0,(float)ui->s_startGreenSB->value()/255.0,(float)ui->s_startBlueSB->value()/255.0);
}

void MainWindow::on_s_startGreenSB_valueChanged(int arg1)
{
    m_gl->setRustStartColour((float)ui->s_startRedSB->value()/255.0,(float)ui->s_startGreenSB->value()/255.0,(float)ui->s_startBlueSB->value()/255.0);
}

void MainWindow::on_s_startBlueSB_valueChanged(int arg1)
{
    m_gl->setRustStartColour((float)ui->s_startRedSB->value()/255.0,(float)ui->s_startGreenSB->value()/255.0,(float)ui->s_startBlueSB->value()/255.0);
}

void MainWindow::on_s_endRedSB_valueChanged(int arg1)
{
    m_gl->setRustEndColour((float)ui->s_endRedSB->value()/255.0,(float)ui->s_endGreenSB->value()/255.0,(float)ui->s_endBlueSB->value()/255.0);
}

void MainWindow::on_s_endGreenSB_valueChanged(int arg1)
{
    m_gl->setRustEndColour((float)ui->s_endRedSB->value()/255.0,(float)ui->s_endGreenSB->value()/255.0,(float)ui->s_endBlueSB->value()/255.0);
}

void MainWindow::on_s_endBlueSB_valueChanged(int arg1)
{
    m_gl->setRustEndColour((float)ui->s_endRedSB->value()/255.0,(float)ui->s_endGreenSB->value()/255.0,(float)ui->s_endBlueSB->value()/255.0);
}

void MainWindow::on_s_toggleDrawLatticeCB_clicked(bool checked)
{
    m_gl->toggleDrawLattice(checked);
}

void MainWindow::on_checkBox_4_clicked(bool checked)
{
    m_gl->toggleDPDUpdate(checked);
}

void MainWindow::on_spinBox_3_valueChanged(int arg1)
{
    m_gl->setNumDPDParticles(arg1);
}

void MainWindow::on_s_probFromPixCB_clicked(bool checked)
{
    m_gl->setDPDFromPixel(checked);
    if(checked){
        ui->s_userDefProbSB->setEnabled(false);
    }
    else{
        ui->s_userDefProbSB->setEnabled(true);
    }
}

void MainWindow::on_s_userDefProbSB_valueChanged(int arg1)
{
    m_gl->setDPDProb(arg1);
}

void MainWindow::on_s_DPDSeedRB_clicked()
{
    ui->s_DPDRandomRB->setChecked(false);
    m_gl->setDPDType(NGLScene::Seed);
}

void MainWindow::on_s_DPDRandomRB_clicked()
{
    ui->s_DPDSeedRB->setChecked(false);
    m_gl->setDPDType(NGLScene::Random);
}

void MainWindow::on_s_DPDSeedRB_clicked(bool checked)
{
    ui->s_DPDNumSeedsSB->setEnabled(checked);
}

void MainWindow::on_s_DPDgenSeedsBtn_clicked()
{
    m_gl->genDPDSeeds();
}

void MainWindow::on_s_DPDNumSeedsSB_valueChanged(int arg1)
{
    m_gl->setDPDNumSeeds(arg1);
}

void MainWindow::on_s_DPDrandomSelectRB_clicked(bool checked)
{
    if(checked){
        m_gl->enableLatticeNoise();
        ui->s_DPDfromTexSelectRB->setChecked(false);
    }
}

void MainWindow::on_s_DPDfromTexSelectRB_clicked(bool checked)
{
    if(checked){
        m_gl->enableLatticeFile();
        ui->s_DPDRandomRB->setChecked(false);
    }
}

void MainWindow::on_s_loadTexBtn_clicked()
{
    QString location = QFileDialog::getOpenFileName(this,tr("Open New Lattice Image"), "textures/", tr("Image Files (*.png *.jpg)"));
    if (!location.isEmpty()){
        QImage newImage = QImage(location);
        if(newImage.isNull())
        {
           QMessageBox::information(this,tr("Weathering Sim"),tr("Cannot load %1.").arg(location));
        }
        else{
            ui->s_DPDfromTexSelectRB->setChecked(true);
            ui->s_DPDRandomRB->setChecked(false);
            std::cout<<location.toUtf8().constData()<<std::endl;
            m_gl->loadNewLatticeFile(newImage);
        }
    }
}

void MainWindow::on_s_genNoiseBtn_clicked()
{
    if(ui->s_randomNoiseRB->isChecked()){
        m_gl->genRandomDPDTexture(ui->s_randNoiseDenseSB->value());
        m_gl->enableLatticeNoise();
    }
    else if(ui->s_PerlinNoiseRB->isChecked()){
        m_gl->genPerlinNoise(ui->s_perlinSizeSB->value(),ui->s_perlinOffsetSB->value(),ui->s_perlinOctavesSB->value());
        m_gl->enableLatticeNoise();
    }
}

void MainWindow::on_s_randomNoiseRB_clicked()
{
    ui->s_randNoiseDenseSB->setEnabled(true);
    ui->s_perlinOctavesSB->setEnabled(false);
    ui->s_perlinOffsetSB->setEnabled(false);
    ui->s_perlinSizeSB->setEnabled(false);
}

void MainWindow::on_s_PerlinNoiseRB_clicked()
{
    ui->s_randNoiseDenseSB->setEnabled(false);
    ui->s_perlinOctavesSB->setEnabled(true);
    ui->s_perlinOffsetSB->setEnabled(true);
    ui->s_perlinSizeSB->setEnabled(true);
}

void MainWindow::on_s_loadMeshBtn_clicked()
{
    QString location = QFileDialog::getOpenFileName(this,tr("Open New Lattice Image"), "textures/", tr("Image Files (*.obj)"));
    if (!location.isEmpty()){
        std::cout<<location.toUtf8().constData()<<std::endl;
        m_gl->loadMesh(1,location.toUtf8().constData());
    }
    else{
        QMessageBox::information(this,tr("Weathering Sim"),tr("Cannot load mesh %1.").arg(location));
    }
}

void MainWindow::on_s_loadBseTexture_clicked()
{
    QString location = QFileDialog::getOpenFileName(this,tr("Open New Lattice Image"), "textures/", tr("Image Files (*.png *.jpg)"));
    if (!location.isEmpty()){
        std::cout<<location.toUtf8().constData()<<std::endl;
        m_gl->loadBaseTexture(location.toUtf8().constData());
    }
    else{
        QMessageBox::information(this,tr("Weathering Sim"),tr("Cannot load texture %1.").arg(location));
    }

}

void MainWindow::on_s_saveRustTextureBtn_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Save Location"),"textures/",QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    QString saveLoc = dir + "/" + ui->s_saveTexName->text() + ".png";
    m_gl->saveFinalRustImage(saveLoc.toUtf8().constData());
    std::cout<<saveLoc.toUtf8().constData()<<std::endl;
}

void MainWindow::on_s_probFromPixCB_clicked()
{

}
