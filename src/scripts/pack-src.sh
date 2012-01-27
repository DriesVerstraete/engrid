#!/bin/sh
# 
# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# +                                                                      +
# + This file is part of enGrid.                                         +
# +                                                                      +
# + Copyright 2008-2012 enGits GmbH                                     +
# +                                                                      +
# + enGrid is free software: you can redistribute it and/or modify       +
# + it under the terms of the GNU General Public License as published by +
# + the Free Software Foundation, either version 3 of the License, or    +
# + (at your option) any later version.                                  +
# +                                                                      +
# + enGrid is distributed in the hope that it will be useful,            +
# + but WITHOUT ANY WARRANTY; without even the implied warranty of       +
# + MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        +
# + GNU General Public License for more details.                         +
# +                                                                      +
# + You should have received a copy of the GNU General Public License    +
# + along with enGrid. If not, see <http://www.gnu.org/licenses/>.       +
# +                                                                      +
# ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 
mkdir git.tmp
cd git.tmp
git clone ssh://engits.eu/git/engrid.git
cd engrid
git checkout $1
echo "#ifndef ENGRID_VERSION" > src/engrid_version.h
echo "#define ENGRID_VERSION \"`git describe`\"" >> src/engrid_version.h
echo "#endif" >> src/engrid_version.h
rm -rf .*ignore
rm -rf .git
rm -rf OBS
rm -rf debian
rm -rf manual
cd ..
mv engrid engrid-$2
tar cvzf ../../engrid-$2.tar.gz engrid-$2
cd ..
rm -rf git.tmp
