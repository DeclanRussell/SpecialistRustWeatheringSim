#include <QMouseEvent>
#include <QGuiApplication>

#include "NGLScene.h"
#include <ngl/Camera.h>
#include <ngl/Light.h>
#include <ngl/TransformStack.h>
#include <ngl/Material.h>
#include <ngl/NGLInit.h>
#include <ngl/VAOPrimitives.h>
#include <ngl/ShaderLib.h>
#include <ngl/Obj.h>
#include <QFont>
#include <QGLWidget>
#include <ngl/Random.h>


//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for x/y translation with mouse movement
//----------------------------------------------------------------------------------------------------------------------
const static float INCREMENT=0.01;
//----------------------------------------------------------------------------------------------------------------------
/// @brief the increment for the wheel zoom
//----------------------------------------------------------------------------------------------------------------------
const static float ZOOM=0.1;

NGLScene::NGLScene(const QGLFormat _format, QWidget *_parent ) : QGLWidget( _format, _parent )
{
  // Set our window to be the current keyboard focus
  setFocus();
  // re-size the widget to that of the parent (in this case the GLFrame passed in on construction)
  m_rotate=false;
  // mouse rotation values set to 0
  m_spinXFace=0;
  m_spinYFace=0;
  m_play = false;
  m_fps=0;
  m_frames=0;
  m_polyMode=GL_FILL;
  m_randomDepositionModel = false;
  m_randomDepVarModel = false;
  m_BallisticDepModel = false;
  m_DPDModelUpdate = true;
  m_numBDParticles = 100;
  m_numRDParticles = 1000;
  m_numRDVParticles = 500;
  m_numDPDSeeds = 5;
  m_numDPDParticles = 1000;
  m_DPDBlockedPickedProb = 30;
  m_DPDTakeProbFromPixel = true;
  m_CurrentDPDType = Random;
}

void NGLScene::saveFinalRustImage(QString _target){
    QImage finalImage(m_baseTexture.width(), m_baseTexture.height(),QImage::Format_RGB32);
    QColor rustHeight;
    QColor baseTexColour;
    float interpRatio;
    int rustRed,rustBlue,rustGreen, finalRed, finalGreen, finalBlue;

    for (int x=0; x<m_baseTexture.width(); x++){
        for (int y=0; y<m_baseTexture.height(); y++){
            baseTexColour = m_baseTexture.pixel(x,y);
            rustHeight = m_rustData.pixel(x,y);
            interpRatio = rustHeight.red()/255.0;
            rustRed = lerp(m_rustStartColour.red(),m_rustEndColour.red(),interpRatio);
            rustGreen = lerp(m_rustStartColour.green(),m_rustEndColour.green(),interpRatio);
            rustBlue = lerp(m_rustStartColour.blue(),m_rustEndColour.blue(),interpRatio);
            finalRed = lerp(baseTexColour.red(),rustRed,interpRatio);
            finalGreen = lerp(baseTexColour.green(),rustGreen,interpRatio);
            finalBlue = lerp(baseTexColour.blue(),rustBlue,interpRatio);
            finalImage.setPixel(x,y,QColor(finalRed,finalGreen,finalBlue).rgb());
        }
    }
    finalImage.save(_target,"PNG");
}

void NGLScene::loadBaseTexture(std::string filePath)
{

  m_baseTexture = QGLWidget::convertToGLFormat(QImage(&filePath[0]));
  if(!m_baseTexture.isNull())
  {
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    int width=m_baseTexture.width();
    int height=m_baseTexture.height();

    glGenTextures(2,m_texId);
    // make active texture 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,m_texId[0]);
    std::cout<<"the m_textID 0 is "<<m_texId[0]<<std::endl;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,m_baseTexture.bits());

    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps
    // add our base texture location to our shader
    shader->setUniform("baseTex",0);
    // create a texture to hold our rust loactions
    // use RGB32 format so we have 32 bits. This will be used to store floats
    m_rustData = QGLWidget::convertToGLFormat(QImage(width, height,QImage::Format_RGB32));
    QColor rustAmount;
    rustAmount.setAlphaF(0.0);
    for (int i=0; i<width; i++){
        for (int j=0; j<height; j++){
            m_rustData.setPixel(i,j,rustAmount.rgba());
//            std::cout<<rustImage.pixel(i,j)<<std::endl;
        }
    }

    // make active texture 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,m_texId[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,m_rustData.bits());

    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps
    // add our rust location to our shader
    std::cout<<"the m_textID 1 is "<<m_texId[1]<<std::endl;
    shader->setUniform("rustLoc",1);
  }
  else{
      std::cerr<<"Texture is NULL line 42 NGLScene.cpp"<<std::endl;
  }
}



