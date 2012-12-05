;VTK libs
  File "${VTKBINDIR}\QVTK.DLL"
  File "${VTKBINDIR}\VTKALGLIB.DLL"
  File "${VTKBINDIR}\VTKCOMMON.DLL"
  File "${VTKBINDIR}\VTKDICOMPARSER.DLL"
  File "${VTKBINDIR}\VTKEXOIIC.DLL"
  File "${VTKBINDIR}\VTKEXPAT.DLL"
  File "${VTKBINDIR}\VTKFILTERING.DLL"
  File "${VTKBINDIR}\VTKFREETYPE.DLL"
  File "${VTKBINDIR}\VTKFTGL.DLL"
  File "${VTKBINDIR}\VTKGRAPHICS.DLL"
  File "${VTKBINDIR}\VTKHYBRID.DLL"
  File "${VTKBINDIR}\VTKIMAGING.DLL"
  File "${VTKBINDIR}\VTKINFOVIS.DLL"
  File "${VTKBINDIR}\VTKIO.DLL"
  File "${VTKBINDIR}\VTKJPEG.DLL"
  File "${VTKBINDIR}\VTKLIBXML2.DLL"
  File "${VTKBINDIR}\VTKMETAIO.DLL"
  File "${VTKBINDIR}\VTKNETCDF.DLL"
  File "${VTKBINDIR}\VTKRENDERING.DLL"
  File "${VTKBINDIR}\VTKSYS.DLL"
  File "${VTKBINDIR}\VTKTIFF.DLL"
  File "${VTKBINDIR}\VTKVERDICT.DLL"
  File "${VTKBINDIR}\VTKVIEWS.DLL"
  File "${VTKBINDIR}\VTKWIDGETS.DLL"
  File "${VTKBINDIR}\ZLIB1.DLL"
  File "${VTKBINDIR}\LIBPNG15.DLL"
  File "${VTKBINDIR}\LIBMPI.DLL"
  File "${VTKBINDIR}\LIBMPI_CXX.DLL"
  File "${VTKBINDIR}\LIBOPEN-PAL.DLL"
  File "${VTKBINDIR}\LIBOPEN-RTE.DLL"
  File "${VTKBINDIR}\VTKNETCDF_CXX.DLL"
  File "${VTKBINDIR}\VTKPARALLEL.DLL"
  File "${VTKBINDIR}\VPIC.DLL"
  File "${VTKBINDIR}\COSMO.DLL"

;Qt libs
  File "${QTBINDIR}\QTCORE4.DLL"
  File "${QTBINDIR}\QTGUI4.DLL"
  File "${QTBINDIR}\QTXML4.DLL"
  File "${QTBINDIR}\QTWEBKIT4.DLL"
  File "${QTBINDIR}\phonon4.dll"
  File "${QTBINDIR}\QtXmlPatterns4.dll"
  File "${QTBINDIR}\QtNetwork4.dll"
  File "${QTBINDIR}\QtSql4.dll"
  CreateDirectory "$INSTDIR\plugins"
  CreateDirectory "$INSTDIR\plugins\sqldrivers"
  SetOutPath "$INSTDIR\plugins\sqldrivers"
  File "${QTBINDIR}\..\plugins\sqldrivers\qsqlite4.dll"
  SetOutPath "$INSTDIR"

;MinGW runtime
!ifdef USE_mingw32
  File "${MINGWBINDIR}\MINGWM10.DLL"
!endif

;MSVC 2008 Runtimes
!ifdef USE_VisualCppExpress2008
  File "${VTKLIBDIR}\msvcm90.dll"
  File "${VTKLIBDIR}\msvcp90.dll"
  File "${VTKLIBDIR}\msvcr90.dll"
  File "${VTKLIBDIR}\Microsoft.VC90.CRT.manifest"
!endif

