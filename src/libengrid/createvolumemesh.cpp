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
#include "createvolumemesh.h"
#include "deletetetras.h"
#include "guimainwindow.h"
#include "updatedesiredmeshdensity.h"
#include <vtkXMLUnstructuredGridWriter.h>

CreateVolumeMesh::CreateVolumeMesh()
{
  EG_TYPENAME;
  maxh     = 1e99;
  fineness = 0.0;
}

void CreateVolumeMesh::setTraceCells(const QVector<vtkIdType> &cells)
{
  trace_cells.resize(cells.size()); 
  qCopy(cells.begin(), cells.end(), trace_cells.begin()); 
}

void CreateVolumeMesh::getTraceCells(QVector<vtkIdType> &cells)
{
  cells.resize(trace_cells.size()); 
  qCopy(trace_cells.begin(), trace_cells.end(), cells.begin()); 
}

void CreateVolumeMesh::prepare()
{
  using namespace nglib;
  DeleteTetras del;
  del.setGrid(m_Grid);
  del.setAllCells();
  del();
  writeGrid(m_Grid, "tringles");
  QVector<vtkIdType> cells, nodes;
  QVector<int>       _cells, _nodes;
  QVector<QVector< int > > c2c;
  QVector<QVector<int> > n2c;
  getAllCells(cells, m_Grid);
  createCellMapping(cells, _cells, m_Grid);
  getNodesFromCells(cells, nodes, m_Grid);
  createNodeMapping(nodes, _nodes, m_Grid);
  createCellToCell(cells, c2c, m_Grid);
  createNodeToCell(cells, nodes, _nodes, n2c, m_Grid);
  QList<QVector<vtkIdType> > ex_tri;
  int N1 = 0;
  int N2 = 0;
  int N3 = 0;
  int N4 = 0;
  foreach (vtkIdType id_cell, cells) {
    vtkIdType type_cell = m_Grid->GetCellType(id_cell);
    vtkIdType *pts, N_pts;
    m_Grid->GetCellPoints(id_cell, N_pts, pts);
    QVector<vtkIdType> T(3);
    if (type_cell == VTK_TRIANGLE) {
      if (findVolumeCell(m_Grid, id_cell, _nodes, cells, _cells, n2c) == -1) {
        T[0] = pts[0];
        T[1] = pts[1];
        T[2] = pts[2];
        ex_tri.append(T);
        ++N4;
      }
    } else if (type_cell == VTK_QUAD) {
      if (findVolumeCell(m_Grid, id_cell, _nodes, cells, _cells, n2c) == -1) {
        //EG_BUG;
        T[0] = pts[0];
        T[1] = pts[1];
        T[2] = pts[2];
        ex_tri.append(T);
        T[0] = pts[2];
        T[1] = pts[3];
        T[2] = pts[0];
        ex_tri.append(T);
        ++N3;
      }
    } else if (type_cell == VTK_WEDGE) {
      if (c2c[id_cell][0] == -1) {
        T[0] = pts[0];
        T[1] = pts[2];
        T[2] = pts[1];
        ex_tri.append(T);
        ++N2;
      }
      if (c2c[id_cell][1] == -1) {
        T[0] = pts[3];
        T[1] = pts[4];
        T[2] = pts[5];
        ex_tri.append(T);
        ++N2;
      }
      if (c2c[id_cell][2] == -1) {
        T[0] = pts[0];
        T[1] = pts[1];
        T[2] = pts[4];
        ex_tri.append(T);
        T[0] = pts[0];
        T[1] = pts[4];
        T[2] = pts[3];
        ex_tri.append(T);
        ++N1;
      }
      if (c2c[id_cell][3] == -1) {
        T[0] = pts[4];
        T[1] = pts[1];
        T[2] = pts[2];
        ex_tri.append(T);
        T[0] = pts[4];
        T[1] = pts[2];
        T[2] = pts[5];
        ex_tri.append(T);
        ++N1;
      }
      if (c2c[id_cell][4] == -1) {
        T[0] = pts[0];
        T[1] = pts[3];
        T[2] = pts[2];
        ex_tri.append(T);
        T[0] = pts[3];
        T[1] = pts[5];
        T[2] = pts[2];
        ex_tri.append(T);
        ++N1;
      }
    } else {
      EG_BUG;
    }
  }
  cout << "*********************************************************************" << endl;
  cout << "prism quads     : " << N1 << endl;
  cout << "prism triangles : " << N2 << endl;
  cout << "stray quads     : " << N3 << endl;
  cout << "stray triangles : " << N4 << endl;
  cout << "*********************************************************************" << endl;
  tri.resize(ex_tri.size());
  qCopy(ex_tri.begin(), ex_tri.end(), tri.begin());
  add_to_ng.fill(false, m_Grid->GetNumberOfPoints());
  foreach (QVector<vtkIdType> T, tri) {
    add_to_ng[T[0]] = true;
    add_to_ng[T[1]] = true;
    add_to_ng[T[2]] = true;
  }
  num_nodes_to_add = 0;
  num_old_nodes = 0;
  for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
    if (add_to_ng[id_node]) {
      ++num_nodes_to_add;
    } else {
      ++num_old_nodes;
    }
  }  
  old2tri.fill(-1, m_Grid->GetNumberOfPoints());
  m_NumTriangles = 0;
  for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
    if (add_to_ng[id_node]) {
      old2tri[id_node] = m_NumTriangles;
      ++m_NumTriangles;
    }
  }
  //writeDebugInfo();
}

