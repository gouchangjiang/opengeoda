#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Debug
CND_DISTDIR=dist

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/Thiessen/Thiessen.o \
	${OBJECTDIR}/Explore/3DPlotView.o \
	${OBJECTDIR}/Regression/DenseMatrix.o \
	${OBJECTDIR}/DialogTools/MapQuantileDlg.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcLagDlg.o \
	${OBJECTDIR}/rc/MyAppResources.o \
	${OBJECTDIR}/Explore/LisaCoordinator.o \
	${OBJECTDIR}/Regression/DiagnosticReport.o \
	${OBJECTDIR}/DialogTools/3DControlPan.o \
	${OBJECTDIR}/DataViewer/DataViewerResizeColDlg.o \
	${OBJECTDIR}/ShapeOperations/shp2gwt.o \
	${OBJECTDIR}/Explore/Geom3D.o \
	${OBJECTDIR}/ShapeOperations/RateSmoothing.o \
	${OBJECTDIR}/ShapeOperations/GeodaWeight.o \
	${OBJECTDIR}/DialogTools/LisaWhat2OpenDlg.o \
	${OBJECTDIR}/DialogTools/PCPDlg.o \
	${OBJECTDIR}/logger.o \
	${OBJECTDIR}/ShapeOperations/GwtWeight.o \
	${OBJECTDIR}/ShapeOperations/ShpFile.o \
	${OBJECTDIR}/DialogTools/WeightCharacterDlg.o \
	${OBJECTDIR}/ShapeOperations/ShapeFileTriplet.o \
	${OBJECTDIR}/Regression/PowerLag.o \
	${OBJECTDIR}/Explore/PCPView.o \
	${OBJECTDIR}/GeneralWxUtils.o \
	${OBJECTDIR}/Regression/PowerSymLag.o \
	${OBJECTDIR}/Explore/CartogramView.o \
	${OBJECTDIR}/Explore/ConditionalView.o \
	${OBJECTDIR}/DialogTools/ASC2SHPDlg.o \
	${OBJECTDIR}/Regression/SparseVector.o \
	${OBJECTDIR}/Generic/HighlightState.o \
	${OBJECTDIR}/DialogTools/RandomizationDlg.o \
	${OBJECTDIR}/DialogTools/VariableSettingsDlg.o \
	${OBJECTDIR}/DialogTools/SelectWeightDlg.o \
	${OBJECTDIR}/DataViewer/DataViewerDeleteColDlg.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcUniDlg.o \
	${OBJECTDIR}/Explore/MoranGView.o \
	${OBJECTDIR}/ShapeOperations/ShapeFileHdr.o \
	${OBJECTDIR}/DataViewer/MergeTableDlg.o \
	${OBJECTDIR}/DialogTools/RateSmootherDlg.o \
	${OBJECTDIR}/Generic/TestScrollWinView.o \
	${OBJECTDIR}/ShapeOperations/WeightsManager.o \
	${OBJECTDIR}/GeoDaConst.o \
	${OBJECTDIR}/Regression/smile2.o \
	${OBJECTDIR}/DataViewer/DbfGridTableBase.o \
	${OBJECTDIR}/DialogTools/GetisOrdChoiceDlg.o \
	${OBJECTDIR}/DialogTools/DBF2SHPDlg.o \
	${OBJECTDIR}/Thiessen/VorUtility.o \
	${OBJECTDIR}/ShapeOperations/Gauss.o \
	${OBJECTDIR}/DialogTools/CreateGridDlg.o \
	${OBJECTDIR}/Explore/LisaBoxView.o \
	${OBJECTDIR}/DialogTools/CCVariableDlg.o \
	${OBJECTDIR}/ShapeOperations/ShapeFile.o \
	${OBJECTDIR}/DialogTools/HistIntervalDlg.o \
	${OBJECTDIR}/DialogTools/PermutationCounterDlg.o \
	${OBJECTDIR}/DialogTools/RangeSelectionDlg.o \
	${OBJECTDIR}/ShapeOperations/DbfFile.o \
	${OBJECTDIR}/Generic/MyShape.o \
	${OBJECTDIR}/DataViewer/DataViewerAddColDlg.o \
	${OBJECTDIR}/Regression/mix.o \
	${OBJECTDIR}/ShapeOperations/GalWeight.o \
	${OBJECTDIR}/DialogTools/MovieControlPan.o \
	${OBJECTDIR}/kNN/kd_search.o \
	${OBJECTDIR}/Explore/MoranScatterPlotView.o \
	${OBJECTDIR}/DialogTools/AddIdVariable.o \
	${OBJECTDIR}/DialogTools/AddCentroidsDlg.o \
	${OBJECTDIR}/TemplateFrame.o \
	${OBJECTDIR}/DialogTools/UserConfigDlg.o \
	${OBJECTDIR}/Regression/SparseRow.o \
	${OBJECTDIR}/DialogTools/RegressionReportDlg.o \
	${OBJECTDIR}/kNN/kd_split.o \
	${OBJECTDIR}/ShapeOperations/shp.o \
	${OBJECTDIR}/mapview.o \
	${OBJECTDIR}/DialogTools/ConditionViewDlg.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcSheetDlg.o \
	${OBJECTDIR}/Explore/GStatCoordinator.o \
	${OBJECTDIR}/Regression/ML_im.o \
	${OBJECTDIR}/DialogTools/ScatterPlotVarsDlg.o \
	${OBJECTDIR}/Regression/SparseMatrix.o \
	${OBJECTDIR}/OpenGeoDa.o \
	${OBJECTDIR}/DialogTools/ProgressDlg.o \
	${OBJECTDIR}/kNN/ANN.o \
	${OBJECTDIR}/DialogTools/3DDlg.o \
	${OBJECTDIR}/ShapeOperations/shp2cnt.o \
	${OBJECTDIR}/DialogTools/RegressionDlg.o \
	${OBJECTDIR}/DialogTools/ThiessenPolygonDlg.o \
	${OBJECTDIR}/GenGeomAlgs.o \
	${OBJECTDIR}/kNN/kd_pr_search.o \
	${OBJECTDIR}/Project.o \
	${OBJECTDIR}/GenUtils.o \
	${OBJECTDIR}/Explore/HistView.o \
	${OBJECTDIR}/DataViewer/DataViewerEditFieldPropertiesDlg.o \
	${OBJECTDIR}/Explore/ScatterPlotView.o \
	${OBJECTDIR}/kNN/kd_util.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcSpecialDlg.o \
	${OBJECTDIR}/ShapeOperations/Randik.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcRateDlg.o \
	${OBJECTDIR}/DialogTools/Statistics.o \
	${OBJECTDIR}/Explore/GetisOrdMapView.o \
	${OBJECTDIR}/NewTableViewer.o \
	${OBJECTDIR}/DialogTools/SaveToTableDlg.o \
	${OBJECTDIR}/TemplateLegend.o \
	${OBJECTDIR}/FramesManager.o \
	${OBJECTDIR}/DialogTools/FieldNewCalcBinDlg.o \
	${OBJECTDIR}/ShapeOperations/Box.o \
	${OBJECTDIR}/Regression/Weights.o \
	${OBJECTDIR}/DataViewer/DataViewer.o \
	${OBJECTDIR}/DialogTools/SHP2ASCDlg.o \
	${OBJECTDIR}/DialogTools/Bnd2ShpDlg.o \
	${OBJECTDIR}/kNN/kNN.o \
	${OBJECTDIR}/Explore/LisaMapView.o \
	${OBJECTDIR}/Regression/DenseVector.o \
	${OBJECTDIR}/TemplateCanvas.o \
	${OBJECTDIR}/ShapeOperations/DBF.o \
	${OBJECTDIR}/kNN/kd_tree.o \
	${OBJECTDIR}/DialogTools/RegressionTitleDlg.o \
	${OBJECTDIR}/DialogTools/SaveSelectionDlg.o \
	${OBJECTDIR}/DialogTools/CreatingWeightDlg.o \
	${OBJECTDIR}/Explore/BoxPlotView.o \
	${OBJECTDIR}/ShapeOperations/BasePoint.o \
	${OBJECTDIR}/ShapeOperations/AbstractShape.o \
	${OBJECTDIR}/DialogTools/Dbf2GaussDlg.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-pthread -g
