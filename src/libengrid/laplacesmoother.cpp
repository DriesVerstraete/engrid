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
#include "laplacesmoother.h"
#include <vtkCellLocator.h>
#include <vtkCharArray.h>
#include <vtkGenericCell.h>

#include "guimainwindow.h"
#include "localnodegraphinterface.h"
#include "checkerboardgraphiterator.h"

using namespace GeometryTools;

LaplaceSmoother::LaplaceSmoother() : SurfaceOperation()
{
  DebugLevel = 0;
  setQuickSave(true);
  m_UseProjection = true;
  //   m_UseNormalCorrection = false;
  getSet("surface meshing", "under relaxation for smoothing", 0.5, m_UnderRelaxation);
  getSet("surface meshing", "feature magic", 1.0, m_FeatureMagic);
  getSet("surface meshing", "smoothing limiter", 1.0, m_Limit);
  getSet("surface meshing", "use uniform smoothing", false, m_UniformSnapPoints);
  m_Limit = min(1.0, max(0.0, m_Limit));
  m_NoCheck = false;
  m_ProjectionIterations = 50;
  m_AllowedCellTypes.clear();
  m_AllowedCellTypes.insert(VTK_TRIANGLE);
  //m_StrictFeatureSnap = false;
}

bool LaplaceSmoother::setNewPosition(vtkIdType id_node, vec3_t x_new)
{
  using namespace GeometryTools;

  vec3_t x_old;
  m_Grid->GetPoint(id_node, x_old.data());
  EG_VTKDCN(vtkCharArray, node_type, m_Grid, "node_type");
  bool move = true;
  if(m_NoCheck) {
    return move;
  }
  QVector<vec3_t> old_cell_normals(m_Part.n2cGSize(id_node));
  EG_VTKDCC(vtkIntArray, cell_code, m_Grid, "cell_code");
  for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
    old_cell_normals[i] = GeometryTools::cellNormal(m_Grid, m_Part.n2cGG(id_node, i));
  }
  m_Grid->GetPoints()->SetPoint(id_node, x_new.data());

  for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
    vec3_t n = GeometryTools::cellNormal(m_Grid, m_Part.n2cGG(id_node, i));
    if (n*old_cell_normals[i] < 0.2*old_cell_normals[i].abs2()) {
      move = false;
      break;
    }
  }
  if (!move) {
    m_Grid->GetPoints()->SetPoint(id_node, x_old.data());
  }
  return move;
}

