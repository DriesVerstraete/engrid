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
#include "converttopolymesh.h"
#include "polymesh.h"
#include "guimainwindow.h"

ConvertToPolyMesh::ConvertToPolyMesh()
{
  m_Optimise = false;
  m_SplitCells = false;
  m_SplitFaces = false;
  m_PullInFactor = 0.0;
}

void ConvertToPolyMesh::operate()
{
  QList<VolumeDefinition> vols = mainWindow()->getAllVols();
  if (vols.size() > 1) {
    EG_ERR_RETURN("can only handle grids with a single volume at the moment");
  }
  vols.first().setVC(0);
  mainWindow()->setAllVols(vols);

  PolyMesh pmesh(m_Grid, true, m_PullInFactor, m_Optimise, m_SplitFaces, m_SplitCells);

  // count the number of boundary faces in the polygonal mesh
  int num_boundary_faces = 0;
  for (int i = 0; i < pmesh.numFaces(); ++i) {
    if (pmesh.boundaryCode(i) != 0) {
      if (pmesh.neighbour(i) != -1) {
        EG_BUG;
      }
      ++num_boundary_faces;
    }
  }

  EG_VTKSP(vtkUnstructuredGrid, grid);
  EgVtkObject::allocateGrid(grid, pmesh.numPolyCells() + num_boundary_faces, pmesh.totalNumNodes());

  // copy all nodes to the new grid
  for (vtkIdType id_node = 0; id_node < pmesh.totalNumNodes(); ++id_node) {
    vec3_t x = pmesh.nodeVector(id_node);
    grid->GetPoints()->SetPoint(id_node, x.data());
  }

  // copy all polyhedral cells to the new grid
  EG_VTKDCC(vtkIntArray, cell_code, grid, "cell_code");
  for (int i = 0; i < pmesh.numCells(); ++i) {
    vtkIdType num_ids = 1;
    for (int j = 0; j < pmesh.numFacesOfPCell(i); ++j) {
      num_ids += 1 + pmesh.numNodes(pmesh.pcell2Face(i, j));
    }
    EG_VTKSP(vtkIdList, ptids);
    ptids->SetNumberOfIds(num_ids);
    vtkIdType i_id = 0;
    ptids->SetId(i_id++, pmesh.numFacesOfPCell(i));
    for (int j = 0; j < pmesh.numFacesOfPCell(i); ++j) {
      int i_face = pmesh.pcell2Face(i, j);
      ptids->SetId(i_id++, pmesh.numNodes(i_face));
      for (int k = 0; k < pmesh.numNodes(i_face); ++k) {
        if (pmesh.owner(i_face) == i) {
          ptids->SetId(i_id++, pmesh.nodeIndex(i_face, k));
        } else {
          ptids->SetId(i_id++, pmesh.nodeIndex(i_face, pmesh.numNodes(i_face) - 1 - k));
        }
      }
    }
    vtkIdType id_cell = grid->InsertNextCell(VTK_POLYHEDRON, ptids);
    cell_code->SetValue(id_cell, 0);
  }

  // copy all boundary faces to the new grid
  for (int i = 0; i < pmesh.numFaces(); ++i) {
    if (pmesh.boundaryCode(i) != 0) {
      EG_VTKSP(vtkIdList, ptids);
      ptids->SetNumberOfIds(pmesh.numNodes(i));
      for (int j = 0; j < pmesh.numNodes(i); ++j) {
        ptids->SetId(j, pmesh.nodeIndex(i, j));
      }
      vtkIdType id_cell = grid->InsertNextCell(VTK_POLYGON, ptids);
      cell_code->SetValue(id_cell, pmesh.boundaryCode(i));
    }
  }

  // compute surface integrals to detect open cells
  {
    MeshPartition part(grid, true);
    EG_VTKDCC(vtkDoubleArray, cell_mesh_quality, grid, "cell_mesh_quality");
    double max_err = 0;
    double min_dist =  1e99;
    double max_dist = -1e99;
    double average_dist = 0;
    int N = 0;
    for (vtkIdType id_cell = 0; id_cell < grid->GetNumberOfCells(); ++id_cell) {
      if (isVolume(id_cell, grid)) {
        vec3_t surf_int(0,0,0);
        double A = 0;
        vec3_t x = cellCentre(grid, id_cell);
        for (int i_face = 0; i_face < part.c2cGSize(id_cell); ++i_face) {
          vec3_t n = getNormalOfCell(grid, id_cell, i_face);
          surf_int += n;
          A += n.abs();
          vec3_t xf = getCentreOfCellFace(grid, id_cell, i_face);
          n.normalise();
          double dist = (xf - x)*n;
          max_dist = max(max_dist, dist);
          min_dist = min(min_dist, dist);
          average_dist += dist;
          ++N;
        }
        max_err = max(max_err, surf_int.abs()/A);
      }
    }
    average_dist /= N;
    cout << "maximal cell error: " << max_err << endl;
    cout << "maximal face distance: " << max_dist << endl;
    cout << "minimal face distance: " << min_dist << endl;
    cout << "average face distance: " << average_dist << endl;
  }

  // replace the original mesh
  makeCopy(grid, m_Grid);
  m_Grid->Modified();
}