CXXFLAGS=-pthread -g

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L/usr/local/lib -pthread ${HOME}/CLAPACK-3.2/lapack.a ${HOME}/CLAPACK-3.2/blas.a ${HOME}/CLAPACK-3.2/F2CLIBS/libf2c.a /usr/local/lib/libwx_gtk2u_gl-2.9.a -lGL -lGLU /usr/local/lib/libwx_gtk2u_richtext-2.9.a /usr/local/lib/libwx_gtk2u_xrc-2.9.a /usr/local/lib/libwx_gtk2u_qa-2.9.a /usr/local/lib/libwx_gtk2u_html-2.9.a /usr/local/lib/libwx_gtk2u_adv-2.9.a /usr/local/lib/libwx_gtk2u_core-2.9.a /usr/local/lib/libwx_baseu_xml-2.9.a /usr/local/lib/libwx_baseu_net-2.9.a /usr/local/lib/libwx_baseu-2.9.a -pthread -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lpangoft2-1.0 -lgdk_pixbuf-2.0 -lfreetype -lfontconfig -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lglib-2.0 -lXinerama -lSM -lwxtiff-2.9 -lwxjpeg-2.9 -lwxpng-2.9 -lexpat -lz -ldl -lm

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-Debug.mk dist/Debug/GNU-Linux-x86/alphatrunk