void CreateVolumeMesh::writeDebugInfo()
{
  {
    EG_VTKSP(vtkUnstructuredGrid,tri_grid);
    allocateGrid(tri_grid, tri.size(), m_NumTriangles);
    for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
      if (add_to_ng[id_node]) {
        vec3_t x;
        m_Grid->GetPoint(id_node, x.data());
        tri_grid->GetPoints()->SetPoint(old2tri[id_node], x.data());
        copyNodeData(m_Grid, id_node, tri_grid, old2tri[id_node]);
      }
    }
    foreach (QVector<vtkIdType> T, tri) {
      vtkIdType pts[3];
      pts[0] = old2tri[T[0]];
      pts[1] = old2tri[T[1]];
      pts[2] = old2tri[T[2]];
      tri_grid->InsertNextCell(VTK_TRIANGLE, 3, pts);
    }
  }
}

void CreateVolumeMesh::computeMeshDensity()
{
  boxes.clear();
  QString buffer = GuiMainWindow::pointer()->getXmlSection("engrid/surface/settings").replace("\n", " ");
  if (!buffer.isEmpty()) {
    QTextStream in(&buffer, QIODevice::ReadOnly);
    in >> m_MaxEdgeLength;
    in >> m_MinEdgeLength;
    in >> m_GrowthFactor;
  } else {
    m_MaxEdgeLength = 1000.0;
    m_MinEdgeLength = 0.0;
    m_GrowthFactor = 1.5;
  }
  m_ELSManager.read();
  QVector<double> H(m_Grid->GetNumberOfPoints(), m_MaxEdgeLength);

  QVector<bool> fixed(m_Grid->GetNumberOfPoints(), false);
  double H_min = 1e99;
  vtkIdType id_min = -1;
  for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
    bool volume_only = true;
    for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
      if (isSurface(m_Part.n2cGG(id_node, i), m_Grid)) {
        volume_only = false;
      }
      if (!volume_only) {
        fixed[id_node] = true;
        H[id_node] = 0;
        int N = 0;
        vec3_t xi;
        m_Grid->GetPoint(id_node, xi.data());
        for (int j = 0; j < m_Part.n2nGSize(id_node); ++j) {
          if (m_Part.n2nGG(id_node, j)) {
            vec3_t xj;
            m_Grid->GetPoint(m_Part.n2nGG(id_node, j), xj.data());
            H[id_node] += (xi-xj).abs();
            ++N;
          }
        }
        if (N < 2) {
          EG_BUG;
        }
        H[id_node] /= N;
		if (H[id_node] < 0) {
	      EG_BUG;
		}
        if (H[id_node] < H_min) {
          id_min = id_node;
          H_min = H[id_node];
        }
      }
    }
  }
  if (id_min < 0) {
    EG_BUG;
  }

  for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
    vec3_t x;
    m_Grid->GetPoint(id_node, x.data());
    double cl_src = m_ELSManager.minEdgeLength(x);
    if (cl_src > 0) {
      if (cl_src < H[id_node]) {
        H[id_node] = cl_src;
      }
    }
  }

  QVector<bool> marked(m_Grid->GetNumberOfPoints(), false);
  marked[id_min] = true;
  bool done = false;
  while (!done) {
    done = true;
    for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
      if (marked[id_node] && H[id_node] <= H_min) {
        vec3_t x1;
        m_Grid->GetPoint(id_node, x1.data());
        for (int i = 0; i < m_Part.n2nGSize(id_node); ++i) {
          vtkIdType id_neigh = m_Part.n2nGG(id_node,i);
          if (!marked[id_neigh]) {
            vec3_t x2;
            m_Grid->GetPoint(id_neigh, x2.data());
            double dist = (x1 - x2).abs();
            double h = H[id_node] + (m_GrowthFactor - 1)*dist;
            if (h < 0) {
              EG_BUG;
            }
            H[id_neigh] = min(H[id_neigh], h);
            marked[id_neigh] = true;
            //H[id_neigh] += 1.0*H[id_node];
            done = false;
          }
        }
      }
    }
    H_min *= m_GrowthFactor;
  }

  for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
    vec3_t x;
    m_Grid->GetPoint(id_node, x.data());
    double cl_src = m_ELSManager.minEdgeLength(x);
    if (cl_src > 0) {
      if (cl_src < H[id_node]) {
        H[id_node] = cl_src;
      }
    }
  }

  for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
    vec3_t x1, x2;
    m_Grid->GetPoint(id_node, x1.data());
    x2 = x1;
    for (int j = 0; j < m_Part.n2nGSize(id_node); ++j) {
      vec3_t xj;
      m_Grid->GetPoint(m_Part.n2nGG(id_node, j), xj.data());
      for (int k = 0; k < 3; ++k) {
        x1[k] = min(xj[k], x1[k]);
        x2[k] = max(xj[k], x2[k]);
      }
    }
    box_t B;
    B.x1 =x1;
    B.x2 =x2;
    if (H[id_node] < 0) {
      EG_BUG;
    }
    B.h = H[id_node];
    boxes.append(B);
  }
}


