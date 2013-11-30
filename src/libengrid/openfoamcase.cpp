// 
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +                                                                      +
// + This file is part of enGrid.                                         +
// +                                                                      +
// + Copyright 2008-2013 enGits GmbH                                      +
// +                                                                      +
// + enGrid is free software: you can redistribute it and/or modify       +
// + it under the terms of the GNU General Public License as published by +
// + the Free Software Foundation, either version 3 of the License, or    +
// + (at your option) any later version.                                  +
// +                                                                      +
// + enGrid is distributed in the hope that it will be useful,            +
// + but WITHOUT ANY WARRANTY; without even the implied warranty of       +
// + MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        +
// + GNU General Public License for more details.                         +
// +                                                                      +
// + You should have received a copy of the GNU General Public License    +
// + along with enGrid. If not, see <http://www.gnu.org/licenses/>.       +
// +                                                                      +
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// 
#include "openfoamcase.h"

#include "filetemplate.h"
#include "guimainwindow.h"

#include <QtDebug>

#include <iostream>
using namespace std;

OpenFOAMcase::OpenFOAMcase()
{
}

///\todo Finish this by adding decomposeParDict creation and calling writeMpiParameters from operate
void OpenFOAMcase::writeMpiParameters()
{
  QString hostfile_text = GuiMainWindow::pointer()->getXmlSection( "solver/general/host_weight_list" );
  
  QVector <QString> host;
  QVector <QString> weight;
  
  QStringList host_weight_list = hostfile_text.split(",");
  foreach(QString host_weight, host_weight_list) {
    if(!host_weight.isEmpty()){
      QStringList values = host_weight.split(":");
      qWarning()<<"values="<<values;
      host.push_back(values[0].trimmed());
      weight.push_back(values[1].trimmed());
    }
  }
  
  // create the hostfile
  QFileInfo fileinfo( getFileName() + "/" + "hostfile.txt" );
  QFile hostfile( fileinfo.filePath() );
  if (!hostfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    try {
      EG_ERR_RETURN( "ERROR: Failed to open file " + fileinfo.filePath() );
    } catch ( Error err ) {
      err.display();
    }
  }
  QTextStream out( &hostfile );
  for(int i = 0; i < host.size(); i++) {
    out << host[i] << endl;
  }
  hostfile.close();
  
}

void OpenFOAMcase::writeSolverParameters()
{
  int idx = GuiMainWindow::pointer()->getXmlSection( "solver/general/solver_type" ).toInt();

  QFileInfo solvers_fileinfo;
  solvers_fileinfo.setFile( ":/resources/solvers/solvers.txt" );
  QFile file( solvers_fileinfo.filePath() );
  if ( !file.exists() ) {
    qDebug() << "ERROR: " << solvers_fileinfo.filePath() << " not found.";
    EG_BUG;
  }
  if ( !file.open( QIODevice::ReadOnly | QIODevice::Text ) ) {
    qDebug() << "ERROR:  Failed to open file " << solvers_fileinfo.filePath();
    EG_BUG;
  }
  QTextStream text_stream( &file );
  QString intext = text_stream.readAll();
  file.close();

  QStringList page_list = intext.split( "=" );
  QString page = page_list[idx];
  QString title;
  QString section;
  QString binary;
  QVector <QString> files;
  QStringList variable_list = page.split( ";" );
  foreach( QString variable, variable_list ) {
    QStringList name_value = variable.split( ":" );
    if ( name_value[0].trimmed() == "title" ) title = name_value[1].trimmed();
    if ( name_value[0].trimmed() == "section" ) section = name_value[1].trimmed();
    if ( name_value[0].trimmed() == "binary" ) binary = name_value[1].trimmed();
    if ( name_value[0].trimmed() == "files" ) {
      QStringList file_list = name_value[1].split( "," );
      foreach( QString file, file_list ) {
        files.push_back( file.trimmed() );
      }
    }
    setFoamVersion(title.split(' ').last().trimmed());
  }

  for ( int i = 0; i < files.size(); i++ ) {
    FileTemplate file_template( ":/resources/solvers/" + section + "/" + files[i], section );
    QFileInfo fileinfo_destination( getFileName() + "/" + files[i] );
    QDir destination_dir = fileinfo_destination.dir();
    QString destination = destination_dir.absolutePath() + "/" + fileinfo_destination.completeBaseName();
    if ( !destination_dir.mkpath( destination_dir.absolutePath() ) ) {
      EG_ERR_RETURN( "ERROR: Could not create directory \n" + destination_dir.absolutePath() );
    }
    qDebug() << "Writing to " << destination;
    file_template.exportToOpenFOAM( destination );
  }
}

