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
#ifndef createvolumemesh_H
#define createvolumemesh_H

class CreateVolumeMesh;

#include "operation.h"
#include "edgelengthsourcemanager.h"

namespace nglib {
  #include <nglib.h>
}

#include <ngexception.hpp>

class CreateVolumeMesh : public Operation
{
  
private: // attributes
  
  double maxh;
  double fineness;
  double m_GrowthFactor;
  double m_MinEdgeLength;
  double m_MaxEdgeLength;
  QVector<QVector<vtkIdType> > tri;
  QVector<bool> add_to_ng;
  int num_nodes_to_add;
  int num_old_nodes;
  QVector<vtkIdType> trace_cells;
  QVector<vtkIdType> old2tri;
  int m_NumTriangles;
  EdgeLengthSourceManager m_ELSManager;



  struct box_t {
    vec3_t x1, x2;
    double h;
  };
  
  struct point_t {
    vec3_t x;
    double h;
  };
  
  QList<box_t> boxes;
  
private: // methods
  
  void computeMeshDensity();
  void prepare();
  void writeDebugInfo();
  
protected: // methods
  
  virtual void operate();
  
public: // methods
  
  CreateVolumeMesh();
  void setMaxH(double h) { maxh = h; }
  void setTraceCells(const QVector<vtkIdType> &cells);
  void getTraceCells(QVector<vtkIdType> &cells);
  
};

#endif
