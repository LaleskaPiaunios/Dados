#include "RasterFile.h"
#include "ui_RasterFile.h"
#include "../Utils.h"
#include "../../terralib5/src/terralib/dataaccess/datasource/DataSource.h"
#include "../../terralib5/src/terralib/dataaccess/datasource/DataSourceFactory.h"
#include "../../terralib5/src/terralib/dataaccess/datasource/DataSourceTransactor.h"
#include "../../terralib5/src/terralib/dataaccess/utils/Utils.h"
#include "../../terralib5/src/terralib/geometry/Utils.h"
#include "../../terralib5/src/terralib/raster/PositionIterator.h"
#include "../../terralib5/src/terralib/raster/Raster.h"
#include "../../terralib5/src/terralib/raster/RasterFactory.h"
#include "../../terralib5/src/terralib/raster/RasterProperty.h"
#include "../../terralib5/src/terralib/raster/Utils.h"
#include <QString>
#include <QFileDialog>
#include <QStringList>
#include <QListWidgetItem>
#include <QAbstractItemView>
#include <QLayout>
#include <QLayoutItem>
#include <QMessageBox>
#include "../../terralib5/src/terralib/vp/Utils.h"
#include <../../terralib5/src/terralib/dataaccess/dataset/DataSetTypeConverter.h>

// BOOST
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

ep::qt::RasterFile::RasterFile(QWidget* parent, Qt::WindowFlags f)
: QDialog(parent, f),
m_ui(new Ui::RasterFile)
{
  // add controls
  m_ui->setupUi(this);

  connect(m_ui->PushButtonImage, SIGNAL(clicked()), this, SLOT(onPushButtonImageClicked()));
  connect(m_ui->PushButtonRemoveImage, SIGNAL(clicked()), this, SLOT(onPushButtonRemoveImageClicked()));
  connect(m_ui->PushButtonShape, SIGNAL(clicked()), this, SLOT(onPushButtonShapeClicked()));
  connect(m_ui->PushButtonRemoveShape, SIGNAL(clicked()), this, SLOT(onPushButtonRemoveShapeClicked()));
  connect(m_ui->PushButtonOK, SIGNAL(clicked()), this, SLOT(onPushButtonOKClicked()));
  connect(m_ui->PushButtonOpenImage, SIGNAL(clicked()), this, SLOT(onPushButtonOpenImageClicked()));
  connect(m_ui->PushButtonOpenShape, SIGNAL(clicked()), this, SLOT(onPushButtonOpenShapeClicked()));
}

ep::qt::RasterFile::~RasterFile()
{

}


void ep::qt::RasterFile::onPushButtonImageClicked()
{
  QStringList file = QFileDialog::getOpenFileNames(this, tr("Open File..."),
    QString(), tr("Images (*.tif *.TIF);;"), 0, QFileDialog::DontConfirmOverwrite);

  if (!file.isEmpty())
  {
    for (int i = 0; i < file.count(); i++){

      QListWidgetItem* listItem = new QListWidgetItem(file.at(i));

      if (m_ui->referenceImage->text() != listItem->text()){
        m_ui->listWidgetTif->addItem(listItem);
      }

      else{
        QMessageBox msgBox;
        msgBox.setText("The reference image should not be added to the list.");
        msgBox.exec();
      }

    }
  }
}

void ep::qt::RasterFile::onPushButtonRemoveImageClicked()
{
  QListWidgetItem* itemTif = m_ui->listWidgetTif->currentItem();

  if (itemTif){
    delete itemTif;
  }
}

void ep::qt::RasterFile::onPushButtonShapeClicked()
{
  QStringList file = QFileDialog::getOpenFileNames(this, tr("Open File..."),
    QString(), tr("Images (*.shp *.SHP);;"), 0, QFileDialog::DontConfirmOverwrite);

  if (!file.isEmpty())
  {
    for (int i = 0; i < file.count(); i++){

      QListWidgetItem* listItem = new QListWidgetItem(file.at(i));
      m_ui->listWidgetRaster->addItem(listItem);
    }
  }
}



void ep::qt::RasterFile::onPushButtonRemoveShapeClicked()
{
  QListWidgetItem* itemShp = m_ui->listWidgetRaster->currentItem();

  if (itemShp){
    delete itemShp;
  }
}

void ep::qt::RasterFile::onPushButtonOpenImageClicked()
{
  QString img_ref = QFileDialog::getOpenFileName(this, tr("Open File..."), "", tr("Images (*.tif *.TIF);;"));

  if (!img_ref.isEmpty())
  {
    m_ui->referenceImage->setText(img_ref);
  }
}

