
AM_CPPFLAGS = -I${top_builddir} -I${top_srcdir}

this_includedir=${includedir}/${subdir}
this_include_HEADERS = \
    all.hpp \
    andreasenhugelocalvoladapter.hpp \
    andreasenhugevolatilityinterpl.hpp \
    andreasenhugevolatilityadapter.hpp \
    blackconstantvol.hpp \
    blackvariancecurve.hpp \
    blackvariancesurface.hpp \
    blackvoltermstructure.hpp \
    fixedlocalvolsurface.hpp \
    gridmodellocalvolsurface.hpp \
    hestonblackvolsurface.hpp \
    impliedvoltermstructure.hpp \
    localconstantvol.hpp \
    localvolcurve.hpp \
    localvolsurface.hpp \
    fixedlocalvoladapter.hpp \
    localvoltermstructure.hpp \
    noexceptlocalvolsurface.hpp \
    fxblackvolsurface.hpp \
    svifxblackvolsurface.hpp \
    kahalefxblackvolsurface.hpp \
    sabrfxblackvolsurface.hpp

cpp_files = \
    andreasenhugelocalvoladapter.cpp \
    andreasenhugevolatilityinterpl.cpp \
    andreasenhugevolatilityadapter.cpp \
    blackvariancecurve.cpp \
    blackvariancesurface.cpp \
    blackvoltermstructure.cpp \
    fixedlocalvolsurface.cpp \
    gridmodellocalvolsurface.cpp \
    hestonblackvolsurface.cpp \
    localvolsurface.cpp \
    fixedlocalvoladapter.cpp \
    localvoltermstructure.cpp \
    fxblackvolsurface.cpp \
    svifxblackvolsurface.cpp \
    sabrfxblackvolsurface.cpp \
    kahalefxblackvolsurface.cpp \
    noexceptlocalvolsurface.cpp

if UNITY_BUILD

nodist_libEquityFxVol_la_SOURCES = unity.cpp

unity.cpp: Makefile.am
	echo "/* This file is automatically generated; do not edit.     */" > ${srcdir}/$@
	echo "/* Add the files to be included into Makefile.am instead. */" >> ${srcdir}/$@
	echo >> ${srcdir}/$@
	for i in $(cpp_files); do \
		echo "#include \"$$i\"" >> ${srcdir}/$@; \
	done

EXTRA_DIST = $(cpp_files)

else

libEquityFxVol_la_SOURCES = $(cpp_files)

endif

noinst_LTLIBRARIES = libEquityFxVol.la

all.hpp: Makefile.am
	echo "/* This file is automatically generated; do not edit.     */" > ${srcdir}/$@
	echo "/* Add the files to be included into Makefile.am instead. */" >> ${srcdir}/$@
	echo >> ${srcdir}/$@
	for i in $(filter-out all.hpp, $(this_include_HEADERS)); do \
		echo "#include <${subdir}/$$i>" >> ${srcdir}/$@; \
	done
	echo >> ${srcdir}/$@
	subdirs='$(SUBDIRS)'; for i in $$subdirs; do \
		echo "#include <${subdir}/$$i/all.hpp>" >> ${srcdir}/$@; \
	done

