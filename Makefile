all: website

SRC=website-src/

MD_OBJS = index.md test_slic.md test_tvsegment.md

%.md:
	cat $(SRC)/index-header.txt > $(subst .md,.html, $@)
	markdown $(SRC)/$@ >> $(subst .md,.html, $@)
	cat $(SRC)/index-footer.txt >> $(subst .md,.html, $@)

website: $(MD_OBJS)

#------------------------------------------------------------------------------
clean:
	rm *.html
