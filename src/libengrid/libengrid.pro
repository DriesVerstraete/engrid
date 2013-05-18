TEMPLATE = lib
LANGUAGE = C++
TARGET   = engrid

# Enable this if the VTK from the ParaView sources and 
# installation want to be used
# Note: Currently only for Windows Compiles with MSVC
Use_VTK_Win_ParaView = yes


CONFIG += qt \
          debug_and_release \
          thread

QT     += xml \
          network \
          opengl

QMAKE_CXXFLAGS += -fopenmp

win32-msvc* {
    QMAKE_CXXFLAGS += -W3
    DEFINES += LIBENGRID_EXPORTS
    DEFINES += DLL_EXPORT
} win32-g++* {
    CONFIG += console
    DEFINES += LIBENGRID_EXPORTS
    DEFINES += DLL_EXPORT
    QMAKE_CXXFLAGS += -Wall
    QMAKE_CXXFLAGS += -Wno-deprecated
    QMAKE_CXXFLAGS += -Wl,--no-undefined
    QMAKE_CXXFLAGS += -Wl,--enable-runtime-pseudo-reloc
} else {
    QMAKE_CXXFLAGS += -Wall
    QMAKE_CXXFLAGS += -Wno-deprecated
    QMAKE_CXXFLAGS += -fno-omit-frame-pointer
    QMAKE_CXXFLAGS += -g
}


INCLUDEPATH += ..
INCLUDEPATH += ./libengrid-build
INCLUDEPATH += ../netgen_svn/netgen-mesher/netgen/nglib
INCLUDEPATH += ../netgen_svn/netgen-mesher/netgen/libsrc/general

#BRL-CAD
INCLUDEPATH += $(BRLCADINCDIR)
INCLUDEPATH += $(BRLCADINCDIR)/openNURBS
DEFINES     += BRLCAD_SUPPORT

!debian {
    INCLUDEPATH += ../netgen_svn/netgen-mesher/netgen/nglib
    INCLUDEPATH += ../netgen_svn/netgen-mesher/netgen/libsrc/general
}

#INCLUDEPATH for VTK depends on the compiler
win32-msvc* {
    DEFINES += _USE_MATH_DEFINES

    !isEmpty(Use_VTK_Win_ParaView) {
        include(../misc/engrid-vtk-win_paraview.pri)
    } else {
        INCLUDEPATH += $(VTKINCDIR)
    }
} win32-g++* {
    INCLUDEPATH += $(VTKINCDIR)
} else {
    INCLUDEPATH += $(VTKINCDIR)
}

RESOURCES += engrid.qrc