void LaplaceSmoother::featureCorrection(vtkIdType id_node, CadInterface *cad_interface, vec3_t &x_new)
{
  EG_VTKDCN(vtkDoubleArray, node_mesh_quality, m_Grid, "node_mesh_quality");
  if (m_FeatureMagic > 0 && node_mesh_quality->GetValue(id_node) > m_FaceOrientationThreshold) {

    EG_VTKDCN(vtkDoubleArray, cl, m_Grid, "node_meshdensity_desired");
    EG_VTKDCN(vtkCharArray, node_type, m_Grid, "node_type");
    bool convex = isConvexNode(id_node);
    if (node_type->GetValue(id_node) == EG_FEATURE_CORNER_VERTEX) {

      vec3_t x;
      double L = 0.1*cl->GetValue(id_node);

      // do not use curvature correction here
      // .. a proper CAD model does not need it
      // .. a discrete model (e.g. triangulation) will create bulges on features
      vec3_t x0 = cad_interface->projectNode(id_node, x_new, false);

      if (convex) {
        x = x0 - L*m_NodeNormal[id_node];
      } else {
        x = x0 + L*m_NodeNormal[id_node];
      }
      vec3_t n = cad_interface->getLastNormal();
      if (!cad_interface->failed()) {
        double d = 2*L/tan(0.5*m_FeatureAngle);
        static const int num_steps = 36;
        double D_alpha = 2*M_PI/num_steps;
        vec3_t v;

        v = GeometryTools::orthogonalVector(m_NodeNormal[id_node]);
        int num_miss = 0;
        int num_hit = 0;
        vec3_t x_corner(0,0,0);
        for (int i = 0; i < num_steps; ++i) {
          v = GeometryTools::rotate(v, m_NodeNormal[id_node], D_alpha);
          vec3_t xp = cad_interface->projectNode(id_node, x, v, true, true);
          if (cad_interface->failed()) {
            ++num_miss;
          } else {
            double l = (x - xp).abs();
            if (l < d) {
              ++num_hit;
              x_corner += xp;
            } else {
              ++num_miss;
            }
          }
        }
        if (num_miss == 0 && num_hit > 0) {
          x_corner *= 1.0/num_hit;
          x_new = cad_interface->projectNode(id_node, x_corner, m_NodeNormal[id_node]);
        }
      }

    } else {

      // "magic" vector to displace node for re-projection
      vec3_t x0  = x_new;

      // do not use curvature correction here
      // .. a proper CAD model does not need it
      // .. a discrete model (e.g. triangulation) will create bulges on features
      vec3_t x1  = cad_interface->projectNode(id_node, x_new);
      vec3_t n   = cad_interface->getLastNormal();
      double l   = cl->GetValue(id_node);
      double eps = 0.01*l;
      vec3_t mv  = m_FeatureMagic*l*m_NodeNormal[id_node];
      mv -= (n*mv)*n;

      if (checkVector(mv)) {
        double L1 = 0;
        double L2 = 1;//m_FeatureMagic*cl->GetValue(id_node);
        bool flipped = false;
        double amp = 0.1;
        int i = 0;
        int hits = 0;
        while (i < 30 && amp < 10) {
          x_new = x1 + 0.5*amp*(L1 + L2)*mv;// + 2*eps*n;
          //x_new = proj->findClosest(x_new, id_node, n);
          x_new = cad_interface->projectNode(id_node, x_new, n, false, m_CorrectCurvature);
          double displacement = fabs((x_new - x1)*n);
          if (displacement > eps || cad_interface->failed()) {
            L2 = 0.5*(L1 + L2);
            ++i;
          } else {

            // if there is no significant displacement after the first iteration
            // the node is probably in a smooth region of the surface
            // ==> stop here
            //
            if (i == 0) {
              if (!flipped) {
                mv *= -1;
                flipped = true;
              } else {
                amp *= 1.5;
                mv *= -1;
                flipped = false;
              }
            } else {
              L1 = 0.5*(L1 + L2);
              ++hits;
              ++i;
            }
          }
        }
        if (hits > 0) {
          x_new = x1 + L1*amp*mv;
          x_new = cad_interface->projectNode(id_node, x_new, n, false, m_CorrectCurvature);
          if (cad_interface->failed()) {
            cout << "bad!" << endl;
          }
        } else {
          x_new = x0;
        }
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

  for (int i_relaxation = 0; i_relaxation < 10; ++i_relaxation) {
    vec3_t x_new = x_old + Dx;
    if (m_UseProjection) {
      int i_nodes = m_Part.localNode(id_node);
      if (m_NodeToBc[i_nodes].size() == 1 || m_UniformSnapPoints) {
        int bc = m_NodeToBc[i_nodes][0];
        x_new = GuiMainWindow::pointer()->getCadInterface(bc)->snapNode(id_node, x_new, m_CorrectCurvature);
        featureCorrection(id_node, GuiMainWindow::pointer()->getCadInterface(bc), x_new);
      } else {
        for (int i_proj_iter = 0; i_proj_iter < m_ProjectionIterations; ++i_proj_iter) {
          foreach (int bc, m_NodeToBc[i_nodes]) {
            x_new = GuiMainWindow::pointer()->getCadInterface(bc)->snapNode(id_node, x_new, m_CorrectCurvature);
          }
        }

        for (int i_proj_iter = 0; i_proj_iter < m_ProjectionIterations; ++i_proj_iter) {
          if (m_CorrectCurvature) {
            foreach (int bc, m_NodeToBc[i_nodes]) {
              //x_new = GuiMainWindow::pointer()->getCadInterface(bc)->correctCurvature(GuiMainWindow::pointer()->getCadInterface(bc)->lastProjTriangle(), x_new);
              x_new = GuiMainWindow::pointer()->getCadInterface(bc)->correctCurvature(x_new);
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
      m_Grid->GetPoints()->SetPoint(id_node, x_new.data());
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
  if (m_BCodeFeatureDefinition) {
    m_FeatureMagic = 0.0;
    m_NoCheck = false;
    m_NoCheck = true;
  } else {
    m_NoCheck = true;
  }
  if (m_Fixed.size() != m_Grid->GetNumberOfPoints()) {
    m_Fixed.fill(false, m_Grid->GetNumberOfPoints());
  }
  QVector<int> bcs;
  GuiMainWindow::pointer()->getAllBoundaryCodes(bcs);
  if (m_UseProjection) {
    foreach (int bc, bcs) {
      GuiMainWindow::pointer()->getCadInterface(bc)->setForegroundGrid(m_Grid);
    }
  }
  updateNodeInfo();
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
      m_Grid->GetPoint(id_node, x_new[i_nodes].data());
      if (!m_Fixed[id_node] && !blocked[i_nodes]) {
        if (smooth_node[id_node] && node_type->GetValue(id_node) != EG_FIXED_VERTEX) {
          if (node_type->GetValue(id_node) != EG_FIXED_VERTEX) {
            QVector<vtkIdType> snap_points = getPotentialSnapPoints(id_node);
            vec3_t n(0,0,0);
            vec3_t x_old = x_new[i_nodes];

            if (snap_points.size() > 0) {
              vec3_t x;
              x_new[i_nodes] = vec3_t(0,0,0);
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
            } else {
              x_new[i_nodes] = x_old;
            }

            if (m_UseNormalCorrection) {
              vec3_t dx = x_new[i_nodes] - x_old;
              double scal = dx*m_NodeNormal[id_node];
              x_new[i_nodes] += scal*m_NodeNormal[id_node];
            }
            vec3_t Dx = x_new[i_nodes] - x_old;
            //Dx *= m_UnderRelaxation;
            if (moveNode(id_node, Dx)) {
              x_new[i_nodes] = x_old + m_UnderRelaxation*Dx;
            } else {
              x_new[i_nodes] = x_old;
              //m_Success = false;
            }
            m_Grid->GetPoints()->SetPoint(id_node, x_old.data());
          }
        }
      }
    }
    for (int i_nodes = 0; i_nodes < nodes.size(); ++i_nodes) {
      vtkIdType id_node = nodes[i_nodes];
      m_Grid->GetPoints()->SetPoint(id_node, x_new[i_nodes].data());
    }

    if (m_Success) {
      break;
    }
  }
}
