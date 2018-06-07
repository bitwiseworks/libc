#
# /emx/src/doc/makefile
#

BOOK=\emx\book\ #
DOC=\emx\doc\ #

default:	all

all:	docs books ndx

docs:	$(DOC)emxrt.doc $(DOC)emxdev.doc $(DOC)emxlib.doc \
    $(DOC)emxgnu.doc $(DOC)emxbsd.doc

books:	$(BOOK)emxrt.inf $(BOOK)emxdev.inf $(BOOK)emxlib.inf \
    $(BOOK)emxgnu.inf $(BOOK)emxbsd.inf

ndx:	$(BOOK)emxdev.ndx $(BOOK)emxlib.ndx $(BOOK)emxgnu.ndx \
    $(BOOK)emxbsd.ndx $(BOOK)emxbook.ndx

emxrt: $(DOC)emxrt.doc $(BOOK)emxrt.inf
emxdev: $(DOC)emxdev.doc $(BOOK)emxdev.inf $(BOOK)emxdev.ndx
emxlib: $(DOC)emxlib.doc $(BOOK)emxlib.inf $(BOOK)emxlib.ndx
emxgnu: $(DOC)emxgnu.doc $(BOOK)emxgnu.inf $(BOOK)emxgnu.ndx
emxbsd: $(DOC)emxbsd.doc $(BOOK)emxbsd.inf $(BOOK)emxbsd.ndx


$(DOC)emxrt.doc:	emxrt.src
	emxdoc -To $(DOC)emxrt.doc emxrt.src

$(DOC)emxdev.doc:	emxdev.src
	emxdoc -To $(DOC)emxdev.doc emxdev.src

$(DOC)emxlib.doc:	emxlib.src
	emxdoc -To $(DOC)emxlib.doc emxlib.src

$(DOC)emxgnu.doc:	emxgnu.src
	emxdoc -To $(DOC)emxgnu.doc emxgnu.src

$(DOC)emxbsd.doc:	emxbsd.src
	emxdoc -To $(DOC)emxbsd.doc emxbsd.src


$(BOOK)emxdev.ndx: emxdev.src
	emxdoc -Ko $(BOOK)emxdev.ndx emxdev.src

$(BOOK)emxlib.ndx: emxlib.src
	emxdoc -Ko $(BOOK)emxlib.ndx emxlib.src

$(BOOK)emxgnu.ndx: emxgnu.src
	emxdoc -Ko $(BOOK)emxgnu.ndx emxgnu.src

$(BOOK)emxbsd.ndx: emxbsd.src
	emxdoc -Ko $(BOOK)emxbsd.ndx emxbsd.src

$(BOOK)emxbook.ndx: $(BOOK)emxdev.ndx $(BOOK)emxlib.ndx $(BOOK)emxgnu.ndx \
    $(BOOK)emxbsd.ndx emxbook.sed
	sed -n -f emxbook.sed $(BOOK)emxdev.ndx $(BOOK)emxlib.ndx $(BOOK)emxgnu.ndx $(BOOK)emxbsd.ndx >$(BOOK)emxbook.ndx

$(BOOK)emxrt.inf: emxrt.ipf
	-del $(BOOK)emxrt.inf
	ipfc emxrt.ipf /inf
	move emxrt.inf $(BOOK)emxrt.inf

$(BOOK)emxdev.inf: emxdev.ipf
	-del $(BOOK)emxdev.inf
	ipfc emxdev.ipf /inf
	move emxdev.inf $(BOOK)emxdev.inf

$(BOOK)emxgnu.inf: emxgnu.ipf
	-del $(BOOK)emxgnu.inf
	ipfc emxgnu.ipf /inf
	move emxgnu.inf $(BOOK)emxgnu.inf

$(BOOK)emxbsd.inf: emxbsd.ipf
	-del $(BOOK)emxbsd.inf
	ipfc emxbsd.ipf /inf
	move emxbsd.inf $(BOOK)emxbsd.inf

$(BOOK)emxlib.inf: emxlib.ipf
	-del $(BOOK)emxlib.inf
	ipfc emxlib.ipf /inf
	move emxlib.inf $(BOOK)emxlib.inf

emxrt.ipf:	emxrt.src
	emxdoc -Ic -icp850 -o emxrt.ipf emxrt.src

emxdev.ipf:	emxdev.src emxbook.dir
	emxdoc -Iac -n1000 -icp850 -o emxdev.ipf -x emxbook.dir emxdev.src

emxgnu.ipf:	emxgnu.src emxbook.dir
	emxdoc -Iac -n2000 -o emxgnu.ipf -x emxbook.dir emxgnu.src

emxbsd.ipf:	emxbsd.src emxbook.dir
	emxdoc -Iac -n3000 -o emxbsd.ipf -x emxbook.dir emxbsd.src

emxlib.ipf:	emxlib.src emxbook.dir
	emxdoc -Iac -n1 -o emxlib.ipf -x emxbook.dir emxlib.src

emxbook.dir:	st-dir
st-dir:		emxdev.dir emxgnu.dir emxbsd.dir emxlib.dir
	emxdoc -Mo tmp emxdev.dir emxgnu.dir emxbsd.dir emxlib.dir
	updt tmp emxbook.dir
	del tmp
	touch st-dir

emxdev.dir:	emxdev.src
	emxdoc -Ig -n1000 -icp850 -o emxdev.dir emxdev.src

emxgnu.dir:	emxgnu.src
	emxdoc -Ig -n2000 -o emxgnu.dir emxgnu.src

emxbsd.dir:	emxbsd.src
	emxdoc -Ig -n3000 -o emxbsd.dir emxbsd.src

emxlib.dir:	emxlib.src
	emxdoc -Ig -n1 -o emxlib.dir emxlib.src

clean:
	-del *.dir
	-del *.ipf
	-del $(BOOK)*.ndx
	-del $(DOC)emxrt.doc
	-del $(DOC)emxdev.doc
	-del $(BOOK)emxrt.inf
	-del $(BOOK)emxdev.inf
	-del $(DOC)emxgnu.doc
	-del $(BOOK)emxgnu.inf
	-del $(DOC)emxbsd.doc
	-del $(BOOK)emxbsd.inf
	-del $(DOC)emxlib.doc
	-del $(BOOK)emxlib.inf
	-del st-dir