HEADERS = \
    blenderreader.h \
    blenderwriter.h \
    boundarycondition.h \
    boxselect.h \
    brlcadinterface.h \
    brlcadreader.h \
    cadinterface.h \
    celllayeriterator.h \
    cellneighbouriterator.h \
    cgnswriter.h \
    checkerboardgraphiterator.h \
    checkforoverlap.h \
    checksurfaceintegrity.h \
    containertricks.h \
    correctsurfaceorientation.h \
    createboundarylayer.h \
    createcadtesselation.h \
    createhexcore.h \
    createvolumemesh.h \
    deletecells.h \
    deletepickedcell.h \
    deletepickedpoint.h \
    deletestraynodes.h \
    deletetetras.h \
    deletevolumegrid.h \
    dialoglineedit.h \
    dialogoperation.h \
    dolfynwriter.h \
    edgelengthsource.h \
    edgelengthsourcemanager.h \
    eghashset.h \
    egvtkinteractorstyle.h \
    egvtkobject.h \
    elements.h \
    eliminatesmallbranches.h \
    engrid.h \
    engrid_version.h \
    error.h \
    facefinder.h \
    filetemplate.h \
    fixcadgeometry.h \
    fixstl.h \
    foamobject.h \
    foamreader.h \
    foamwriter.h \
    geometrytools.h \
    globalnodegraphinterface.h \
    gmshiooperation.h \
    gmshreader.h \
    gmshwriter.h \
    gridsmoother.h \
    guibrlcadimportdialogue.h \
    guicreateboundarylayer.h \
    guicreatehexcore.h \
    guicreatesurfacemesh.h \
    guicreatevolumemesh.h \
    guideletebadaspecttris.h \
    guidivideboundarylayer.h \
    guiedgelengthsourcebox.h \
    guiedgelengthsourcecone.h \
    guiedgelengthsourcepipe.h \
    guiedgelengthsourcesphere.h \
    guieditboundaryconditions.h \
    guiimproveaspectratio.h \
    guimainwindow.h \
    guimergevolumes.h \
    guimirrormesh.h \
    guinormalextrusion.h \
    guipick.h \
    guiselectboundarycodes.h \
    guisetboundarycode.h \
    guisettingstab.h \
    guisettingsviewer.h \
    guisurfacemesher.h \
    guitransform.h \
    guivolumedelegate.h \
    insertpoints.h \
    iooperation.h \
    iterator.h \
    laplacesmoother.h \
    layeriterator.h \
    localnodegraphinterface.h \
    ../math/linsolve.h \
    ../math/mathvector.h \
    ../math/mathvector_methods.h \
    ../math/mathvector_operators.h \
    ../math/mathvector_structs.h \
    ../math/smallsquarematrix.h \
    mergenodes.h \
    meshpartition.h \
    meshqualityfaceorientation.h \
    meshquality.h \
    multipagewidget.h \
    multipagewidgetpage.h \
    neutralwriter.h \
    nodelayeriterator.h \
    octree.h \
    openfoamcase.h \
    openfoamtools.h \
    operation.h \
    optimisation.h \
    optimisenormalvector.h \
    physicalboundarycondition.h \
    plywriter.h \
    pointfinder.h \
    polydatareader.h \
    polymesh.h \
    polymolecule.h \
    reducedpolydatareader.h \
    reducesurfacetriangulation.h \
    removepoints.h \
    seedsimpleprismaticlayer.h \
    seligairfoilreader.h \
    setboundarycode.h \
    showinfo.h \
    simplefoamwriter.h \
    smoothandswapsurface.h \
    smoothingutilities.h \
    sortablevector.h \
    statistics.h \
    std_connections.h \
    std_includes.h \
    stlreader.h \
    stlwriter.h \
    su2writer.h \
    surfacealgorithm.h \
    surfacemesher.h \
    surfacenodemovementcheck.h \
    surfaceoperation.h \
    swaptriangles.h \
    tauwriter.h \
    text3d.h \
    timer.h \
    triangle.h \
    triangularcadinterface.h \
    tricoord.h \
    tvtkoperation.h \
    uniquevector.h \
    updatedesiredmeshdensity.h \
    updatesurfproj.h \
    utilities.h \
    vertexdelegate.h \
    vertexmeshdensity.h \
    volumedefinition.h \
    vtkEgBoundaryCodesFilter.h \
    vtkEgEliminateShortEdges.h \
    vtkEgExtractVolumeCells.h \
    vtkEgGridFilter.h \
    vtkEgNormalExtrusion.h \
    vtkEgPolyDataToUnstructuredGridFilter.h \
    vtkImplicitPolyData.h \
    vtkreader.h \
    xmlhandler.h \


