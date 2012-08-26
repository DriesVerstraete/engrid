// 
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +                                                                      +
// + This file is part of enGrid.                                         +
// +                                                                      +
// + Copyright 2008-2012 enGits GmbH                                     +
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
#include "laplacesmoother.h"
#include <vtkCellLocator.h>
#include <vtkCharArray.h>
#include <vtkGenericCell.h>

#include "guimainwindow.h"
#include "globalnodegraphinterface.h"

using namespace GeometryTools;

LaplaceSmoother::LaplaceSmoother() : SurfaceOperation()
{
  DebugLevel = 0;
  setQuickSave(true);
  m_UseProjection = true;
//   m_UseNormalCorrection = false;
  getSet("surface meshing", "under relaxation for smoothing", 0.5, m_UnderRelaxation);
  getSet("surface meshing", "feature magic", 0.0, m_FeatureMagic);
  getSet("surface meshing", "smoothing limiter", 1.0, m_Limit);
  getSet("surface meshing", "use uniform smoothing", false, m_UniformSnapPoints);
  m_Limit = min(1.0, max(0.0, m_Limit));
  m_NoCheck = false;
  m_ProjectionIterations = 50;
  m_AllowedCellTypes.clear();
  m_AllowedCellTypes.insert(VTK_TRIANGLE);
}

bool LaplaceSmoother::setNewPosition(vtkIdType id_node, vec3_t x_new)
{
  using namespace GeometryTools;

  vec3_t x_old;
  m_Grid->GetPoint(id_node, x_old.data());
  m_Grid->GetPoints()->SetPoint(id_node, x_new.data());
  
  bool move = true;
  if(m_NoCheck) return move;
  
  // compute the extrusion vector to compute the tetrahedrons for volume checking
  // start with an average of all adjacent cell normals and count the number of
  // adjacent boundary codes (for one boundary code an alternative vector can be
  // computed with the help of a SurfaceProjection
  //
  vec3_t n(0,0,0);  
  QVector<vec3_t> cell_normals(m_Part.n2cGSize(id_node));
  QSet<int> bcs;
  EG_VTKDCC(vtkIntArray, cell_code, m_Grid, "cell_code");
  double A_max = 0; //area of the biggest neighbour cell of id_node
  for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
    double A = fabs(GeometryTools::cellVA(m_Grid, m_Part.n2cGG(id_node, i)));
    A_max = max(A, A_max);
    cell_normals[i] = GeometryTools::cellNormal(m_Grid, m_Part.n2cGG(id_node, i));
    cell_normals[i].normalise();
    bcs.insert(cell_code->GetValue(m_Part.n2cGG(id_node, i)));
  }
  int N = 0;
  for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
    double A = fabs(GeometryTools::cellVA(m_Grid, m_Part.n2cGG(id_node, i)));
    if (A > 0.01*A_max) {
      n += cell_normals[i];
      ++N;
    }
  }
  /*
  SurfaceProjection* proj = NULL;
  if (bcs.size() == 1) {
    proj = GuiMainWindow::pointer()->getSurfProj(*bcs.begin());
    if (proj) {
      proj->project(x_new, id_node, false, n);
      n = proj->lastProjNormal();
    }
  }
  */
  if (N == 0) {
    //EG_BUG;
    move = false;
  } else {
    n.normalise();
    double L_max = 0;// distance to the furthest neighbour node of id_node
    for (int i = 0; i < m_Part.n2nGSize(id_node); ++i) {
      vec3_t xn;
      m_Grid->GetPoint(m_Part.n2nGG(id_node, i), xn.data());
      double L = (xn - x_old).abs();
      L_max = max(L, L_max);
    }
    
    vec3_t x_summit;
    if(m_CorrectCurvature) {
      // better for mesher with interpolation
      x_summit = x_new + L_max*n;
    }
    else {
    // better for mesher without interpolation
      x_summit = x_old + L_max*n;
    }

    for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
      vec3_t x[3];
      vtkIdType N_pts, *pts;
      m_Grid->GetCellPoints(m_Part.n2cGG(id_node, i), N_pts, pts);
      if (N_pts != 3) {
        //EG_BUG;
        move = false;
        break;
      }
      for (int j = 0; j < N_pts; ++j) {
        m_Grid->GetPoint(pts[j], x[j].data());
      }
      if (GeometryTools::tetraVol(x[0], x[1], x[2], x_summit, false) <= 0) {
        move = false;
        break;
      }
    }
  }
  /*
  if (move) {
    for (int i = 0; i < cell_normals.size(); ++i) {
      for (int j = 0; j < cell_normals.size(); ++j) {
        if (cell_normals[i]*cell_normals[j] < -100.0) {//Why -100.0?
          saveGrid(m_Grid, "after_move2");
          qWarning()<<"Cannot move point "<<id_node<<" because opposite cell_normals.";
          move = false;
          m_Grid->GetPoints()->SetPoint(id_node, x_old.data());
          saveGrid(m_Grid, "before_move2");
          EG_BUG;
          break;
        }
      }
    }
  }
  */
  if (!move) {
    m_Grid->GetPoints()->SetPoint(id_node, x_old.data());
  }
  return move;
}

