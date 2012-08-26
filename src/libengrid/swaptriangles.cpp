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
#include "swaptriangles.h"
#include "guimainwindow.h"

#include <vtkCellArray.h>

#include "checksurfaceintegrity.h"
#include "statistics.h"

using namespace GeometryTools;

SwapTriangles::SwapTriangles() : SurfaceOperation()
{
  m_RespectBC = false;
  m_FeatureSwap = false;
  m_FeatureAngle = GeometryTools::deg2rad(30);
  m_MaxNumLoops = 20;
  m_SmallAreaSwap = false;
  m_SmallAreaRatio = 1e-3;
  m_Verbose = false;
  getSet("surface meshing", "small area ratio for edge-swapping", 1e-3, m_SmallAreaRatio);
  getSet("surface meshing", "surface error threshold (lower)", 0.05, m_SurfErrorLowerThreshold);
  getSet("surface meshing", "surface error threshold (higher)", 0.25, m_SurfErrorHigherThreshold);
}

bool SwapTriangles::testOrientation(stencil_t S)
{
  // old triangles
  vec3_t n1_old = triNormal(m_Grid, S.id_node[0], S.p1, S.p2);
  vec3_t n2_old = triNormal(m_Grid, S.id_node[1], S.p2, S.p1);

  // new triangles
  vec3_t n1_new = triNormal(m_Grid, S.p1, S.id_node[1], S.id_node[0]);
  vec3_t n2_new = triNormal(m_Grid, S.p2, S.id_node[0], S.id_node[1]);

  // top point
  vec3_t x_summit(0,0,0);
  vec3_t x[4];
  double l_max = 0;
  m_Grid->GetPoints()->GetPoint(S.id_node[0], x[0].data());
  m_Grid->GetPoints()->GetPoint(S.p1,         x[1].data());
  m_Grid->GetPoints()->GetPoint(S.id_node[1], x[2].data());
  m_Grid->GetPoints()->GetPoint(S.p2,         x[3].data());
  for (int k = 0; k < 4; ++k) {
    x_summit += x[k];
  }
  for (int k = 0; k < 4; ++k) {
    int i = k;
    int j = k + 1;
    if (j == 4) {
      j = 0;
    }
    l_max = max(l_max, (x[i]-x[j]).abs());
  }
  x_summit *= 0.25;
  vec3_t n = n1_old + n2_old;
  n.normalise();
  x_summit += 3*l_max*n;

  // old volumes
  double V1_old = tetraVol(x[0], x[1], x[3], x_summit, true);
  double V2_old = tetraVol(x[2], x[3], x[1], x_summit, true);
  // new volumes
  double V1_new = tetraVol(x[1], x[2], x[0], x_summit, true);
  double V2_new = tetraVol(x[3], x[0], x[2], x_summit, true);

  bool swap_ok = false;
  if (m_SmallAreaSwap) {
     swap_ok = V1_new>0 && V2_new>0;
  } else {
     swap_ok = V1_old>0 && V2_old>0 && V1_new>0 && V2_new>0;
  }
  return swap_ok;
}

bool SwapTriangles::isEdge(vtkIdType id_node1, vtkIdType id_node2)
{
  l2g_t  nodes = getPartNodes();
  g2l_t _nodes = getPartLocalNodes();
  l2l_t  n2n   = getPartN2N();

  bool ret = false;
  foreach(int i_node, n2n[_nodes[id_node1]]) {
    vtkIdType id_node = nodes[i_node];
    if( id_node == id_node2 ) ret = true;
  }
  return(ret);
}

void SwapTriangles::computeSurfaceErrors(const QVector<vec3_t> &x, int bc, double &err1, double &err2)
{
  using namespace GeometryTools;
  err1 = 0;
  err2 = 0;
  SurfaceProjection* proj = GuiMainWindow::pointer()->getSurfProj(bc);
  if (!proj) {
    return;
  }

  vec3_t n = triNormal(x[0], x[1], x[3]) + triNormal(x[1], x[2], x[3]);
  n.normalise();

  vec3_t xe1 = 0.5*(x[1] + x[3]);
  vec3_t xe2 = 0.5*(x[0] + x[2]);

  vec3_t xe1_proj = proj->project(xe1, -1, true, n);
  vec3_t xe2_proj = proj->project(xe2, -1, true, n);

  err1 = (xe1 - xe1_proj).abs()/(x[1] - x[3]).abs();
  err2 = (xe2 - xe2_proj).abs()/(x[0] - x[2]).abs();
}