void CreateVolumeMesh::operate()
{
  using namespace nglib;
  setAllCells();
  if (m_Grid->GetNumberOfCells() == 0) {
    EG_ERR_RETURN("The grid appears to be empty.");
  }
  //nglib::Ng_Init();
  Ng_Init();
  Ng_Meshing_Parameters mp;
  mp.fineness = fineness;
  //mp.secondorder = 0;
  Ng_Mesh *mesh = Ng_NewMesh();
  computeMeshDensity();
  mp.maxh = m_MaxEdgeLength;
  mp.minh = m_MinEdgeLength;
  mp.grading = 1;
  prepare();
  QVector<vtkIdType> ng2eg(num_nodes_to_add+1);
  QVector<vtkIdType> eg2ng(m_Grid->GetNumberOfPoints());
  {
    int N = 1;
    for (vtkIdType id_node = 0; id_node < m_Grid->GetNumberOfPoints(); ++id_node) {
      if (add_to_ng[id_node]) {
        vec3_t x;
        m_Grid->GetPoints()->GetPoint(id_node, x.data());
        Ng_AddPoint(mesh, x.data());
        ng2eg[N] = id_node;
        eg2ng[id_node] = N;
        ++N;
      }
    }
  }
  
  foreach (QVector<vtkIdType> T, tri) {
    int trig[3];
    for (int i = 0; i < 3; ++i) {
      trig[i] = eg2ng[T[i]];
    }
    Ng_AddSurfaceElement(mesh, NG_TRIG, trig);
  }
  Ng_Result res;
  try {
    int box_counter = 0;
    foreach (box_t B, boxes) {
      Ng_RestrictMeshSizeBox(mesh, B.x1.data(), B.x2.data(), B.h);
      ++box_counter;
    }
    GuiMainWindow::pointer()->setSystemOutput();
    writeDebugInfo();
    res = Ng_GenerateVolumeMesh (mesh, &mp);
    GuiMainWindow::pointer()->setLogFileOutput();
  } catch (netgen::NgException ng_err) {
    GuiMainWindow::pointer()->setLogFileOutput();
    writeDebugInfo();
    Error err;
    QString msg = "Netgen stopped with the following error:\n";
    msg += ng_err.What().c_str();
    msg += "\n\nDebug information has been saved to:\n" + mainWindow()->getCwd();
    err.setType(Error::ExitOperation);
    err.setText(msg);
    throw err;
  }
  if (res == NG_OK) {
    int Npoints_ng = Ng_GetNP(mesh);
    int Ncells_ng  = Ng_GetNE(mesh);
    int Nscells_ng = Ng_GetNSE(mesh);
    EG_VTKSP(vtkUnstructuredGrid,vol_grid);
    allocateGrid(vol_grid, m_Grid->GetNumberOfCells() + Ncells_ng, Npoints_ng + num_old_nodes);
    vtkIdType new_point = 0;
    
    // copy existing points
    QVector<vtkIdType> old2new(m_Grid->GetNumberOfPoints(), -1);
    for (vtkIdType id_point = 0; id_point < m_Grid->GetNumberOfPoints(); ++id_point) {
      vec3_t x;
      m_Grid->GetPoints()->GetPoint(id_point, x.data());
      vol_grid->GetPoints()->SetPoint(new_point, x.data());
      copyNodeData(m_Grid, id_point, vol_grid, new_point);
      old2new[id_point] = new_point;
      ++new_point;
    }
    
    // mark all surface nodes coming from NETGEN
    QVector<bool> ng_surf_node(Npoints_ng + 1, false);
    for (int i = 1; i <= Nscells_ng; ++i) {
      int pts[8];
      Ng_Surface_Element_Type ng_type;
      ng_type = Ng_GetSurfaceElement(mesh, i, pts);
      int N = 0;
      if (ng_type == NG_TRIG) {
        N = 3;
      } else if (ng_type == NG_QUAD) {
        N = 4;
      } else {
        EG_BUG;
      }
      for (int j = 0; j < N; ++j) {
        ng_surf_node[pts[j]] = true;
      }
    }
    
    // add new points from NETGEN
    QVector<vtkIdType> ng2new(Npoints_ng+1, -1);
    for (int i = 1; i <= Npoints_ng; ++i) {
      if (!ng_surf_node[i]) {
        vec3_t x;
        Ng_GetPoint(mesh, i, x.data());
        vol_grid->GetPoints()->SetPoint(new_point, x.data());
        ng2new[i] = new_point;
        ++new_point;
      }
    }
    
    // copy existing cells
    QVector<vtkIdType> old2new_cell(m_Grid->GetNumberOfCells(), -1);
    for (vtkIdType id_cell = 0; id_cell < m_Grid->GetNumberOfCells(); ++id_cell) {
      bool ins_cell = false;
      vtkIdType type_cell = m_Grid->GetCellType(id_cell);
      if (type_cell == VTK_TRIANGLE) ins_cell = true;
      if (type_cell == VTK_QUAD)     ins_cell = true;
      if (type_cell == VTK_WEDGE)    ins_cell = true;
      if (ins_cell) {
        vtkIdType N_pts, *pts;
        m_Grid->GetCellPoints(id_cell, N_pts, pts);
        for (int i = 0; i < N_pts; ++i) {
          pts[i] = old2new[pts[i]];
          if (pts[i] == -1) {
            EG_BUG;
          }
        }
        vtkIdType id_new = vol_grid->InsertNextCell(type_cell, N_pts, pts);
        copyCellData(m_Grid, id_cell, vol_grid, id_new);
        old2new_cell[id_cell] = id_new;
      }
    }
    
    // add new cells
    vtkIdType id_new_cell;
    for (vtkIdType cellId = 0; cellId < Ncells_ng; ++cellId) {
      int       pts[8];
      vtkIdType new_pts[4];
      for (int i = 0; i < 8; ++i) {
        pts[i] = 0;
      }
      Ng_Volume_Element_Type ng_type;
      ng_type = Ng_GetVolumeElement(mesh, cellId + 1, pts);
      if (ng_type != NG_TET) {
        EG_BUG;
      }
      for (int i = 0; i < 4; ++i) {
        if (!ng_surf_node[pts[i]]) {
          new_pts[i] = ng2new[pts[i]];
        } else {
          new_pts[i] = ng2eg[pts[i]];
        }
      }
      if (ng_type == NG_TET) {
        vtkIdType tet[4];
        tet[0] = new_pts[0];
        tet[1] = new_pts[1];
        tet[2] = new_pts[3];
        tet[3] = new_pts[2];
        id_new_cell = vol_grid->InsertNextCell(VTK_TETRA, 4, tet);
      } else if (ng_type == NG_PYRAMID) {
        EG_ERR_RETURN("pyramids cannot be handled yet");
      } else if (ng_type == NG_PRISM) {
        EG_ERR_RETURN("prisms cannot be handled yet");
      } else {
        EG_ERR_RETURN("bug encountered");
      }
    }
    makeCopy(vol_grid, m_Grid);
    for (int i = 0; i < trace_cells.size(); ++i) {
      if (old2new_cell[trace_cells[i]] == -1) {
        EG_BUG;
      }
      trace_cells[i] = old2new_cell[trace_cells[i]];
    }
  } else {
    Error err;
    QString msg = "NETGEN did not succeed.\nPlease check if the surface mesh is oriented correctly";
    msg += " (red edges on the outside of the domain)";
    err.setType(Error::ExitOperation);
    err.setText(msg);
    throw err;
  }
  Ng_DeleteMesh(mesh);
  Ng_Exit();
  cout << "\n\nNETGEN call finished" << endl;
  cout << endl;
}

