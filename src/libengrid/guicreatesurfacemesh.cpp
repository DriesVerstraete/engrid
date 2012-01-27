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
#include "guicreatesurfacemesh.h"

#include "swaptriangles.h"
#include "surfacemesher.h"
#include "vertexdelegate.h"
#include "laplacesmoother.h"
#include "guimainwindow.h"
#include "containertricks.h"
#include "updatedesiredmeshdensity.h"
#include "utilities.h"

#include <vtkSmoothPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>
#include <vtkLongArray.h>
#include <vtkEgNormalExtrusion.h>
#include <vtkIdList.h>
#include <vtkCell.h>
#include <vtkCellLocator.h>
#include <vtkFloatArray.h>
#include <vtkCellArray.h>

#include <QtGui>
#include <QTextStream>

#include <iostream>
#include <fstream>
#include <cmath>

#include <stdio.h>

//////////////////////////////////////////////

GuiCreateSurfaceMesh::GuiCreateSurfaceMesh()
{
  EG_TYPENAME;
  
  setQuickSave(true);

  populateBoundaryCodes(ui.listWidget);
  ui.lineEditMaximalEdgeLength->setText("1000");

  //Load settings
  readSettings();
  
  Nbc = ui.listWidget-> count ();
  
  current_filename= GuiMainWindow::pointer()->getFilename();

  {
    QString buffer = GuiMainWindow::pointer()->getXmlSection("engrid/surface/table");
    QTextStream in(&buffer, QIODevice::ReadOnly);

    m_NumRows = 0;
    m_NumCols = 0;
    in >> m_NumRows >> m_NumCols;

    if (m_NumRows == 0) {
      m_NumCols = Nbc + 3;
    }
    if(m_NumCols != Nbc + 3) {
      EG_BUG;
    }

    int row, column;
    QString str;

    m_Table.clear();
    m_Table.fill(QVector<QString>(m_NumCols), m_NumRows);

    for (int i = 0; i < m_NumRows; ++i) {
      for (int j = 0; j < m_NumCols; ++j) {
        in >> row >> column >> str;
        if (str == "{{{empty}}}") {
          str = "";
        }
        m_Table[row][column] = str;
      }
    }
  }
  setTextFromTable();

  
  connect(ui.pushButton_SelectAll_BC, SIGNAL(clicked()), this, SLOT(SelectAll_BC()));
  connect(ui.pushButton_ClearAll_BC, SIGNAL(clicked()), this, SLOT(ClearAll_BC()));

  m_ELSManager.setListWidget(ui.listWidgetSources);
  m_ELSManager.read();
  m_ELSManager.populateListWidget();
  connect(ui.pushButtonAddSphere,    SIGNAL(clicked()), this, SLOT(addSphere()));
  connect(ui.pushButtonAddCone,      SIGNAL(clicked()), this, SLOT(addCone()));
  connect(ui.pushButtonEditSource,   SIGNAL(clicked()), this, SLOT(edit()));
  connect(ui.pushButtonDeleteSource, SIGNAL(clicked()), this, SLOT(remove()));

}

///////////////////////////////////////////

int GuiCreateSurfaceMesh::readSettings()
{
  QString buffer = GuiMainWindow::pointer()->getXmlSection("engrid/surface/settings");
  if(!buffer.isEmpty()) {
    QTextStream in(&buffer, QIODevice::ReadOnly);
    QString str;
    in >> str;
    ui.lineEditMaximalEdgeLength->setText(str);
    in >> str;
    ui.lineEditMinimalEdgeLength->setText(str);
    in >> str;
    ui.lineEditGrowthFactor->setText(str);
    double nodes_per_quarter_circle;
    in >> nodes_per_quarter_circle;
    //nodes_per_quarter_circle = 0; ///\todo implement curvature resolution
    ui.doubleSpinBoxCurvature->setValue(nodes_per_quarter_circle);
    int num_bcs;
    in >> num_bcs;
    if (num_bcs == ui.listWidget->count()) {
      int check_state;
      for (int i = 0; i < ui.listWidget->count(); ++i) {
        in >> check_state;
        if (check_state == 1) {
          ui.listWidget->item(i)->setCheckState(Qt::Checked);
        } else {
          ui.listWidget->item(i)->setCheckState(Qt::Unchecked);
        }
      }
    }
  }
  m_ELSManager.read();
  return(0);
}