void LaplaceSmoother::featureCorrection(vtkIdType id_node, SurfaceProjection* proj, vec3_t &x_new)
{
  if (m_FeatureMagic > 0) {
    EG_VTKDCN(vtkDoubleArray, cl, m_Grid, "node_meshdensity_desired");
    EG_VTKDCN(vtkCharArray, node_type, m_Grid, "node_type");
    //if (node_type->GetValue(id_node) == VTK_FEATURE_EDGE_VERTEX) {
    {
      // "magic" vector to displace node for re-projection
      vec3_t magic_vector = m_NodeNormal[id_node];

      vec3_t x0 = x_new;
      vec3_t x1 = proj->project(x_new, id_node, m_CorrectCurvature);
      vec3_t n = proj->lastProjNormal();

      // check if mesh normal and projection normal are aligned
      // .. try to displace node slightly in order to get a proper projection normal
      //
      if (fabs(n*magic_vector) > 0.99) {
        x_new += 0.1*cl->GetValue(id_node)*magic_vector;
        x1 = proj->project(x_new, id_node, m_CorrectCurvature);
        n = proj->lastProjNormal();
      }

      if (fabs(n*magic_vector) <= 0.99) {

        // start the procedure if the vectors are not aligned
        //
        double L1 = 0;
        double L2 = m_FeatureMagic*cl->GetValue(id_node);
        for (int i = 0; i < 30; ++i) {
          x_new = x1 - 0.5*(L1 + L2)*magic_vector;
          x_new = proj->project(x_new, id_node, m_CorrectCurvature, n);
          double displacement = fabs((x_new - x1)*n);
          if (displacement > 0.01*cl->GetValue(id_node)) {
            L2 = 0.5*(L1 + L2);
          } else {

            // if there is no significant displacement after the first iteration
            // the node is probably in a smooth region of the surface
            // ==> stop here
            //
            if (i == 0) {
              x_new = x0;
              break;
            }

            L1 = 0.5*(L1 + L2);
          }
        }
        x_new = x1 - L1*magic_vector;
        x_new = proj->project(x_new, id_node, m_CorrectCurvature, n);
      } else {

        // If they are still aligned it is an awkward situation.
        // .. The node might be in a corner already
        // .. skip iteration in this case
        //
        x_new = x0;

      }
    }
  }
}

bool LaplaceSmoother::moveNode(vtkIdType id_node, vec3_t &Dx)
{
  if (!checkVector(Dx)) {
    return false;
  }
  EG_VTKDCN(vtkDoubleArray, cl, m_Grid, "node_meshdensity_desired");
  vec3_t x_old;
  m_Grid->GetPoint(id_node, x_old.data());
  bool moved = false;
  for (int i_relaxation = 0; i_relaxation < 1; ++i_relaxation) {
    vec3_t x_new = x_old + Dx;
    if (m_UseProjection) {
      int i_nodes = m_Part.localNode(id_node);
      if (m_NodeToBc[i_nodes].size() == 1 || m_UniformSnapPoints) {
        int bc = m_NodeToBc[i_nodes][0];
        x_new = GuiMainWindow::pointer()->getSurfProj(bc)->project(x_new, id_node, m_CorrectCurvature);
        featureCorrection(id_node, GuiMainWindow::pointer()->getSurfProj(bc), x_new);
      } else {
        for (int i_proj_iter = 0; i_proj_iter < m_ProjectionIterations; ++i_proj_iter) {
          foreach (int bc, m_NodeToBc[i_nodes]) {
            x_new = GuiMainWindow::pointer()->getSurfProj(bc)->project(x_new, id_node, m_CorrectCurvature);
          }
        }

        for (int i_proj_iter = 0; i_proj_iter < m_ProjectionIterations; ++i_proj_iter) {
          if (m_CorrectCurvature) {
            foreach (int bc, m_NodeToBc[i_nodes]) {
              x_new = GuiMainWindow::pointer()->getSurfProj(bc)->correctCurvature(GuiMainWindow::pointer()->getSurfProj(bc)->lastProjTriangle(), x_new);
            }
          }
        }

      }
    }

    // compute the minimal length of any edge adjacent to this node
    // .. This will be used to limit the node movement.
    // .. Hopefully jammed topologies can be avoided this way.
    //
    EG_VTKDCN(vtkDoubleArray, cl, m_Grid, "node_meshdensity_desired");
    vec3_t x_old;
    m_Grid->GetPoint(id_node, x_old.data());
    double L_min = cl->GetValue(id_node);
    for (int i = 0; i < m_Part.n2nGSize(id_node); ++i) {
      vtkIdType id_neigh = m_Part.n2nGG(id_node, i);
      vec3_t x_neigh;
      m_Grid->GetPoint(id_neigh, x_neigh.data());
      L_min = min(L_min, (x_old - x_neigh).abs());
    }

    // limit node displacement
    vec3_t dx = x_new - x_old;
    if (dx.abs() > m_Limit*L_min) {
      x_new -= dx;
      dx.normalise();
      x_new += m_Limit*L_min*dx;
    }

    if (setNewPosition(id_node, x_new)) {
      //if (m_NodeMovementCheck.moveNode(id_node, x_new)) {
      moved = true;
      Dx = x_new - x_old;
      break;
    }
    Dx *= 0.5;
  }
  return moved;
}

