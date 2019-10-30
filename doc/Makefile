all: proposal.pdf

proposal.pdf: metadata.md proposal.md
	pandoc $^ -o $@ --pdf-engine=xelatex

clean:
	rm *.pdf