dist/Debug/GNU-Linux-x86/alphatrunk: ${HOME}/CLAPACK-3.2/lapack.a

dist/Debug/GNU-Linux-x86/alphatrunk: ${HOME}/CLAPACK-3.2/blas.a

dist/Debug/GNU-Linux-x86/alphatrunk: ${HOME}/CLAPACK-3.2/F2CLIBS/libf2c.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_gtk2u_gl-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_gtk2u_richtext-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_gtk2u_xrc-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_gtk2u_qa-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_gtk2u_html-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_gtk2u_adv-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_gtk2u_core-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_baseu_xml-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_baseu_net-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: /usr/local/lib/libwx_baseu-2.9.a

dist/Debug/GNU-Linux-x86/alphatrunk: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/GNU-Linux-x86
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/alphatrunk ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/Thiessen/Thiessen.o: Thiessen/Thiessen.cpp 
	${MKDIR} -p ${OBJECTDIR}/Thiessen
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Thiessen/Thiessen.o Thiessen/Thiessen.cpp

${OBJECTDIR}/Explore/3DPlotView.o: Explore/3DPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/3DPlotView.o Explore/3DPlotView.cpp

${OBJECTDIR}/Regression/DenseMatrix.o: Regression/DenseMatrix.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/DenseMatrix.o Regression/DenseMatrix.cpp

${OBJECTDIR}/DialogTools/MapQuantileDlg.o: DialogTools/MapQuantileDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/MapQuantileDlg.o DialogTools/MapQuantileDlg.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcLagDlg.o: DialogTools/FieldNewCalcLagDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcLagDlg.o DialogTools/FieldNewCalcLagDlg.cpp

${OBJECTDIR}/rc/MyAppResources.o: rc/MyAppResources.cpp 
	${MKDIR} -p ${OBJECTDIR}/rc
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/rc/MyAppResources.o rc/MyAppResources.cpp

${OBJECTDIR}/Explore/LisaCoordinator.o: Explore/LisaCoordinator.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/LisaCoordinator.o Explore/LisaCoordinator.cpp

${OBJECTDIR}/Regression/DiagnosticReport.o: Regression/DiagnosticReport.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/DiagnosticReport.o Regression/DiagnosticReport.cpp