SOURCES = \
    blenderreader.cpp \
    blenderwriter.cpp \
    boundarycondition.cpp \
    boxselect.cpp \
    brlcadinterface.cpp \
    brlcadreader.cpp \
    cadinterface.cpp \
    celllayeriterator.cpp \
    cellneighbouriterator.cpp \
    cgnswriter.cpp \
    checkforoverlap.cpp \
    checksurfaceintegrity.cpp \
    correctsurfaceorientation.cpp \
    createboundarylayer.cpp \
    createcadtesselation.cpp \
    createhexcore.cpp \
    createvolumemesh.cpp \
    deletecells.cpp \
    deletepickedcell.cpp \
    deletepickedpoint.cpp \
    deletestraynodes.cpp \
    deletetetras.cpp \
    deletevolumegrid.cpp \
    dialoglineedit.cpp \
    dolfynwriter.cpp \
    edgelengthsourcemanager.cpp \
    egvtkinteractorstyle.cpp \
    egvtkobject.cpp \
    elements.cpp \
    eliminatesmallbranches.cpp \
    error.cpp \
    facefinder.cpp \
    filetemplate.cpp \
    fixcadgeometry.cpp \
    fixstl.cpp \
    foamobject.cpp \
    foamreader.cpp \
    foamwriter.cpp \
    geometrytools.cpp \
    gmshiooperation.cpp \
    gmshreader.cpp \
    gmshwriter.cpp \
    gridsmoother.cpp \
    guibrlcadimportdialogue.cpp \
    guicreateboundarylayer.cpp \
    guicreatehexcore.cpp \
    guicreatesurfacemesh.cpp \
    guicreatevolumemesh.cpp \
    guideletebadaspecttris.cpp \
    guidivideboundarylayer.cpp \
    guiedgelengthsourcebox.cpp \
    guiedgelengthsourcecone.cpp \
    guiedgelengthsourcepipe.cpp \
    guiedgelengthsourcesphere.cpp \
    guieditboundaryconditions.cpp \
    guiimproveaspectratio.cpp \
    guimainwindow.cpp \
    guimergevolumes.cpp \
    guimirrormesh.cpp \
    guinormalextrusion.cpp \
    guipick.cpp \
    guiselectboundarycodes.cpp \
    guisetboundarycode.cpp \
    guisettingstab.cpp \
    guisettingsviewer.cpp \
    guisurfacemesher.cpp \
    guitransform.cpp \
    guivolumedelegate.cpp \
    insertpoints.cpp \
    iooperation.cpp \
    iterator.cpp \
    laplacesmoother.cpp \
    layeriterator.cpp \
    mergenodes.cpp \
    meshpartition.cpp \
    meshquality.cpp \
    meshqualityfaceorientation.cpp \
    multipagewidget.cpp \
    multipagewidgetpage.cpp \
    neutralwriter.cpp \
    nodelayeriterator.cpp \
    octree.cpp \
    openfoamcase.cpp \
    openfoamtools.cpp \
    operation.cpp \
    optimisation.cpp \
    optimisenormalvector.cpp \
    physicalboundarycondition.cpp \
    plywriter.cpp \
    pointfinder.cpp \
    polydatareader.cpp \
    polymesh.cpp \
    polymolecule.cpp \
    reducedpolydatareader.cpp \
    reducesurfacetriangulation.cpp \
    removepoints.cpp \
    seedsimpleprismaticlayer.cpp \
    seligairfoilreader.cpp \
    setboundarycode.cpp \
    showinfo.cpp \
    simplefoamwriter.cpp \
    smoothandswapsurface.cpp \
    smoothingutilities.cpp \
    stlreader.cpp \
    stlwriter.cpp \
    su2writer.cpp \
    surfacealgorithm.cpp \
    surfacemesher.cpp \
    surfacenodemovementcheck.cpp \
    surfaceoperation.cpp \
    swaptriangles.cpp \
    tauwriter.cpp \
    text3d.cpp \
    timer.cpp \
    triangle.cpp \
    triangularcadinterface.cpp \
    tricoord.cpp \
    updatedesiredmeshdensity.cpp \
    updatesurfproj.cpp \
    utilities.cpp \
    vertexdelegate.cpp \
    vertexmeshdensity.cpp \
    volumedefinition.cpp \
    vtkEgBoundaryCodesFilter.cxx \
    vtkEgEliminateShortEdges.cxx \
    vtkEgExtractVolumeCells.cxx \
    vtkEgGridFilter.cxx \
    vtkEgNormalExtrusion.cxx \
    vtkEgPolyDataToUnstructuredGridFilter.cxx \
    vtkImplicitPolyData.cpp \
    vtkreader.cpp \
    xmlhandler.cpp \


FORMS = \
    guibooleangeometryoperation.ui \
    guibrlcadimportdialogue.ui \
    guicreateboundarylayer.ui \
    guicreatehexcore.ui \
    guicreatesurfacemesh.ui \
    guicreatevolumemesh.ui \
    guideletebadaspecttris.ui \
    guidivideboundarylayer.ui \
    guiedgelengthsourcebox.ui \
    guiedgelengthsourcecone.ui \
    guiedgelengthsourcepipe.ui \
    guiedgelengthsourcesphere.ui \
    guieditboundaryconditions.ui \
    guiimproveaspectratio.ui \
    guimainwindow.ui \
    guimergevolumes.ui \
    guimirrormesh.ui \
    guinormalextrusion.ui \
    guipick.ui \
    guiselectboundarycodes.ui \
    guisetboundarycode.ui \
    guisurfacemesher.ui \
    guitransform.ui \

