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
#ifndef LAPLACESMOOTHER_H
#define LAPLACESMOOTHER_H

#include "surfaceoperation.h"
#include "surfaceprojection.h"
#include "surfacenodemovementcheck.h"

class LaplaceSmoother : public SurfaceOperation
{

private:

  QSet<int> m_BCs;
  int       m_NumberOfIterations;
  bool      m_UseProjection;
  bool      m_UseNormalCorrection;
  double    m_UnderRelaxation;
  bool      m_Success;
  int       m_ProjectionIterations;
  double    m_FeatureMagic;
  double    m_Limit;

  QVector<QVector<int> > m_NodeToBc;

  bool      m_CorrectCurvature;
  bool      m_NoCheck;

  QSet<vtkIdType> m_AllowedCellTypes;
  QVector<bool> m_Fixed;

  
private: // methods

  bool setNewPosition(vtkIdType id_node, vec3_t x_new);
  bool moveNode(vtkIdType id_node, vec3_t &Dx);
  void featureCorrection(vtkIdType id_node, SurfaceProjection *proj, vec3_t &x_new);


public:

  LaplaceSmoother(); ///< default constructor
  virtual void operate(); ///< Run operation
  void setNumberOfIterations(int N) { m_NumberOfIterations = N;} ///< Set number of iterations
  void setProjectionOn() { m_UseProjection = true; }
  void setProjectionOff() { m_UseProjection = false; }
  void setNormalCorrectionOn() { m_UseNormalCorrection = true; }
  void setNormalCorrectionOff() { m_UseNormalCorrection = false; }
  bool succeeded() { return m_Success; }
  void fixNodes(const QVector<bool> &fixnodes);

public:

  void setCorrectCurvature(bool b) { m_CorrectCurvature = b; }
  bool getCorrectCurvature() { return m_CorrectCurvature; }
  void setNoCheck(bool b) { m_NoCheck = b; }
  bool getNoCheck() { return m_NoCheck; }
  void setProjectionIterations(int n) { m_ProjectionIterations = n; }

};

#endif