${OBJECTDIR}/DialogTools/3DControlPan.o: DialogTools/3DControlPan.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/3DControlPan.o DialogTools/3DControlPan.cpp

${OBJECTDIR}/DataViewer/DataViewerResizeColDlg.o: DataViewer/DataViewerResizeColDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerResizeColDlg.o DataViewer/DataViewerResizeColDlg.cpp

${OBJECTDIR}/ShapeOperations/shp2gwt.o: ShapeOperations/shp2gwt.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/shp2gwt.o ShapeOperations/shp2gwt.cpp

${OBJECTDIR}/Explore/Geom3D.o: Explore/Geom3D.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/Geom3D.o Explore/Geom3D.cpp

${OBJECTDIR}/ShapeOperations/RateSmoothing.o: ShapeOperations/RateSmoothing.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/RateSmoothing.o ShapeOperations/RateSmoothing.cpp

${OBJECTDIR}/ShapeOperations/GeodaWeight.o: ShapeOperations/GeodaWeight.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/GeodaWeight.o ShapeOperations/GeodaWeight.cpp

${OBJECTDIR}/DialogTools/LisaWhat2OpenDlg.o: DialogTools/LisaWhat2OpenDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/LisaWhat2OpenDlg.o DialogTools/LisaWhat2OpenDlg.cpp

${OBJECTDIR}/DialogTools/PCPDlg.o: DialogTools/PCPDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/PCPDlg.o DialogTools/PCPDlg.cpp

${OBJECTDIR}/logger.o: logger.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/logger.o logger.cpp

${OBJECTDIR}/ShapeOperations/GwtWeight.o: ShapeOperations/GwtWeight.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/GwtWeight.o ShapeOperations/GwtWeight.cpp

${OBJECTDIR}/ShapeOperations/ShpFile.o: ShapeOperations/ShpFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShpFile.o ShapeOperations/ShpFile.cpp

${OBJECTDIR}/DialogTools/WeightCharacterDlg.o: DialogTools/WeightCharacterDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/WeightCharacterDlg.o DialogTools/WeightCharacterDlg.cpp

${OBJECTDIR}/ShapeOperations/ShapeFileTriplet.o: ShapeOperations/ShapeFileTriplet.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShapeFileTriplet.o ShapeOperations/ShapeFileTriplet.cpp

${OBJECTDIR}/Regression/PowerLag.o: Regression/PowerLag.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/PowerLag.o Regression/PowerLag.cpp

${OBJECTDIR}/Explore/PCPView.o: Explore/PCPView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/PCPView.o Explore/PCPView.cpp

${OBJECTDIR}/GeneralWxUtils.o: GeneralWxUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GeneralWxUtils.o GeneralWxUtils.cpp

${OBJECTDIR}/Regression/PowerSymLag.o: Regression/PowerSymLag.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/PowerSymLag.o Regression/PowerSymLag.cpp

${OBJECTDIR}/Explore/CartogramView.o: Explore/CartogramView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/CartogramView.o Explore/CartogramView.cpp

${OBJECTDIR}/Explore/ConditionalView.o: Explore/ConditionalView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ConditionalView.o Explore/ConditionalView.cpp

${OBJECTDIR}/DialogTools/ASC2SHPDlg.o: DialogTools/ASC2SHPDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ASC2SHPDlg.o DialogTools/ASC2SHPDlg.cpp

${OBJECTDIR}/Regression/SparseVector.o: Regression/SparseVector.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/SparseVector.o Regression/SparseVector.cpp

${OBJECTDIR}/Generic/HighlightState.o: Generic/HighlightState.cpp 
	${MKDIR} -p ${OBJECTDIR}/Generic
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Generic/HighlightState.o Generic/HighlightState.cpp

${OBJECTDIR}/DialogTools/RandomizationDlg.o: DialogTools/RandomizationDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RandomizationDlg.o DialogTools/RandomizationDlg.cpp

