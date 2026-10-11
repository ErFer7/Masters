.PHONY: all
all:
	latexmk -pdf -jobname=output -output-directory=cache -aux-directory=cache -pdflatex="pdflatex -interaction=nonstopmode" -use-make main.tex

.PHONY: clean
clean:
	latexmk -c
	rm -rf cache
	rm -f main.bbl main.loq main.synctex.gz