int GuiCreateSurfaceMesh::writeSettings()
{
  QString buffer = "";
  {
    QTextStream out(&buffer, QIODevice::WriteOnly);
    out << "\n";
    out << ui.lineEditMaximalEdgeLength->text() << "\n";
    out << ui.lineEditMinimalEdgeLength->text() << "\n";
    out << ui.lineEditGrowthFactor->text() << "\n";
    out << ui.doubleSpinBoxCurvature->value() << "\n";
    out << ui.listWidget->count() << "\n";
    for (int i = 0; i < ui.listWidget->count(); ++i) {
      if (ui.listWidget->item(i)->checkState() == Qt::Checked) {
        out << "1 \n";
      } else {
        out << "0 \n";
      }
    }
  }
  GuiMainWindow::pointer()->setXmlSection("engrid/surface/settings", buffer);
  m_ELSManager.write();
  return(0);
}

///////////////////////////////////////////

void GuiCreateSurfaceMesh::SelectAll_BC()
{
  for (int i = 0; i < ui.listWidget->count(); ++i) {
    ui.listWidget->item(i)->setCheckState(Qt::Checked);
  }
}

void GuiCreateSurfaceMesh::ClearAll_BC()
{
  for (int i = 0; i < ui.listWidget->count(); ++i) {
    ui.listWidget->item(i)->setCheckState(Qt::Unchecked);
  }
}

void GuiCreateSurfaceMesh::setTextFromTable()
{
  QString text = "";
  QSet<QString> values;
  for (int i = 0; i < m_NumRows; ++i) {
    values.insert(m_Table[i][m_NumCols-1]);
  }
  foreach (QString value, values) {
    int Ni = 0;
    for (int i = 0; i < m_NumRows; ++i) {
      if (m_Table[i][m_NumCols-1] == value) {
        ++Ni;
      }
    }
    int ni = 0;
    for (int i = 0; i < m_NumRows; ++i) {
      if (m_Table[i][m_NumCols-1] == value) {
        int Nj = 0;
        for (int j = 0; j < m_NumCols-3; ++j) {
          if (m_Table[i][j] == "2") {
            ++Nj;
          }
        }
        int nj = 0;
        for (int j = 0; j < m_NumCols-3; ++j) {
          if (m_Table[i][j] == "2") {
            QString bc = ui.listWidget->item(j)->text().split(":")[1].trimmed();
            text += bc;
            ++nj;
            if (nj < Nj) {
              text += " <AND> ";
            }
          }
        }
        ++ni;
        if (ni < Ni) {
          text += " <OR>\n";
        }
      }
    }
    text += " = " + value + ";\n\n";
  }
  ui.textEdit->setText(text);
}

void GuiCreateSurfaceMesh::getTableFromText()
{
  QMap<QString, int> bc_map;
  for (int i = 0; i < ui.listWidget->count(); ++i) {
    bc_map[ui.listWidget->item(i)->text().split(":")[1].trimmed()] = i;
  }
  QStringList rules = ui.textEdit->toPlainText().split(";", QString::SkipEmptyParts);
  m_Table.clear();
  foreach (QString rule, rules) {
    rule = rule.trimmed();
    QStringList parts = rule.split("=");
    if (parts.count() > 1) {
      QString left = parts[0].trimmed();
      QString right = parts[1].trimmed();
      QStringList rows = left.split("<OR>");
      foreach (QString row, rows) {
        row = row.trimmed();
        m_Table.append(QVector<QString>(m_NumCols, "1"));
        int r = m_Table.count() - 1;
        m_Table[r][m_NumCols - 3] = "any";
        m_Table[r][m_NumCols - 2] = "";
        m_Table[r][m_NumCols - 1] = right;
        QStringList cols = row.split("<AND>");
        foreach (QString col, cols) {
          col = col.trimmed();
          m_Table[r][bc_map[col]] = "2";
        }
      }
    }
  }
  m_NumRows = m_Table.size();
}

void GuiCreateSurfaceMesh::operate()
{
  writeSettings();
  getTableFromText();
  QString buffer = "";
  {
    QTextStream out(&buffer, QIODevice::WriteOnly);
    out << "\n";
    out << m_NumRows << " " << m_NumCols << "\n";
    for (int row = 0; row < m_NumRows; ++row) {
      for (int column = 0; column < m_NumCols; ++column) {
        QString str = m_Table[row][column];
        if (str.isEmpty()) {
          str = "{{{empty}}}";
        }
        out << row << " " << column << " " << str << "\n";
      }
    }
  }
  GuiMainWindow::pointer()->setXmlSection("engrid/surface/table", buffer);
}