${OBJECTDIR}/DialogTools/VariableSettingsDlg.o: DialogTools/VariableSettingsDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/VariableSettingsDlg.o DialogTools/VariableSettingsDlg.cpp

${OBJECTDIR}/DialogTools/SelectWeightDlg.o: DialogTools/SelectWeightDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SelectWeightDlg.o DialogTools/SelectWeightDlg.cpp

${OBJECTDIR}/DataViewer/DataViewerDeleteColDlg.o: DataViewer/DataViewerDeleteColDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerDeleteColDlg.o DataViewer/DataViewerDeleteColDlg.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcUniDlg.o: DialogTools/FieldNewCalcUniDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcUniDlg.o DialogTools/FieldNewCalcUniDlg.cpp

${OBJECTDIR}/Explore/MoranGView.o: Explore/MoranGView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/MoranGView.o Explore/MoranGView.cpp

${OBJECTDIR}/ShapeOperations/ShapeFileHdr.o: ShapeOperations/ShapeFileHdr.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShapeFileHdr.o ShapeOperations/ShapeFileHdr.cpp

${OBJECTDIR}/DataViewer/MergeTableDlg.o: DataViewer/MergeTableDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/MergeTableDlg.o DataViewer/MergeTableDlg.cpp

${OBJECTDIR}/DialogTools/RateSmootherDlg.o: DialogTools/RateSmootherDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RateSmootherDlg.o DialogTools/RateSmootherDlg.cpp

${OBJECTDIR}/Generic/TestScrollWinView.o: Generic/TestScrollWinView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Generic
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Generic/TestScrollWinView.o Generic/TestScrollWinView.cpp

${OBJECTDIR}/ShapeOperations/WeightsManager.o: ShapeOperations/WeightsManager.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/WeightsManager.o ShapeOperations/WeightsManager.cpp

${OBJECTDIR}/GeoDaConst.o: GeoDaConst.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GeoDaConst.o GeoDaConst.cpp

${OBJECTDIR}/Regression/smile2.o: Regression/smile2.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/smile2.o Regression/smile2.cpp

${OBJECTDIR}/DataViewer/DbfGridTableBase.o: DataViewer/DbfGridTableBase.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DbfGridTableBase.o DataViewer/DbfGridTableBase.cpp

${OBJECTDIR}/DialogTools/GetisOrdChoiceDlg.o: DialogTools/GetisOrdChoiceDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/GetisOrdChoiceDlg.o DialogTools/GetisOrdChoiceDlg.cpp

${OBJECTDIR}/DialogTools/DBF2SHPDlg.o: DialogTools/DBF2SHPDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/DBF2SHPDlg.o DialogTools/DBF2SHPDlg.cpp

${OBJECTDIR}/Thiessen/VorUtility.o: Thiessen/VorUtility.cpp 
	${MKDIR} -p ${OBJECTDIR}/Thiessen
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Thiessen/VorUtility.o Thiessen/VorUtility.cpp

${OBJECTDIR}/ShapeOperations/Gauss.o: ShapeOperations/Gauss.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/Gauss.o ShapeOperations/Gauss.cpp

${OBJECTDIR}/DialogTools/CreateGridDlg.o: DialogTools/CreateGridDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/CreateGridDlg.o DialogTools/CreateGridDlg.cpp

${OBJECTDIR}/Explore/LisaBoxView.o: Explore/LisaBoxView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/LisaBoxView.o Explore/LisaBoxView.cpp

${OBJECTDIR}/DialogTools/CCVariableDlg.o: DialogTools/CCVariableDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/CCVariableDlg.o DialogTools/CCVariableDlg.cpp

${OBJECTDIR}/ShapeOperations/ShapeFile.o: ShapeOperations/ShapeFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/ShapeFile.o ShapeOperations/ShapeFile.cpp

${OBJECTDIR}/DialogTools/HistIntervalDlg.o: DialogTools/HistIntervalDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/HistIntervalDlg.o DialogTools/HistIntervalDlg.cpp

