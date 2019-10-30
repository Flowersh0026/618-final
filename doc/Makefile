all: proposal.pdf

proposal.pdf: metadata.md ../README.md
	pandoc $^ -o $@ --pdf-engine=xelatex

clean:
	rm *.pdf

