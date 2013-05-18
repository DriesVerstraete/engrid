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
#ifndef CHECKSURFACEINTEGRITY_H
#define CHECKSURFACEINTEGRITY_H

#include <surfaceoperation.h>

class CheckSurfaceIntegrity : public SurfaceOperation
{

private:

  bool m_IsWaterTight;
  int  m_Nmin;
  int  m_Nmax;
  QSet <vtkIdType> m_BadCells;
  QVector<int> m_NumCells;
  

public:

  CheckSurfaceIntegrity();
  bool isWaterTight();
  int getNmin() { return(m_Nmin); }
  int getNmax() { return(m_Nmax); }
  QSet <vtkIdType> getBadCells() { return(m_BadCells); }
  

protected: // methods

  virtual void operate();
  
};

#endif