${OBJECTDIR}/DialogTools/PermutationCounterDlg.o: DialogTools/PermutationCounterDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/PermutationCounterDlg.o DialogTools/PermutationCounterDlg.cpp

${OBJECTDIR}/DialogTools/RangeSelectionDlg.o: DialogTools/RangeSelectionDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RangeSelectionDlg.o DialogTools/RangeSelectionDlg.cpp

${OBJECTDIR}/ShapeOperations/DbfFile.o: ShapeOperations/DbfFile.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/DbfFile.o ShapeOperations/DbfFile.cpp

${OBJECTDIR}/Generic/MyShape.o: Generic/MyShape.cpp 
	${MKDIR} -p ${OBJECTDIR}/Generic
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Generic/MyShape.o Generic/MyShape.cpp

${OBJECTDIR}/DataViewer/DataViewerAddColDlg.o: DataViewer/DataViewerAddColDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerAddColDlg.o DataViewer/DataViewerAddColDlg.cpp

${OBJECTDIR}/Regression/mix.o: Regression/mix.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/mix.o Regression/mix.cpp

${OBJECTDIR}/ShapeOperations/GalWeight.o: ShapeOperations/GalWeight.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/GalWeight.o ShapeOperations/GalWeight.cpp

${OBJECTDIR}/DialogTools/MovieControlPan.o: DialogTools/MovieControlPan.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/MovieControlPan.o DialogTools/MovieControlPan.cpp

${OBJECTDIR}/kNN/kd_search.o: kNN/kd_search.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_search.o kNN/kd_search.cpp

${OBJECTDIR}/Explore/MoranScatterPlotView.o: Explore/MoranScatterPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/MoranScatterPlotView.o Explore/MoranScatterPlotView.cpp

${OBJECTDIR}/DialogTools/AddIdVariable.o: DialogTools/AddIdVariable.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/AddIdVariable.o DialogTools/AddIdVariable.cpp

${OBJECTDIR}/DialogTools/AddCentroidsDlg.o: DialogTools/AddCentroidsDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/AddCentroidsDlg.o DialogTools/AddCentroidsDlg.cpp

${OBJECTDIR}/TemplateFrame.o: TemplateFrame.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/TemplateFrame.o TemplateFrame.cpp

${OBJECTDIR}/DialogTools/UserConfigDlg.o: DialogTools/UserConfigDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/UserConfigDlg.o DialogTools/UserConfigDlg.cpp

${OBJECTDIR}/Regression/SparseRow.o: Regression/SparseRow.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/SparseRow.o Regression/SparseRow.cpp

${OBJECTDIR}/DialogTools/RegressionReportDlg.o: DialogTools/RegressionReportDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RegressionReportDlg.o DialogTools/RegressionReportDlg.cpp

${OBJECTDIR}/kNN/kd_split.o: kNN/kd_split.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_split.o kNN/kd_split.cpp

${OBJECTDIR}/ShapeOperations/shp.o: ShapeOperations/shp.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/shp.o ShapeOperations/shp.cpp

${OBJECTDIR}/mapview.o: mapview.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/mapview.o mapview.cpp

${OBJECTDIR}/DialogTools/ConditionViewDlg.o: DialogTools/ConditionViewDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ConditionViewDlg.o DialogTools/ConditionViewDlg.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcSheetDlg.o: DialogTools/FieldNewCalcSheetDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcSheetDlg.o DialogTools/FieldNewCalcSheetDlg.cpp

${OBJECTDIR}/Explore/GStatCoordinator.o: Explore/GStatCoordinator.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/GStatCoordinator.o Explore/GStatCoordinator.cpp

${OBJECTDIR}/Regression/ML_im.o: Regression/ML_im.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/ML_im.o Regression/ML_im.cpp

${OBJECTDIR}/DialogTools/ScatterPlotVarsDlg.o: DialogTools/ScatterPlotVarsDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ScatterPlotVarsDlg.o DialogTools/ScatterPlotVarsDlg.cpp

