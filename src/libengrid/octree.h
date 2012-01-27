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
#ifndef OCTREE_H
#define OCTREE_H

class Octree;

#include "egvtkobject.h"

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class OctreeNode
{

  friend class Octree;

  vec3_t m_Position;

public:

  vec3_t& getPosition()                { return m_Position; }
  void    setPosition(const vec3_t& x) { m_Position = x; }

};

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class OctreeCell
{

  friend class Octree;

  int m_Node[8];
  int m_Child[8];
  int m_Neighbour[6];
  int m_Parent;
  int m_Level;

public:

  OctreeCell();

  int  getNode     (int i) { return m_Node[i]; }
  int  getNeighbour(int i) { return m_Neighbour[i]; }
  bool hasChildren ()      { return m_Child[0] != -1; }
  int  getParent   ()      { return m_Parent; }

  int  getEdgeNode(Octree* octree, int n1, int n2, int f);
  void getFaceNodes(int i, Octree* octree, QVector<int>& face_nodes, bool reverse = false);
  void getFaceNodes(int i, Octree* octree, QVector<QVector<int> >& face_nodes, bool reverse = false);

};


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

class Octree : public EgVtkObject
{

  friend class OctreeCell;

private: // attributes

  vec3_t m_Origin;  ///< origin of internal coordinate system
  mat3_t m_Base;    ///< base vectors of internal coordinate system
  mat3_t m_InvBase; ///< inverted base of internal coordiante system
  vec3_t m_Corner1; ///< first corner of extend box of the whole domain (in internal coordinates)
  vec3_t m_Corner2; ///< second corner of extend box of the whole domain (in internal coordinates)
  double m_Dx;      ///< extend in x direction
  double m_Dy;      ///< extend in y direction
  double m_Dz;      ///< extend in z direction

  bool   m_SmoothTransition;

  QVector<OctreeNode> m_Nodes;
  QVector<OctreeCell> m_Cells;
  QVector<bool>       m_ToRefine;
  QVector<int>        m_SameNodes;
  int                 m_MaxCells;

private: // methods

  void mergeNodes_identifyDuplicates();
  void mergeNodes_compactNodes();
  void mergeNodes_updateCells();
  void mergeNodes();
  void checkNeighbours();

  int  opposingFace(int i);

  void toVtkGrid_HangingNodes(vtkUnstructuredGrid *grid, bool create_fields);
  void toVtkGrid_Conforming(vtkUnstructuredGrid *grid, bool create_fields);


public: // methods

  Octree();

  void setOrigin(vec3_t x0);
  void setBase(vec3_t g1, vec3_t g2, vec3_t g3);
  void setBounds(vec3_t corner1, vec3_t corner2);

  //int  getNeighbour(int cell, int neigh) { return m_Cells[cell].m_Neighbour[neigh]; }

  void markToRefine(int cell) { m_ToRefine[cell] = true; }
  bool markedForRefine(int cell) { return m_ToRefine[cell]; }
  int  refineAll();
  void resetRefineMarks();
  void setSmoothTransitionOn()  { m_SmoothTransition = true; }
  void setSmoothTransitionOff() { m_SmoothTransition = false; }

  vec3_t getCellCentre(int cell);
  vec3_t getFaceCentre(int i_cells, int i_faces);
  vec3_t getNodePosition(int cell, int node) { return m_Nodes[m_Cells[cell].m_Node[node]].m_Position; }
  vec3_t getNodePosition(int node) { return m_Nodes[node].m_Position; }
  int    getNode(int cell, int node) { return m_Cells[cell].m_Node[node]; }
  int    getNumCells() { return m_Cells.size(); }
  int    getNumNodes() { return m_Nodes.size(); }
  void   getEdges(int cell, QVector<SortedPair<int> >& edges);
  int    getLevel(int cell) { return m_Cells[cell].m_Level; }
  double getDx(int cell);
  double getDy(int cell);
  double getDz(int cell);
  double getDx(const OctreeCell& cell);
  double getDy(const OctreeCell& cell);
  double getDz(const OctreeCell& cell);
  bool   hasChildren(int i_cells) { return m_Cells[i_cells].m_Child[0] != -1; }
  int    getParent(int cell) { return m_Cells[cell].m_Parent; }
  int    findCell(vec3_t x);
  bool   intersectsFace(int cell, int face, vec3_t x1, vec3_t x2, double &k, double tol = 1e-4);
  void   setMaxCells(int n) { m_MaxCells = n; }
  bool   isInsideBounds(vec3_t x);

  void toVtkGrid(vtkUnstructuredGrid *grid, bool hanging_nodes = true, bool create_fields = false);

};


inline double Octree::getDx(int cell)
{
  double dx = m_Dx;
  for (int i = 0; i < m_Cells[cell].m_Level; ++i) {
    dx *= 0.5;
  }
  return dx;
}

inline double Octree::getDx(const OctreeCell& cell)
{
  double dx = m_Dx;
  for (int i = 0; i < cell.m_Level; ++i) {
    dx *= 0.5;
  }
  return dx;
}

inline double Octree::getDy(int cell)
{
  double dy = m_Dy;
  for (int i = 0; i < m_Cells[cell].m_Level; ++i) {
    dy *= 0.5;
  }
  return dy;
}

inline double Octree::getDy(const OctreeCell& cell)
{
  double dy = m_Dy;
  for (int i = 0; i < cell.m_Level; ++i) {
    dy *= 0.5;
  }
  return dy;
}

inline double Octree::getDz(int cell)
{
  double dz = m_Dz;
  for (int i = 0; i < m_Cells[cell].m_Level; ++i) {
    dz *= 0.5;
  }
  return dz;
}

inline double Octree::getDz(const OctreeCell& cell)
{
  double dz = m_Dz;
  for (int i = 0; i < cell.m_Level; ++i) {
    dz *= 0.5;
  }
  return dz;
}

#endif // OCTREE_H