int SwapTriangles::swap()
{
  int N_swaps = 0;
  setAllSurfaceCells();
  //computeAverageSurfaceError();
  EG_VTKDCC(vtkIntArray, cell_code, m_Grid, "cell_code");
  QVector<bool> marked(m_Grid->GetNumberOfCells(), false);
  for (int i = 0; i < m_Part.getNumberOfCells(); ++i) {
    vtkIdType id_cell = m_Part.globalCell(i);
    if (!m_BoundaryCodes.contains(cell_code->GetValue(id_cell)) && m_Grid->GetCellType(id_cell) == VTK_TRIANGLE) { //if it is a selected triangle
      if (!marked[id_cell] && !m_Swapped[id_cell]) {
        for (int j = 0; j < 3; ++j) {
          bool swap = false;
          stencil_t S = getStencil(id_cell, j);
          if(S.id_cell.size() == 2 && S.sameBC) {
            if (S.type_cell[1] == VTK_TRIANGLE) {
              if(!isEdge(S.id_node[0], S.id_node[1]) ) {
                if (!marked[S.id_cell[1]] && !m_Swapped[S.id_cell[1]]) {
                  QVector<vec3_t> x3(4);
                  vec2_t x[4];

                  m_Grid->GetPoint(S.id_node[0], x3[0].data());
                  m_Grid->GetPoint(S.p1,         x3[1].data());
                  m_Grid->GetPoint(S.id_node[1], x3[2].data());
                  m_Grid->GetPoint(S.p2,         x3[3].data());

                  vec3_t n1 = triNormal(x3[0], x3[1], x3[3]);
                  vec3_t n2 = triNormal(x3[1], x3[2], x3[3]);

                  bool force_swap = false;
                  if (m_SmallAreaSwap) {
                    double A1 = n1.abs();
                    double A2 = n2.abs();
                    if (isnan(A1) || isnan(A2)) {
                      force_swap = true;
                    } else {
                      force_swap = A1 < m_SmallAreaRatio*A2 || A2 < m_SmallAreaRatio*A1;
                    }
                  }
                  if (m_FeatureSwap || GeometryTools::angle(n1, n2) < m_FeatureAngle || force_swap) {

                    // surface errors
                    double se1, se2;
                    bool surf_block = false;
                    computeSurfaceErrors(x3, cell_code->GetValue(id_cell), se1, se2);
                    if (se2 > se1 && se2 > m_SurfErrorHigherThreshold && se1 < m_SurfErrorLowerThreshold) {
                      surf_block = true;
                    } else {
                      if (se2 < se1 && se1 > m_SurfErrorHigherThreshold && se2 < m_SurfErrorLowerThreshold) {
                        swap = true;
                      }
                    }

                    // Delaunay
                    if (!swap && !surf_block) {
                      vec3_t n = n1 + n2;
                      n.normalise();
                      vec3_t ex = orthogonalVector(n);
                      vec3_t ey = ex.cross(n);
                      for (int k = 0; k < 4; ++k) {
                        x[k] = vec2_t(x3[k]*ex, x3[k]*ey);
                      }
                      vec2_t r1, r2, r3, u1, u2, u3;
                      r1 = 0.5*(x[0] + x[1]); u1 = turnLeft(x[1] - x[0]);
                      r2 = 0.5*(x[1] + x[2]); u2 = turnLeft(x[2] - x[1]);
                      r3 = 0.5*(x[1] + x[3]); u3 = turnLeft(x[3] - x[1]);
                      double k, l;
                      vec2_t xm1, xm2;
                      bool ok = true;
                      if (intersection(k, l, r1, u1, r3, u3)) {
                        xm1 = r1 + k*u1;
                        if (intersection(k, l, r2, u2, r3, u3)) {
                          xm2 = r2 + k*u2;
                        } else {
                          ok = false;
                        }
                      } else {
                        ok = false;
                        swap = true;
                      }
                      if (ok) {
                        if ((xm1 - x[2]).abs() < (xm1 - x[0]).abs()) {
                          swap = true;
                        }
                        if ((xm2 - x[0]).abs() < (xm2 - x[2]).abs()) {
                          swap = true;
                        }
                      }
                    }
                  //}// end of testswap
                  } //end of if feature angle
                } //end of if l_marked
              } //end of if TestSwap
            }
          } //end of S valid

          if (swap) {
            if (testOrientation(S)) {
              marked[S.id_cell[0]] = true;
              marked[S.id_cell[1]] = true;
              for (int k = 0; k < m_Part.n2cGSize(S.id_node[0]); ++k) {
                vtkIdType id_neigh = m_Part.n2cGG(S.id_node[0], k);
                marked[id_neigh] = true;
              }
              for (int k = 0; k < m_Part.n2cGSize(S.id_node[1]); ++k) {
                vtkIdType id_neigh = m_Part.n2cGG(S.id_node[1], k);
                marked[id_neigh] = true;
              }
              for (int k = 0; k < m_Part.n2cGSize(S.p1); ++k) {
                vtkIdType id_neigh = m_Part.n2cGG(S.p1, k);
                marked[id_neigh] = true;
              }
              for (int k = 0; k < m_Part.n2cGSize(S.p2); ++k) {
                vtkIdType id_neigh = m_Part.n2cGG(S.p2, k);
                marked[id_neigh] = true;
              }
              vtkIdType new_pts1[3], new_pts2[3];
              new_pts1[0] = S.p1;
              new_pts1[1] = S.id_node[1];
              new_pts1[2] = S.id_node[0];
              new_pts2[0] = S.id_node[1];
              new_pts2[1] = S.p2;
              new_pts2[2] = S.id_node[0];
              vtkIdType old_pts1[3], old_pts2[3];
              old_pts1[0] = S.id_node[0];
              old_pts1[1] = S.p1;
              old_pts1[2] = S.p2;
              old_pts2[0] = S.id_node[1];
              old_pts2[1] = S.p2;
              old_pts2[2] = S.p1;
              m_Grid->ReplaceCell(S.id_cell[0], 3, new_pts1);
              m_Grid->ReplaceCell(S.id_cell[1], 3, new_pts2);
              m_Swapped[S.id_cell[0]] = true;
              m_Swapped[S.id_cell[1]] = true;
              ++N_swaps;
              break;
            }
          } //end of if swap
        } //end of loop through sides
      } //end of if marked
    } //end of if selected triangle
  } //end of loop through cells
  return N_swaps;
}