void ep::qt::RasterFile::onPushButtonOpenShapeClicked()
{
  QString shp_ref = QFileDialog::getOpenFileName(this, tr("Open Shape..."), "", tr("Images (*.shp *.SHP);;"));

  if (!shp_ref.isEmpty())
  {
    m_ui->referenceShape->setText(shp_ref);
  }
}


void ep::qt::RasterFile::onPushButtonOKClicked()
{
  //Lê  a imagem principal
  te::rst::Raster* inraster = openImage(m_ui->referenceImage->text().toStdString());

  //Lê o dado vetorial da imagem principal
  std::auto_ptr<te::da::DataSource> inDataSource = openShape(m_ui->referenceShape->text().toStdString());

  //DataSets da imagem principal
  std::auto_ptr<te::da::DataSet> inDataset = createDataSetGeometry(inDataSource);
 
  te::gm::Geometry* inGeomColl = dataSetToGeometryCollection(inDataset,inraster);

  //Cria o raster da imagem principal
  te::rst::Raster* outraster = createOutput("D:/Desktop/RESULTADO_NEWEPSG4291/mask.tif", inraster);

  //Copia a imagem pricipal para uma imagem de saida
  te::rst::Copy(*inraster, *outraster);

  //Percorre um ListWidget e recupera seu valore o usa para passar para a próxima imagem
  size_t aux = m_ui->listWidgetTif->count();
  size_t cont = 0;
  //Para cada imagem auxiliar
  for (size_t i = 0; i < aux; ++i)
  {
    //Lê  a imagem principal
    std::auto_ptr<te::rst::Raster> auxRaster(openImage(m_ui->listWidgetTif->item(i)->text().toStdString()));

    //Lê o dado vetorial da imagem principal
    std::auto_ptr<te::da::DataSource> auxDataSource = openShape(m_ui->listWidgetRaster->item(i)->text().toStdString());

    //DataSets da imagem principal
    std::auto_ptr<te::da::DataSet> auxDataset = createDataSetGeometry(auxDataSource);

    te::gm::Geometry* auxGeomColl = dataSetToGeometryCollection(auxDataset, auxRaster.get());

    te::gm::MultiPolygon* m_polygon = createMultipolygonDiff(inGeomColl, auxGeomColl);

    cleanRaster(outraster, inGeomColl, auxRaster, auxGeomColl, m_polygon, cont);

    cont++;
  }
 
}

te::rst::Raster* ep::qt::RasterFile::openImage(const std::string& image)
{

  std::map<std::string, std::string> rinfo;
  rinfo["URI"] = image;

  te::rst::Raster* raster = te::rst::RasterFactory::open(rinfo);

  return raster;
}

std::auto_ptr<te::da::DataSource> ep::qt::RasterFile::openShape(const std::string& shape)
{
  std::map<std::string, std::string> ogrInfo;
  ogrInfo["URI"] = shape;

  std::auto_ptr<te::da::DataSource> ds_pols = te::da::DataSourceFactory::make("OGR");
  ds_pols->setConnectionInfo(ogrInfo);
  ds_pols->open();

  return ds_pols;
}

te::rst::Raster*  ep::qt::RasterFile::createOutput(const std::string& output, te::rst::Raster* inraster)
{
  std::map<std::string, std::string> orinfo;
  orinfo["URI"] = output;

  std::size_t nBands = inraster->getNumberOfBands();

  te::rst::Grid* g = new te::rst::Grid(*inraster->getGrid());

  std::vector<te::rst::BandProperty*> bp;

  for (std::size_t t = 0; t < nBands; ++t)
  {
    bp.push_back(new te::rst::BandProperty(t, te::dt::UCHAR_TYPE));
  }

  te::rst::Raster* outraster = te::rst::RasterFactory::make("GDAL", g, bp, orinfo);

  return outraster;
}

te::gm::MultiPolygon* ep::qt::RasterFile::createMultipolygonDiff(te::gm::Geometry* inGeomColl, te::gm::Geometry* auxGeomColl)
{
  te::gm::Geometry* diff = inGeomColl->difference(auxGeomColl);

  te::gm::MultiPolygon* m_polygonDiff = dynamic_cast<te::gm::MultiPolygon*>(diff);

  return m_polygonDiff;
}

te::gm::MultiPolygon* ep::qt::RasterFile::createMultipolygonInter(te::gm::Geometry* inGeomColl, te::gm::Geometry* oldAuxGeomColl)
{
  te::gm::Geometry* intersect = inGeomColl->intersection(oldAuxGeomColl);

  te::gm::MultiPolygon* m_polygonInter = dynamic_cast<te::gm::MultiPolygon*>(intersect);

  return m_polygonInter;
}

