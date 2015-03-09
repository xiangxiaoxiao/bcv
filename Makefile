all: website

SRC=website-src/

MD_OBJS = index.md installation.md test_slic.md test_tvsegment.md

%.md:
	cat $(SRC)/index-header.txt > $(subst .md,.html, $@)
	python -m markdown -x mathjax $(SRC)/$@ >> $(subst .md,.html, $@)
	cat $(SRC)/index-footer.txt >> $(subst .md,.html, $@)
	echo "website rebuilt on " >> $(subst .md,.html, $@)
	date >> $(subst .md,.html, $@)
	cat $(SRC)/index-footer-end.txt >> $(subst .md,.html, $@)

website: $(MD_OBJS)

#------------------------------------------------------------------------------
clean:
	rm *.html