${OBJECTDIR}/Regression/SparseMatrix.o: Regression/SparseMatrix.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/SparseMatrix.o Regression/SparseMatrix.cpp

${OBJECTDIR}/OpenGeoDa.o: OpenGeoDa.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/OpenGeoDa.o OpenGeoDa.cpp

${OBJECTDIR}/DialogTools/ProgressDlg.o: DialogTools/ProgressDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ProgressDlg.o DialogTools/ProgressDlg.cpp

${OBJECTDIR}/kNN/ANN.o: kNN/ANN.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/ANN.o kNN/ANN.cpp

${OBJECTDIR}/DialogTools/3DDlg.o: DialogTools/3DDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/3DDlg.o DialogTools/3DDlg.cpp

${OBJECTDIR}/ShapeOperations/shp2cnt.o: ShapeOperations/shp2cnt.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/shp2cnt.o ShapeOperations/shp2cnt.cpp

${OBJECTDIR}/DialogTools/RegressionDlg.o: DialogTools/RegressionDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RegressionDlg.o DialogTools/RegressionDlg.cpp

${OBJECTDIR}/DialogTools/ThiessenPolygonDlg.o: DialogTools/ThiessenPolygonDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/ThiessenPolygonDlg.o DialogTools/ThiessenPolygonDlg.cpp

${OBJECTDIR}/GenGeomAlgs.o: GenGeomAlgs.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GenGeomAlgs.o GenGeomAlgs.cpp

${OBJECTDIR}/kNN/kd_pr_search.o: kNN/kd_pr_search.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_pr_search.o kNN/kd_pr_search.cpp

${OBJECTDIR}/Project.o: Project.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Project.o Project.cpp

${OBJECTDIR}/GenUtils.o: GenUtils.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/GenUtils.o GenUtils.cpp

${OBJECTDIR}/Explore/HistView.o: Explore/HistView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/HistView.o Explore/HistView.cpp

${OBJECTDIR}/DataViewer/DataViewerEditFieldPropertiesDlg.o: DataViewer/DataViewerEditFieldPropertiesDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewerEditFieldPropertiesDlg.o DataViewer/DataViewerEditFieldPropertiesDlg.cpp

${OBJECTDIR}/Explore/ScatterPlotView.o: Explore/ScatterPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/ScatterPlotView.o Explore/ScatterPlotView.cpp

${OBJECTDIR}/kNN/kd_util.o: kNN/kd_util.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_util.o kNN/kd_util.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcSpecialDlg.o: DialogTools/FieldNewCalcSpecialDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcSpecialDlg.o DialogTools/FieldNewCalcSpecialDlg.cpp

${OBJECTDIR}/ShapeOperations/Randik.o: ShapeOperations/Randik.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/Randik.o ShapeOperations/Randik.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcRateDlg.o: DialogTools/FieldNewCalcRateDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcRateDlg.o DialogTools/FieldNewCalcRateDlg.cpp

${OBJECTDIR}/DialogTools/Statistics.o: DialogTools/Statistics.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/Statistics.o DialogTools/Statistics.cpp

${OBJECTDIR}/Explore/GetisOrdMapView.o: Explore/GetisOrdMapView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/GetisOrdMapView.o Explore/GetisOrdMapView.cpp

${OBJECTDIR}/NewTableViewer.o: NewTableViewer.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/NewTableViewer.o NewTableViewer.cpp

${OBJECTDIR}/DialogTools/SaveToTableDlg.o: DialogTools/SaveToTableDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SaveToTableDlg.o DialogTools/SaveToTableDlg.cpp

${OBJECTDIR}/TemplateLegend.o: TemplateLegend.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/TemplateLegend.o TemplateLegend.cpp

${OBJECTDIR}/FramesManager.o: FramesManager.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/FramesManager.o FramesManager.cpp

${OBJECTDIR}/DialogTools/FieldNewCalcBinDlg.o: DialogTools/FieldNewCalcBinDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/FieldNewCalcBinDlg.o DialogTools/FieldNewCalcBinDlg.cpp