void ep::qt::RasterFile::cleanRaster(te::rst::Raster* outraster, te::gm::Geometry* inGeomColl, std::auto_ptr<te::rst::Raster> auxRaster, te::gm::Geometry* auxGeomColl, te::gm::MultiPolygon* m_polygon, size_t cont)
{

  if (cont == 0)
  {
    copyPixels(auxRaster.get(), m_polygon, outraster);
  }
  
  else if (cont > 0)
  {
    m_polygon = createMultipolygonInter(inGeomColl, auxGeomColl);
    copyPixels(auxRaster.get(), m_polygon, outraster);
  }
}

te::gm::Geometry* ep::qt::RasterFile::dataSetToGeometryCollection(std::auto_ptr<te::da::DataSet> dataSet, te::rst::Raster* raster)
{
  std::size_t geomPos = te::da::GetFirstPropertyPos(dataSet.get(), te::dt::GEOMETRY_TYPE);
  //te::gm::Envelope* geomRasterBox = new te::gm::Envelope();
  te::gm::MultiPolygon* geomCollection = new te::gm::MultiPolygon(0, te::gm::MultiPolygonType, raster->getSRID());


  //Adicionar um MultiPolygon com várias geometrias geomRasterBoxAux

  int cont = 0;
  
  dataSet->moveBeforeFirst();
  while (dataSet->moveNext())
  {
    //Recuperar a geometria de polígono

    std::auto_ptr<te::gm::Geometry> geometry = dataSet->getGeometry(geomPos); //Poligono 

    te::gm::MultiPolygon* m_polygon = dynamic_cast<te::gm::MultiPolygon*>(geometry.get());

    te::gm::Polygon* polygon = dynamic_cast<te::gm::Polygon*>(m_polygon->getGeometryN(0));

    if (polygon->isValid())
    {
      if (geomCollection->isEmpty() == true)
      {
        geomCollection->add(static_cast<te::gm::Polygon*>(polygon->clone()));
      }
      else
      {
        geomCollection = dynamic_cast<te::gm::MultiPolygon*>(geomCollection->Union(polygon));
      }
    }

    else
    {
      cont = cont + 1;
    }

  }

  return geomCollection;
}

std::auto_ptr<te::da::DataSet> ep::qt::RasterFile::createDataSetGeometry(std::auto_ptr<te::da::DataSource> dataSource)
{

  //DataSets
  std::auto_ptr<te::da::DataSourceTransactor> transactor = dataSource->getTransactor();
  std::vector<std::string> datasets = transactor->getDataSetNames();
  std::auto_ptr<te::da::DataSet> dataset = transactor->getDataSet(datasets[0]);

  return dataset;

}

void ep::qt::RasterFile::copyPixels(te::rst::Raster* raster, te::gm::MultiPolygon* m_polygon, te::rst::Raster* outraster)
{

  int geom = m_polygon->getNumGeometries();
  for (std::size_t i = 0; i < geom; i++)
  {
    te::gm::Polygon* polygon = dynamic_cast<te::gm::Polygon*>(m_polygon->getGeometryN(i));
    polygon->setSRID(m_polygon->getSRID());
    copyPixels(raster, polygon, outraster);
  }

}

void ep::qt::RasterFile::copyPixels(te::rst::Raster* raster, te::gm::Polygon* polygon, te::rst::Raster* outraster)
{
  std::auto_ptr<te::gm::Geometry> geomRasterBox(te::gm::GetGeomFromEnvelope(raster->getExtent(),raster->getSRID()));
  std::size_t nBands = raster->getNumberOfBands();

  if (polygon && polygon->intersects(geomRasterBox.get()))
  {
    te::rst::PolygonIterator<double> it = te::rst::PolygonIterator<double>::begin(raster, polygon);
    te::rst::PolygonIterator<double> itend = te::rst::PolygonIterator<double>::end(raster, polygon);

    while (it != itend)
    {
      te::gm::Coord2D coordGeoIn = raster->getGrid()->gridToGeo(it.getColumn(), it.getRow());
      te::gm::Coord2D coordGridOut = outraster->getGrid()->geoToGrid(coordGeoIn.getX(), coordGeoIn.getY());

      //Preencher a área do polígono na imagem de saída com pixels limpos da imagem auxiliar
      for (std::size_t band = 0; band < nBands; ++band)
      {
        double value = 0.;
        raster->getValue(it.getColumn(), it.getRow(), value, band);
        outraster->setValue(coordGridOut.getX(), coordGridOut.getY(), value, band);
      }

      ++it;
    }

  }
}

te::rst::Raster* ep::qt::RasterFile::createOutputShapeFile(te::gm::Polygon* polygon)
{

}