void LaplaceSmoother::fixNodes(const QVector<bool> &fixnodes)
{
  if (fixnodes.size() != m_Grid->GetNumberOfPoints()) {
    EG_BUG;
  }
  m_Fixed = fixnodes;
}

void LaplaceSmoother::operate()
{
  if (m_Fixed.size() != m_Grid->GetNumberOfPoints()) {
    m_Fixed.fill(false, m_Grid->GetNumberOfPoints());
  }
  QVector<int> bcs;
  GuiMainWindow::pointer()->getAllBoundaryCodes(bcs);
  if (m_UseProjection) {
    foreach (int bc, bcs) {
      GuiMainWindow::pointer()->getSurfProj(bc)->setForegroundGrid(m_Grid);
    }
  }
  UpdatePotentialSnapPoints(false, false);
  EG_VTKDCC(vtkIntArray,    cell_code, m_Grid, "cell_code");
  EG_VTKDCN(vtkCharArray,   node_type, m_Grid, "node_type" );
  EG_VTKDCN(vtkDoubleArray, cl,        m_Grid, "node_meshdensity_desired");
  QVector<vtkIdType> smooth_node(m_Grid->GetNumberOfPoints(), false);
  {
    l2g_t nodes = m_Part.getNodes();
    foreach (vtkIdType id_node, nodes) {
      smooth_node[id_node] = true;
    }
  }
  setAllSurfaceCells();
  l2g_t  nodes = m_Part.getNodes();
  m_NodeToBc.resize(nodes.size());
  for (int i_nodes = 0; i_nodes < nodes.size(); ++i_nodes) {
    QSet<int> bcs;
    for (int j = 0; j < m_Part.n2cLSize(i_nodes); ++j) {
      bcs.insert(cell_code->GetValue(m_Part.n2cLG(i_nodes, j)));
    }
    m_NodeToBc[i_nodes].resize(bcs.size());
    qCopy(bcs.begin(), bcs.end(), m_NodeToBc[i_nodes].begin());
  }

  QVector<vec3_t> x_new(nodes.size());

  QVector<bool> blocked(nodes.size(), false);
  for (int i_nodes = 0; i_nodes < nodes.size(); ++i_nodes) {
    for (int i = 0; i < m_Part.n2cLSize(i_nodes); ++i) {
      vtkIdType type = m_Grid->GetCellType(m_Part.n2cLG(i_nodes, i));
      if (!m_AllowedCellTypes.contains(type)) {
        blocked[i_nodes] = true;
        break;
      }
    }
  }

  for (int i_iter = 0; i_iter < m_NumberOfIterations; ++i_iter) {
    m_Success = true;
    computeNormals();
    for (int i_nodes = 0; i_nodes < nodes.size(); ++i_nodes) {
      vtkIdType id_node = nodes[i_nodes];
      if (!m_Fixed[id_node] && !blocked[i_nodes]) {
        if (smooth_node[id_node] && node_type->GetValue(id_node) != VTK_FIXED_VERTEX) {
          if (node_type->GetValue(id_node) != VTK_FIXED_VERTEX) {
            QVector<vtkIdType> snap_points = getPotentialSnapPoints(id_node);
            vec3_t n(0,0,0);
            if (snap_points.size() > 0) {
              vec3_t x_old;
              vec3_t x;
              x_new[i_nodes] = vec3_t(0,0,0);
              m_Grid->GetPoint(id_node, x_old.data());
              double w_tot = 0;
              double L_min = 1e99;
              foreach (vtkIdType id_snap_node, snap_points) {
                m_Grid->GetPoint(id_snap_node, x.data());
                double w = 1.0;
                w_tot += w;
                x_new[i_nodes] += w*x;
                n += m_NodeNormal[id_snap_node];
                double L = (x - x_old).abs();
                L_min = min(L, L_min);
              }
              n.normalise();
              x_new[i_nodes] *= 1.0/w_tot;

              if (m_UseNormalCorrection) {
                vec3_t dx = x_new[i_nodes] - x_old;
                double scal = dx*m_NodeNormal[id_node];
                x_new[i_nodes] += scal*m_NodeNormal[id_node];
              }
              vec3_t Dx = x_new[i_nodes] - x_old;
              Dx *= m_UnderRelaxation;
              if (moveNode(id_node, Dx)) {
                x_new[i_nodes] = x_old + Dx;
              } else {
                x_new[i_nodes] = x_old;
                m_Success = false;
              }
            }
          }
        }
      }
    }
    if (m_Success) {
      break;
    }
  }
}