void SwapTriangles::computeAverageSurfaceError()
{
  EG_VTKDCC(vtkIntArray, cell_code, m_Grid, "cell_code");
  QList<double> errors;
  EG_FORALL_NODES(id_node1, m_Grid) {
    vec3_t n1 = m_Part.globalNormal(id_node1);
    vec3_t x1;
    m_Grid->GetPoint(id_node1, x1.data());
    for (int i = 0; i < m_Part.n2nGSize(id_node1); ++i) {
      vtkIdType id_node2 = m_Part.n2nGG(id_node1, i);
      if (id_node1 < id_node2) {
        QVector<vtkIdType> faces;
        getEdgeCells(id_node1, id_node2, faces);
        if (faces.size() == 2) {
          int bc1 = cell_code->GetValue(faces[0]);
          int bc2 = cell_code->GetValue(faces[1]);
          if (bc1 == bc2) {
            SurfaceProjection* proj = GuiMainWindow::pointer()->getSurfProj(bc1);
            if (proj) {
              vec3_t n2 = m_Part.globalNormal(id_node2);
              vec3_t n = 0.5*(n1 + n2);
              vec3_t x2;
              m_Grid->GetPoint(id_node2, x2.data());
              vec3_t x = 0.5*(x1 + x2);
              vec3_t xp = proj->project(x, -1, true, n);
              double err = (x - xp).abs()/(x1 - x2).abs();
              errors.append(err);
            }
          }
        }
      }
    }
  }

  m_AverageSurfaceError = Statistics::meanValue(errors);
  m_SurfaceErrorDeviation = Statistics::standardDeviation(errors, m_AverageSurfaceError);
  cout << "mean: " << m_AverageSurfaceError << "  sigma: " << m_SurfaceErrorDeviation << endl;
}

void SwapTriangles::operate()
{
  if (m_Verbose) cout << "swapping edges for surface triangles ..." << endl;
  long int N_swaps      = 100000000;
  long int N_last_swaps = 100000001;
  int loop = 1;
  while ((N_swaps > 0) && (loop <= m_MaxNumLoops) && (N_swaps < N_last_swaps)) {
    N_last_swaps = N_swaps;
    if (m_Verbose) cout << "  loop " << loop << "/" << m_MaxNumLoops << endl;
    m_Swapped.fill(false, m_Grid->GetNumberOfCells());
    N_swaps = 0;
    int N;
    int sub_loop = 0;
    do {
      ++sub_loop;
      N = swap();
      if (m_Verbose) cout << "    sub-loop " << sub_loop << ": " << N << " swaps" << endl;
      N_swaps += N;
    } while (N > 0);
    ++loop;
  }
}