${OBJECTDIR}/ShapeOperations/Box.o: ShapeOperations/Box.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/Box.o ShapeOperations/Box.cpp

${OBJECTDIR}/Regression/Weights.o: Regression/Weights.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/Weights.o Regression/Weights.cpp

${OBJECTDIR}/DataViewer/DataViewer.o: DataViewer/DataViewer.cpp 
	${MKDIR} -p ${OBJECTDIR}/DataViewer
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DataViewer/DataViewer.o DataViewer/DataViewer.cpp

${OBJECTDIR}/DialogTools/SHP2ASCDlg.o: DialogTools/SHP2ASCDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SHP2ASCDlg.o DialogTools/SHP2ASCDlg.cpp

${OBJECTDIR}/DialogTools/Bnd2ShpDlg.o: DialogTools/Bnd2ShpDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/Bnd2ShpDlg.o DialogTools/Bnd2ShpDlg.cpp

${OBJECTDIR}/kNN/kNN.o: kNN/kNN.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kNN.o kNN/kNN.cpp

${OBJECTDIR}/Explore/LisaMapView.o: Explore/LisaMapView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/LisaMapView.o Explore/LisaMapView.cpp

${OBJECTDIR}/Regression/DenseVector.o: Regression/DenseVector.cpp 
	${MKDIR} -p ${OBJECTDIR}/Regression
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Regression/DenseVector.o Regression/DenseVector.cpp

${OBJECTDIR}/TemplateCanvas.o: TemplateCanvas.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/TemplateCanvas.o TemplateCanvas.cpp

${OBJECTDIR}/ShapeOperations/DBF.o: ShapeOperations/DBF.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/DBF.o ShapeOperations/DBF.cpp

${OBJECTDIR}/kNN/kd_tree.o: kNN/kd_tree.cpp 
	${MKDIR} -p ${OBJECTDIR}/kNN
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/kNN/kd_tree.o kNN/kd_tree.cpp

${OBJECTDIR}/DialogTools/RegressionTitleDlg.o: DialogTools/RegressionTitleDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/RegressionTitleDlg.o DialogTools/RegressionTitleDlg.cpp

${OBJECTDIR}/DialogTools/SaveSelectionDlg.o: DialogTools/SaveSelectionDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/SaveSelectionDlg.o DialogTools/SaveSelectionDlg.cpp

${OBJECTDIR}/DialogTools/CreatingWeightDlg.o: DialogTools/CreatingWeightDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/CreatingWeightDlg.o DialogTools/CreatingWeightDlg.cpp

${OBJECTDIR}/Explore/BoxPlotView.o: Explore/BoxPlotView.cpp 
	${MKDIR} -p ${OBJECTDIR}/Explore
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/Explore/BoxPlotView.o Explore/BoxPlotView.cpp

${OBJECTDIR}/ShapeOperations/BasePoint.o: ShapeOperations/BasePoint.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/BasePoint.o ShapeOperations/BasePoint.cpp

${OBJECTDIR}/ShapeOperations/AbstractShape.o: ShapeOperations/AbstractShape.cpp 
	${MKDIR} -p ${OBJECTDIR}/ShapeOperations
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/ShapeOperations/AbstractShape.o ShapeOperations/AbstractShape.cpp

${OBJECTDIR}/DialogTools/Dbf2GaussDlg.o: DialogTools/Dbf2GaussDlg.cpp 
	${MKDIR} -p ${OBJECTDIR}/DialogTools
	${RM} $@.d
	$(COMPILE.cc) -g -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D__WXGTK__ -D__WXDEBUG__ -DDEBUG -I/usr/local/lib/wx/include/gtk2-unicode-static-2.9 -I/usr/local/include/wx-2.9 -I/usr/local/include/boost -MMD -MP -MF $@.d -o ${OBJECTDIR}/DialogTools/Dbf2GaussDlg.o DialogTools/Dbf2GaussDlg.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Debug
	${RM} dist/Debug/GNU-Linux-x86/alphatrunk

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