void OpenFOAMcase::upateVarFile(QString file_name, QString bc_txt)
{
  QFile file(getFileName() + "/0/" + file_name);
  if (file.exists()) {
    QString buffer;
    {
      file.open(QIODevice::ReadOnly);
      QTextStream f(&file);
      buffer = f.readAll();
      file.close();
    }
    buffer = buffer.replace("$$$", bc_txt);
    {
      file.open(QIODevice::WriteOnly);
      QTextStream f(&file);
      f << buffer << "\n";
      file.close();
    }
  }
}

void OpenFOAMcase::writeBoundaryConditions()
{
  QVector<int> bcs;
  EG_VTKDCC(vtkIntArray, cell_code, m_Grid, "cell_code");
  GuiMainWindow::pointer()->getAllBoundaryCodes(bcs);
  QString U_buffer = "";
  QString p_buffer = "";
  QString T_buffer = "";
  QString k_buffer = "";
  QString epsilon_buffer = "";
  QString omega_buffer = "";
  QString nut_buffer = "";
  foreach (int bc, bcs) {
    BoundaryCondition BC = GuiMainWindow::pointer()->getBC(bc);
    if (!GuiMainWindow::pointer()->physicalTypeDefined(BC.getType())) {
      QString msg;
      msg.setNum(bc);
      msg = "boundary code " + msg + " has not been properly defined";
      EG_ERR_RETURN(msg);
    }
    vec3_t n(0,0,0);
    for (vtkIdType id_cell = 0; id_cell < m_Grid->GetNumberOfCells(); ++id_cell) {
      if (isSurface(id_cell, m_Grid)) {
        if (cell_code->GetValue(id_cell) == bc) {
          n += GeometryTools::cellNormal(m_Grid, id_cell);
        }
      }
    }
    n.normalise();
    n *= -1;
    PhysicalBoundaryCondition PBC = GuiMainWindow::pointer()->getPhysicalBoundaryCondition(BC.getType());
    U_buffer       += "    " + BC.getName() + "\n    {\n" + PBC.getFoamU(getFoamVersion(), n)    + "    }\n";
    p_buffer       += "    " + BC.getName() + "\n    {\n" + PBC.getFoamP(getFoamVersion())       + "    }\n";
    T_buffer       += "    " + BC.getName() + "\n    {\n" + PBC.getFoamT(getFoamVersion())       + "    }\n";
    k_buffer       += "    " + BC.getName() + "\n    {\n" + PBC.getFoamK(getFoamVersion())       + "    }\n";
    epsilon_buffer += "    " + BC.getName() + "\n    {\n" + PBC.getFoamEpsilon(getFoamVersion()) + "    }\n";
    omega_buffer   += "    " + BC.getName() + "\n    {\n" + PBC.getFoamOmega(getFoamVersion())   + "    }\n";
    nut_buffer     += "    " + BC.getName() + "\n    {\n" + PBC.getFoamNut(getFoamVersion())     + "    }\n";
  }
  upateVarFile("U", U_buffer);
  upateVarFile("p", p_buffer);
  upateVarFile("T", T_buffer);
  upateVarFile("k", k_buffer);
  upateVarFile("epsilon", epsilon_buffer);
  upateVarFile("omega", omega_buffer);
  upateVarFile("nut", nut_buffer);
}

void OpenFOAMcase::operate()
{
  try {
    if ( getFileName() == "" ) {
      readOutputDirectory();
    }
    if (isValid()) {
      writeSolverParameters();
      bool has_volume = false;
      for (vtkIdType id_cell = 0; id_cell < m_Grid->GetNumberOfCells(); ++id_cell) {
        if (isVolume(id_cell, m_Grid)) {
          has_volume = true;
        }
      }
      setFixedFileName(getFileName());
      FoamWriter::operate();
      QFileInfo file_info(getFileName());
      QFile file(getFileName() + "/" + file_info.baseName() + ".foam");
      file.open(QIODevice::WriteOnly);
      writeBoundaryConditions();
    }
  }
  catch ( Error err ) {
    err.display();
  }
}