//----------------------------------------------------------------------------------------------------------------------
/// @brief create a cube and stuff it into a VBO on the GPU
/// @param[in] _scale a scale factor for the unit vertices
void NGLScene::loadMesh( GLfloat _scale, std::string _location )
{
//  ngl::Obj mesh("textures/testCube.obj");
  ngl::Obj mesh(_location);
  std::vector<ngl::Face> meshFaces = mesh.getFaceList();
  std::vector<ngl::Vec3> meshVerticies = mesh.getVertexList();
  std::vector<ngl::Vec3> meshNormals = mesh.getNormalList();
  std::vector<ngl::Vec3> meshTexCoords = mesh.getTextureCordList();

   // vertex coords array
  std::vector<ngl::Vec3> vertices;
  std::vector<ngl::Vec3> normals;
  std::vector<GLfloat> texture;
  for (unsigned int i=0; i<mesh.getNumFaces(); i++){
    for (int j=0; j<3; j++){
        vertices.push_back(meshVerticies[meshFaces[i].m_vert[j]]);
        normals.push_back(meshNormals[meshFaces[i].m_vert[j]]);
        texture.push_back((GLfloat)meshTexCoords[meshFaces[i].m_tex[j]].m_x);
        texture.push_back((GLfloat)meshTexCoords[meshFaces[i].m_tex[j]].m_y);
    }
  }

  // first we scale our vertices to _scale
  for(uint i=0; i<sizeof(vertices)/sizeof(GLfloat); ++i)
  {
    vertices[i]*=_scale;
  }

  // first we create a vertex array Object
  glGenVertexArrays(1, &m_vaoID);

  // now bind this to be the currently active one
  glBindVertexArray(m_vaoID);
  // now we create three VBO's one for each of the objects these are only used here
  // as they will be associated with the vertex array object
  GLuint vboID[3];
  glGenBuffers(3, &vboID[0]);
  // now we will bind an array buffer to the first one and load the data for the verts
  glBindBuffer(GL_ARRAY_BUFFER, vboID[0]);
  glBufferData(GL_ARRAY_BUFFER, vertices.size()*3*sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);
  // now we bind the vertex attribute pointer for this object in this case the
  // vertex data
  glVertexAttribPointer(0,3,GL_FLOAT,false,0,0);
  glEnableVertexAttribArray(0);
  // now we repeat for the Normal data using the second VBO
  glBindBuffer(GL_ARRAY_BUFFER, vboID[1]);
  glBufferData(GL_ARRAY_BUFFER, normals.size()*3*sizeof(GLfloat), &normals[0], GL_STATIC_DRAW);
  glVertexAttribPointer(1,3,GL_FLOAT,false,0,0);
  glEnableVertexAttribArray(1);
  // now we repeat for the UV data using the third VBO
  glBindBuffer(GL_ARRAY_BUFFER, vboID[2]);
  glBufferData(GL_ARRAY_BUFFER, texture.size()*2*sizeof(GLfloat), &texture[0], GL_STATIC_DRAW);
  glVertexAttribPointer(2,2,GL_FLOAT,false,0,0);
  glEnableVertexAttribArray(2);

}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::setRustStartColour(float _r, float _g, float _b){
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    (*shader)["TextureShader"]->use();
    shader->setUniform("rustStartColour",ngl::Vec4(_r,_g,_b,1.0));
    m_rustStartColour.setRgb(_r,_g,_b);
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::setRustEndColour(float _r, float _g, float _b){
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    (*shader)["TextureShader"]->use();
    shader->setUniform("rustEndColour",ngl::Vec4(_r,_g,_b,1.0));
    m_rustEndColour.setRgb(_r,_g,_b);
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::clearRust(){
    for (int i=0; i<m_rustData.width(); i++){
        for (int j=0; j<m_rustData.height(); j++){
            m_rustData.setPixel(i,j,0);
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::togglePlay(){
    if (m_play){
        m_play = false;
    }
    else{
        m_play = true;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::toggleWireFrameView(){
    if (m_polyMode==GL_LINE){
        m_polyMode = GL_FILL;
    }
    else{
        m_polyMode=GL_LINE;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::toggleRandDepModel(){
    if (m_randomDepositionModel){
        m_randomDepositionModel = false;
    }
    else{
        m_randomDepositionModel = true;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::toggleRandDepVarModel(){
    if (m_randomDepVarModel){
        m_randomDepVarModel = false;
    }
    else{
        m_randomDepVarModel = true;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::toggleBallisticModel(){
    if (m_BallisticDepModel){
        m_BallisticDepModel = false;
    }
    else{
        m_BallisticDepModel = true;
    }
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::toggleDrawLattice(bool _toggle){
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    (*shader)["TextureShader"]->use();
    shader->setUniform("drawLattice",_toggle);
}

//----------------------------------------------------------------------------------------------------------------------
/// need to search each ring grabbing the lowest point until you reach a ring where you cannot get a lower point
ngl::Vec2 NGLScene::lowestNeighbouringPosition(int levelsOfSearch, int sampleX, int sampleY){
    QColor lowestNeighbourHeight = m_rustData.pixel(sampleX,sampleY);
    QColor tempColour;
    ngl::Vec2 lowestNeighbourPoint(sampleX,sampleY);
    int lowerX = sampleX-levelsOfSearch;
    int higherX = sampleX+levelsOfSearch;
    int lowerY = sampleY-levelsOfSearch;
    int higherY = sampleY+levelsOfSearch;
        // Check that the postition is within our texture space
    if (lowerY<0){
        lowerY = 0;
    }
    if (higherY>=m_rustData.height()){
        higherY=m_rustData.height()-1;
    }
    if (lowerX<0){
        lowerX = 0;
    }
    if (higherX>=m_rustData.width()){
        higherX=m_rustData.width()-1;
    }
    for(int x=lowerX; x<=higherX;x++){
        // compare if the current value of our pixel is lower than our current lowest
        // if successful set our new lowest neighbour
        tempColour = m_rustData.pixel(x,lowerY);
        if(tempColour.red()<lowestNeighbourHeight.red()){
            lowestNeighbourHeight = m_rustData.pixel(x,lowerY);
            lowestNeighbourPoint.set(x,lowerY);
            return lowestNeighbourPoint;
        }
        tempColour = m_rustData.pixel(x,higherY);
        if(tempColour.red()<lowestNeighbourHeight.red()){
            lowestNeighbourHeight = m_rustData.pixel(x,higherY);
            lowestNeighbourPoint.set(x,higherY);
            return lowestNeighbourPoint;
        }
    }

    for(int y=lowerY; y<=higherY;y++){
        // compare if the current value of our pixel is lower than our current lowest
        // if successful set our new lowest neighbour
        tempColour = m_rustData.pixel(lowerX,y);
        if(tempColour.red()<lowestNeighbourHeight.red()){
            lowestNeighbourHeight = m_rustData.pixel(lowerX,y);
            lowestNeighbourPoint.set(lowerX,y);
            return lowestNeighbourPoint;
        }
        tempColour = m_rustData.pixel(higherX,y);
        if(tempColour.red()<lowestNeighbourHeight.red()){
            lowestNeighbourHeight = m_rustData.pixel(higherX,y);
            lowestNeighbourPoint.set(higherX,y);
            return lowestNeighbourPoint;
        }
    }
    return lowestNeighbourPoint;
}
//----------------------------------------------------------------------------------------------------------------------
std::vector<ngl::Vec2> NGLScene::findClosestNonFullNeighbours(int _x, int _y){
    std::vector<ngl::Vec2> closestNonFullNeighbours;
    int levelsOfSearch = 0;
    int lowerX = _x-levelsOfSearch;
    int higherX = _x+levelsOfSearch;
    int lowerY = _y-levelsOfSearch;
    int higherY = _y+levelsOfSearch;
    QColor tempColour;
    while(closestNonFullNeighbours.size()==0){
        levelsOfSearch++;
        lowerX = _x-levelsOfSearch;
        higherX = _x+levelsOfSearch;
        lowerY = _y-levelsOfSearch;
        higherY = _y+levelsOfSearch;
        // Check that the postition is within our texture space
        if (lowerY<0){
            lowerY = 0;
        }
        if (higherY>=m_rustData.height()){
            higherY=m_rustData.height()-1;
        }
        if (lowerX<0){
            lowerX = 0;
        }
        if (higherX>=m_rustData.width()){
            higherX=m_rustData.width()-1;
        }
        for(int x=lowerX; x<=higherX;x++){
            // compare if the current value of our pixel is lower than our current lowest
            // if successful set our new lowest neighbour
            tempColour = m_rustData.pixel(x,lowerY);
            if(tempColour.red()<255){
                closestNonFullNeighbours.push_back(ngl::Vec2(x,lowerY));
            }
            tempColour = m_rustData.pixel(x,higherY);
            if(tempColour.red()<255){
                closestNonFullNeighbours.push_back(ngl::Vec2(x,higherY));
            }
        }

        for(int y=lowerY; y<=higherY;y++){
            // compare if the current value of our pixel is lower than our current lowest
            // if successful set our new lowest neighbour
            tempColour = m_rustData.pixel(lowerX,y);
            if(tempColour.red()<255){
                closestNonFullNeighbours.push_back(ngl::Vec2(lowerX,y));
            }
            tempColour = m_rustData.pixel(higherX,y);
            if(tempColour.red()<255){
                closestNonFullNeighbours.push_back(ngl::Vec2(higherX,y));
            }
        }
    }
    return closestNonFullNeighbours;
}

//----------------------------------------------------------------------------------------------------------------------
//--------------Deriected percolation depinning Model-------------------------------------------------------------------
void NGLScene::genDPDSeeds(){
    m_DPDSeeds.clear();
    int seedXPos;
    int seedYPos;
    std::cout<<"seeds generated are "<<std::endl;
    ngl::Random *rnd = ngl::Random::instance();
    for(int i=0; i<m_numDPDSeeds; i++){
        seedXPos = (int)rnd->randomPositiveNumber(m_rustData.width()-1);
        seedYPos = (int)rnd->randomPositiveNumber(m_rustData.height()-1);
        std::cout<<seedXPos<<","<<seedYPos<<std::endl;
        m_DPDSeeds.push_back(ngl::Vec2(seedXPos,seedYPos));
    }
}

void NGLScene::loadNewLatticeFile(QImage _newImage){
    m_DPDLatticeData = _newImage;
    m_DPDLatticeFromFile = _newImage;
    enableLatticeFile();
}

float NGLScene::Noise(int _x, int _y){
    int num = _x + _y * 57;
    num = (num<<13) ^ num;
    return ( 1.0 - ( (num * (num * num * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);
}

float NGLScene::smoothNoise(float _x, float _y){
    float corners = ( Noise(_x-1, _y-1)+Noise(_x+1, _y-1)+Noise(_x-1, _y+1)+Noise(_x+1, _y+1) ) / 16;
    float sides   = ( Noise(_x-1, _y)  +Noise(_x+1, _y)  +Noise(_x, _y-1)  +Noise(_x, _y+1) ) /  8;
    float center  =  Noise(_x, _y) / 4;
    return corners + sides + center;
}

float NGLScene::interpolatedNoise(float _x, float _y){
    int integer_X    = (int)_x;
    float fractional_X = _x - integer_X;

    int integer_Y    = (int)_y;
    float fractional_Y = _y - integer_Y;

    float v1 = smoothNoise(integer_X,     integer_Y);
    float v2 = smoothNoise(integer_X + 1, integer_Y);
    float v3 = smoothNoise(integer_X,     integer_Y + 1);
    float v4 = smoothNoise(integer_X + 1, integer_Y + 1);

    float i1 = lerp(v1 , v2 , fractional_X);
    float i2 = lerp(v3 , v4 , fractional_X);

    return lerp(i1 , i2 , fractional_Y);
}

float NGLScene::PerlinNoise(int _x, int _y, float _size, float _offset, int _octaves){
    float total = 0;
    float persistence = 1.0/2.0;
    float frequency = 0;
    float amplitude = 0;
    for(int i=0; i<_octaves;i++){
        frequency = pow(2,i);
        amplitude = pow(persistence,i);
        total = total + interpolatedNoise((_x+_offset)/_size*frequency,(_y+_offset)/_size*frequency) * amplitude;
    }
    return total;
}

void NGLScene::genPerlinNoise(float _size, float _offset, int _octaves){
    QImage DPDLatticeData = QImage(m_baseTexture.width(),m_baseTexture.height(),QImage::Format_RGB32);
    QColor currentColour;
    float perlinNoise;
    for (int i=0; i<m_baseTexture.width(); i++){
        for (int j=0; j<m_baseTexture.height(); j++){
            //fill our lattice with some random data
            //50/50 probability
            perlinNoise = ((PerlinNoise(i,j,_size,_offset,_octaves)+1)/2)*255.0;
            DPDLatticeData.setPixel(i,j,QColor(perlinNoise,perlinNoise,perlinNoise,1).rgba());
        }
    }
    m_DPDLatticeNoise = DPDLatticeData;
}

void NGLScene::enableLatticeNoise(){
    //change our lattice texture to our noise texture
    m_DPDLatticeData = m_DPDLatticeNoise;
    //load our new texture to openGL
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,m_texId[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,m_DPDLatticeData.width(),m_DPDLatticeData.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,m_DPDLatticeData.bits());
    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps
}

void NGLScene::enableLatticeFile(){
    //change our lattice texture to our loaded in texture
    m_DPDLatticeData = m_DPDLatticeFromFile;
    //load our new texture to openGL
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,m_texId[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,m_DPDLatticeData.width(),m_DPDLatticeData.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,m_DPDLatticeData.bits());
    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps
}

void NGLScene::genRandomDPDTexture(float _density){
    QImage DPDLatticeData = QImage(m_baseTexture.width(),m_baseTexture.height(),QImage::Format_RGB32);
    ngl::Random *rnd = ngl::Random::instance();
    QColor currentColour;
    for (int i=0; i<m_baseTexture.width(); i++){
        for (int j=0; j<m_baseTexture.height(); j++){
            //fill our lattice with some random data
            //50/50 probability
            if(rnd->randomPositiveNumber()<_density){
                int randHeight = rnd->randomPositiveNumber(255);
                DPDLatticeData.setPixel(i,j,QColor(randHeight,randHeight,randHeight,1).rgba());
            }
            else{
                DPDLatticeData.setPixel(i,j,QColor(0,0,0,1).rgba());
            }
            currentColour = DPDLatticeData.pixel(i,j);
        }
    }
    m_DPDLatticeNoise = DPDLatticeData;
}

void NGLScene::DPDModelInit(){
    // Generate our lattice information
//    genRandomDPDTexture(0.5);
    // set perlin noise as our current lattice texture
    genPerlinNoise();
    m_DPDLatticeFromFile = QImage("textures/perlin_noise.png","PNG");
    m_DPDLatticeData = m_DPDLatticeNoise;

    // add the texture to openGL
    glGenTextures(1,&m_texId[2]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D,m_texId[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,m_DPDLatticeData.width(),m_DPDLatticeData.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,m_DPDLatticeData.bits());
    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps
    // add our rust location to our shader
    std::cout<<"the m_textID 2 is "<<m_texId[1]<<std::endl;
    ngl::ShaderLib *shader=ngl::ShaderLib::instance();
    (*shader)["TextureShader"]->use();
    shader->setUniform("latticeTex",2);
    genDPDSeeds();
}
void NGLScene::DPDModelRandom(){
    ngl::Vec2 currentPos;
    ngl::Vec2 diffusePoint;
    ngl::Random *rnd = ngl::Random::instance();
    QColor currentLatticeValue;
    int blockedProb;
    QColor rustAmount;
    bool isFull=false;
    //for every particle
    for(int i=0;i<m_numDPDParticles;i++){
//        currentPos = m_DPDSeeds[currentSeed];
//        diffusePoint = lowestNeighbouringPosition(currentLevel,currentPos.m_x ,currentPos.m_y);
        //50/50 probability of diffusing the particle from the first position
//        std::cout<<"start pixel"<<currentPos.m_x<<","<<currentPos.m_y<<std::endl;

        currentPos.set(rnd->randomPositiveNumber(m_rustData.width()),rnd->randomPositiveNumber(m_rustData.height()));
        currentPos.m_x = (int)currentPos.m_x;
        currentPos.m_y = (int)currentPos.m_y;
        //if the current pixel is full then diffuse the particle a bit
        if(rustAmount.red()==255){
            isFull=true;
            while(isFull){
                //diffuse a random amount
                currentPos.m_x+=(int)rnd->randomNumber(50);
                currentPos.m_y+=(int)rnd->randomNumber(50);
                //make sure that it is in range of the texture
                if(currentPos.m_x>=m_rustData.width()){
                    currentPos.m_x = 0;
                }
                if(currentPos.m_x<0){
                    currentPos.m_x = m_rustData.width()-1;
                }
                if(currentPos.m_y>=m_rustData.height()){
                    currentPos.m_y = 0;
                }
                if(currentPos.m_y<0){
                    currentPos.m_y = m_rustData.height()-1;
                }

                rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                // the higher the value the more chance of diffusing more
                if(rnd->randomPositiveNumber(100)>((rustAmount.red()/255.0)*100.0)){
                    isFull=false;
                }
            }
        }
        currentLatticeValue = m_DPDLatticeData.pixel(currentPos.m_x,currentPos.m_y);
        //if we want to calculate the blocked pixel probability from the value in the lattice texture
        if(m_DPDTakeProbFromPixel){
            blockedProb = (currentLatticeValue.red()/255.0)*100;
            if(rnd->randomPositiveNumber(100)<(blockedProb)){
                rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                if((rustAmount.red()+25)<255){
                    m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(rustAmount.red()+25,0,0).rgba());
                }
                else{
                    m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(255,0,0).rgba());
                }
            }
        }
        //else just take probability from user defined
        else{
            blockedProb = m_DPDBlockedPickedProb;
            if(currentLatticeValue.red()>30){
                if(rnd->randomPositiveNumber(100)<(100-blockedProb)){
                    rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                    if((rustAmount.red()+25)<255){
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(rustAmount.red()+25,0,0).rgba());
                    }
                    else{
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(255,0,0).rgba());
                    }
                }
            }
            else{
                if(rnd->randomPositiveNumber(100)<(blockedProb)){
                    rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                    if((rustAmount.red()+25)<255){
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(rustAmount.red()+25,0,0).rgba());
                    }
                    else{
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(255,0,0).rgba());
                    }
                }
            }
        }

    }


}
void NGLScene::DPDModelSeed(){
    ngl::Vec2 currentPos;
    ngl::Vec2 diffusePoint;
    ngl::Random *rnd = ngl::Random::instance();
    QColor currentLatticeValue;
    int blockedProb;
    int currentSeed = 0;
    QColor rustAmount;
    int isFull = false;

    //for every particle
    for(int i=0;i<m_numDPDParticles;i++){
        currentPos = m_DPDSeeds[currentSeed];
        rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
        //if the current pixel is full then diffuse the particle a bit
        if(rustAmount.red()==255){
            isFull=true;
            while(isFull){
                //diffuse a random amount
                currentPos.m_x+=(int)rnd->randomNumber(50);
                currentPos.m_y+=(int)rnd->randomNumber(50);
                //make sure that it is in range of the texture
                if(currentPos.m_x>=m_rustData.width()){
                    currentPos.m_x = 0;
                }
                if(currentPos.m_x<0){
                    currentPos.m_x = m_rustData.width()-1;
                }
                if(currentPos.m_y>=m_rustData.height()){
                    currentPos.m_y = 0;
                }
                if(currentPos.m_y<0){
                    currentPos.m_y = m_rustData.height()-1;
                }

                rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                // the higher the value the more chance of diffusing more
                if(rnd->randomPositiveNumber(100)>((rustAmount.red()/255.0)*100.0)){
                    isFull=false;
                }
            }
        }
        // Now we have our position lets add some rust
        currentLatticeValue = m_DPDLatticeData.pixel(currentPos.m_x,currentPos.m_y);
        //if we want to calculate the blocked pixel probability from the value in the lattice texture
        if(m_DPDTakeProbFromPixel){
            blockedProb = (currentLatticeValue.red()/255.0)*100;
            if(rnd->randomPositiveNumber(100)<(blockedProb)){
                rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                if((rustAmount.red()+25)<255){
                    m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(rustAmount.red()+25,0,0).rgba());
                }
                else{
                    m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(255,0,0).rgba());
                }
            }
        }
        //else just take probability from user defined
        else{
            blockedProb = m_DPDBlockedPickedProb;
            if(currentLatticeValue.red()>30){
                if(rnd->randomPositiveNumber(100)<(100-blockedProb)){
                    rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                    if((rustAmount.red()+25)<255){
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(rustAmount.red()+25,0,0).rgba());
                    }
                    else{
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(255,0,0).rgba());
                    }
                }
            }
            else{
                if(rnd->randomPositiveNumber(100)<(blockedProb)){
                    rustAmount = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
                    if((rustAmount.red()+25)<255){
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(rustAmount.red()+25,0,0).rgba());
                    }
                    else{
                        m_rustData.setPixel(currentPos.m_x,currentPos.m_y,QColor(255,0,0).rgba());
                    }
                }
            }
        }

        // increment the current seed to next seed in list
        if((currentSeed+1)<m_numDPDSeeds){
            currentSeed++;
        }
        else{
            currentSeed=0;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
//--------------Random Deposition Model Variation-----------------------------------------------------------------------
void NGLScene::randomDepVariationModel(){
    for(int i=0; i<m_numRDVParticles;i++){
        ngl::Random *rnd = ngl::Random::instance();
        ngl::Vec3 randPixel;
        randPixel = rnd->getRandomPoint(m_rustData.width(),m_rustData.height());
        if (randPixel.m_x<0) randPixel.m_x=randPixel.m_x*-1;
        if (randPixel.m_y<0) randPixel.m_y=randPixel.m_y*-1;
        randPixel.m_x = (int)randPixel.m_x;
        randPixel.m_y = (int)randPixel.m_y;
        ngl::Vec2 currentPos(randPixel.m_x,randPixel.m_y);
        ngl::Vec2 diffusePoint = lowestNeighbouringPosition(1,randPixel.m_x ,randPixel.m_y);
        bool comeToRest = false;
        int currentLevel = 1;
        QColor currentHeight;
        while(!comeToRest){
            currentHeight = m_rustData.pixel(currentPos.m_x,currentPos.m_y);
            //if the particle has settled
            if(currentPos==diffusePoint&&currentHeight.red()!=255){
                comeToRest=true;
            }
            else{
                currentPos = diffusePoint;
                // if a big block of max height search further away
                if(currentHeight.red()==255)
                    currentLevel++;
                diffusePoint = lowestNeighbouringPosition(currentLevel,diffusePoint.m_x ,diffusePoint.m_y);
            }
            //so that we dont crash
            if(currentLevel>20){
                comeToRest=true;
            }
        }

        QColor rustAmount = m_rustData.pixel(diffusePoint.m_x,diffusePoint.m_y);
        if((rustAmount.red()+25)<250){
            m_rustData.setPixel(diffusePoint.m_x,diffusePoint.m_y,QColor(rustAmount.red()+25,0,0).rgb());
        }
        else{
            m_rustData.setPixel(diffusePoint.m_x,diffusePoint.m_y,QColor(255,0,0).rgb());
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
//------------Random Deposition Model-----------------------------------------------------------------------------------
void NGLScene::randomDepModel(){
    for(int i=0; i<m_numRDParticles;i++){
        ngl::Random *rnd = ngl::Random::instance();
        ngl::Vec3 randPixel;
        randPixel = rnd->getRandomPoint(m_rustData.width(),m_rustData.height());
        if (randPixel.m_x<0) randPixel.m_x=randPixel.m_x*-1;
        if (randPixel.m_y<0) randPixel.m_y=randPixel.m_y*-1;
        randPixel.m_x = (int)randPixel.m_x;
        randPixel.m_y = (int)randPixel.m_y;
        QColor rustAmount = m_rustData.pixel(randPixel.m_x,randPixel.m_y);
//        std::cout<<rustAmount<<std::endl;
        if(rustAmount.red()<250)
        m_rustData.setPixel(randPixel.m_x,randPixel.m_y,QColor(rustAmount.red()+25,0,0).rgb());
    }
}
//----------------------------------------------------------------------------------------------------------------------
int NGLScene::maxNeighbouringHeight(int levelsOfSearch, int sampleX, int sampleY){
    int maxNeighbourPoint = 0;
    QColor tempColor;
    int finalX=0, finalY=0;
    int lowerX = sampleX-levelsOfSearch;
    int higherX = sampleX+levelsOfSearch;
    int lowerY = sampleY-levelsOfSearch;
    int higherY = sampleY+levelsOfSearch;
    for(int x=lowerX; x<higherX;x++){
        for(int y=lowerY; y<higherY;y++){
            if (x<0){
                finalX = 0;
            }
            else if (x>=m_rustData.width()){
                finalX=m_rustData.width()-1;
            }
            else{
                finalX=x;
            }
            if (y<0){
                finalY = 0;
            }
            else if (y>=m_rustData.height()){
                finalY=m_rustData.height()-1;
            }
            else{
                finalY=y;
            }
            tempColor = m_rustData.pixel(finalX,finalY);
            if(tempColor.red()>maxNeighbourPoint){
                maxNeighbourPoint = tempColor.red();
            }
        }
    }
    return maxNeighbourPoint;
}
//----------------------------------------------------------------------------------------------------------------------
//--------------Ballistic Deposition Model------------------------------------------------------------------------------
void NGLScene::ballisticDepModel(){
    for(int i=0; i<m_numBDParticles;i++){
        ngl::Random *rnd = ngl::Random::instance();
        ngl::Vec3 randPixel;
        randPixel = rnd->getRandomPoint(m_rustData.width(),m_rustData.height());
        if (randPixel.m_x<0) randPixel.m_x=randPixel.m_x*-1;
        if (randPixel.m_y<0) randPixel.m_y=randPixel.m_y*-1;
        randPixel.m_x = (int)randPixel.m_x;
        randPixel.m_y = (int)randPixel.m_y;
        // find the highest neighbouring point
        int highestNeighbour = maxNeighbouringHeight(5,randPixel.m_x,randPixel.m_y);
        QColor rustAmount = m_rustData.pixel(randPixel.m_x,randPixel.m_y);
//        std::cout<<rustAmount<<std::endl;
        if (highestNeighbour>rustAmount.red()+25){
            if(rustAmount.red()<250)
            m_rustData.setPixel(randPixel.m_x,randPixel.m_y,QColor(highestNeighbour,0,0).rgb());
        }
        else{
            if(rustAmount.red()<250)
            m_rustData.setPixel(randPixel.m_x,randPixel.m_y,QColor(rustAmount.red()+25,0,0).rgb());
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------
NGLScene::~NGLScene()
{
  ngl::NGLInit *Init = ngl::NGLInit::instance();
  std::cout<<"Shutting down NGL, removing VAO's and Shaders\n";
  Init->NGLQuit();
  // remove the textures now we are done
  glDeleteTextures(1,m_texId);
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::resizeGL(const int _w, const int _h)
{
  // set the viewport for openGL
  glViewport(0,0,_w,_h);
  // now set the camera size values as the screen size has changed
  m_cam->setShape(45,(float)_w/_h,0.05,350);
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::initializeGL()
{
  // we must call this first before any other GL commands to load and link the
  // gl commands from the lib, if this is not done program will crash
  ngl::NGLInit::instance();

  glClearColor(0.3f, 0.7f, 0.8f, 1.0f);			   // Grey Background
  // enable depth testing for drawing
  glEnable(GL_DEPTH_TEST);
  // enable multisampling for smoother drawing
  glEnable(GL_MULTISAMPLE);
  // Now we will create a basic Camera from the graphics library
  // This is a static camera so it only needs to be set once
  // First create Values for the camera position
  ngl::Vec3 from(0,3,7);
  ngl::Vec3 to(0,0,0);
  ngl::Vec3 up(0,1,0);
  m_cam= new ngl::Camera(from,to,up);
  // set the shape using FOV 45 Aspect Ratio based on Width and Height
  // The final two are near and far clipping planes of 0.5 and 10
  m_cam->setShape(45,(float)720.0/576.0,0.5,150);
  // now to load the shader and set the values
  // grab an instance of shader manager
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();

  shader->createShaderProgram("TextureShader");

  // load a frag and vert shaders
  shader->attachShader("SimpleVertex",ngl::VERTEX);
  shader->attachShader("SimpleFragment",ngl::FRAGMENT);
  shader->loadShaderSource("SimpleVertex","shaders/TextureVert.glsl");
  shader->loadShaderSource("SimpleFragment","shaders/TextureFrag.glsl");

  shader->compileShader("SimpleVertex");
  shader->compileShader("SimpleFragment");
  shader->attachShaderToProgram("TextureShader","SimpleVertex");
  shader->attachShaderToProgram("TextureShader","SimpleFragment");


  shader->linkProgramObject("TextureShader");
  shader->use("TextureShader");
  shader->autoRegisterUniforms("TextureShader");
//  shader->registerUniform("TextureShader","MVP");
//  shader->registerUniform("TextureShader","normalMat");
//  shader->registerUniform("TextureShader","MV");
//  shader->registerUniform("TextureShader","rustLoc");

  shader->setShaderParam3f("material.Ka",0.1,0.1,0.1);
  shader->setShaderParam3f("material.Kd",0.8,0.8,0.8);
  // white spec
  shader->setShaderParam3f("material.Ks",1.0,1.0,1.0);
  shader->setShaderParam1f("material.shininess",1000);

  shader->setUniform("light.position",ngl::Vec3(-5,1,3));
  shader->setShaderParam3f("light.La",0.1,0.1,0.1);
  shader->setShaderParam3f("light.Ld",1.0,1.0,1.0);
  shader->setShaderParam3f("light.Ls",0.4,0.4,0.4);
  m_rustStartColour.setRgb(204,173,54);
  shader->setUniform("rustStartColour",ngl::Vec4(0.8,0.68,0.21,1.0));
  m_rustEndColour.setRgb(139,69,19);
  shader->setUniform("rustEndColour",ngl::Vec4(0.54,0.27,0.01,1.0));
  shader->setUniform("drawLattice",false);
  loadBaseTexture("textures/MetalTexture.png");

  loadMesh(1,"textures/testCube.obj");
  DPDModelInit();
  m_text = new ngl::Text(QFont("Arial",14));
  m_text->setScreenSize(width(),height());
  // as re-size is not explicitly called we need to do this.
  glViewport(0,0,width(),height());
  m_timer.start();
  m_fpsTimer =startTimer(0);


}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::loadMatricesToShader()
{
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  ngl::Mat4 MV=m_transformStack.getCurrentTransform().getMatrix()
                *m_mouseGlobalTX*m_cam->getViewMatrix();
  ngl::Mat4 MVP=m_transformStack.getCurrentTransform().getMatrix()
                *m_mouseGlobalTX*m_cam->getVPMatrix();
  ngl::Mat4 normalMat = MV.inverse().transpose();
  shader->setRegisteredUniform("MVP",MVP);
  shader->setRegisteredUniform("MV",MV);
  shader->setRegisteredUniform("normalMat",normalMat);
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::paintGL()
{
  // clear the screen and depth buffer
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 // build our transform stack
  ngl::Transformation trans;
  // Rotation based on the mouse position for our global
  // transform
  ngl::Mat4 rotX;
  ngl::Mat4 rotY;
  // create the rotation matrices
  rotX.rotateX(m_spinXFace);
  rotY.rotateY(m_spinYFace);
  // multiply the rotations
  m_mouseGlobalTX=rotY*rotX;
  // add the translations
  m_mouseGlobalTX.m_m[3][0] = m_modelPos.m_x;
  m_mouseGlobalTX.m_m[3][1] = m_modelPos.m_y;
  m_mouseGlobalTX.m_m[3][2] = m_modelPos.m_z;
  ngl::ShaderLib *shader=ngl::ShaderLib::instance();
  (*shader)["TextureShader"]->use();
  // now we bind back our vertex array object and draw
  glBindVertexArray(m_vaoID);		// select first VAO


  // need to bind the active texture before drawing
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_texId[0]);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D,m_texId[1]);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D,m_texId[2]);
  glPolygonMode(GL_FRONT_AND_BACK,m_polyMode);


        m_transformStack.pushTransform();
        {

       m_transformStack.setPosition(0,0,0);
       loadMatricesToShader();

       glDrawArrays(GL_TRIANGLES, 0,36 );	// draw object
    } // and before a pop
     m_transformStack.popTransform();

	// calculate and draw FPS
	++m_frames;
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	m_text->setColour(1,1,0);
    QString text=QString("Metalic Surfaces Weathering Demo at %2 fps").arg(m_fps);
	m_text->renderText(10,20,text);
    if(m_play){
        text = QString("Playing");
        m_text->setColour(0,255,0);
    }
    else{
        text = QString("Paused");
        m_text->setColour(255,0,0);
    }
    m_text->renderText(10,40,text);
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseMoveEvent (QMouseEvent * _event)
{
  // note the method buttons() is the button state when event was called
  // this is different from button() which is used to check which button was
  // pressed when the mousePress/Release event is generated
  if(m_rotate && _event->buttons() == Qt::LeftButton)
  {
    int diffx=_event->x()-m_origX;
    int diffy=_event->y()-m_origY;
    m_spinXFace += (float) 0.5f * diffy;
    m_spinYFace += (float) 0.5f * diffx;
    m_origX = _event->x();
    m_origY = _event->y();
    updateGL();

  }
        // right mouse translate code
  else if(m_translate && _event->buttons() == Qt::RightButton)
  {
    int diffX = (int)(_event->x() - m_origXPos);
    int diffY = (int)(_event->y() - m_origYPos);
    m_origXPos=_event->x();
    m_origYPos=_event->y();
    m_modelPos.m_x += INCREMENT * diffX;
    m_modelPos.m_y -= INCREMENT * diffY;
    updateGL();

   }
}


//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mousePressEvent ( QMouseEvent * _event)
{
  // this method is called when the mouse button is pressed in this case we
  // store the value where the maouse was clicked (x,y) and set the Rotate flag to true
  if(_event->button() == Qt::LeftButton)
  {
    m_origX = _event->x();
    m_origY = _event->y();
    m_rotate =true;
  }
  // right mouse translate mode
  else if(_event->button() == Qt::RightButton)
  {
    m_origXPos = _event->x();
    m_origYPos = _event->y();
    m_translate=true;
  }

}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::mouseReleaseEvent ( QMouseEvent * _event )
{
  // this event is called when the mouse button is released
  // we then set Rotate to false
  if (_event->button() == Qt::LeftButton)
  {
    m_rotate=false;
  }
        // right mouse translate mode
  if (_event->button() == Qt::RightButton)
  {
    m_translate=false;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void NGLScene::wheelEvent(QWheelEvent *_event)
{

	// check the diff of the wheel position (0 means no change)
	if(_event->delta() > 0)
	{
		m_modelPos.m_z+=ZOOM;
	}
	else if(_event->delta() <0 )
	{
		m_modelPos.m_z-=ZOOM;
	}
    updateGL();
}
//----------------------------------------------------------------------------------------------------------------------

void NGLScene::keyPressEvent(QKeyEvent *_event)
{
  // this method is called every time the main window recives a key event.
  // we then switch on the key value and set the camera in the GLWindow
  switch (_event->key())
  {
  // escape key to quite
  case Qt::Key_Escape : QGuiApplication::exit(EXIT_SUCCESS); break;
  // turn on wirframe rendering
  case Qt::Key_W : m_polyMode=GL_LINE; break;
  // turn off wire frame
  case Qt::Key_S : m_polyMode=GL_FILL; break;
  // show full screen
  case Qt::Key_F : showFullScreen(); break;
  // show windowed
  case Qt::Key_N : showNormal(); break;
  // toggle playing the simulation
  case Qt::Key_Space :
      if(m_play){
          m_play = false;
      }
      else{
          m_play = true;
      }
  break;
  case Qt::Key_R:
      m_play = false;
      clearRust();
  break;
  default : break;
  }
  // finally update the GLWindow and re-draw
  //if (isExposed())
    updateGL();
}
//----------------------------------------------------------------------------------------------------------------------
void NGLScene::timerEvent(QTimerEvent *_event)
{
    if(m_play){
        if(m_DPDModelUpdate){
            if(m_CurrentDPDType==Random){
                DPDModelRandom();
            }
            else{
                DPDModelSeed();
            }
        }
        if(m_randomDepositionModel){
            randomDepModel();
        }
        if(m_randomDepVarModel){
            randomDepVariationModel();
        }
        if(m_BallisticDepModel){
            ballisticDepModel();
        }
    }
    // make active texture 1 and load in new rust data to shader
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D,m_texId[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,m_rustData.width(),m_rustData.height(),0,GL_RGBA,GL_UNSIGNED_BYTE,m_rustData.bits());

    glGenerateMipmap(GL_TEXTURE_2D); //  Allocate the mipmaps
	if(_event->timerId() == m_fpsTimer)
		{
			if( m_timer.elapsed() > 1000.0)
			{
				m_fps=m_frames;
				m_frames=0;
				m_timer.restart();
			}
		 }
			// re-draw GL
    updateGL();
}
