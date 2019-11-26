all: proposal.pdf checkpoint.pdf

proposal.pdf: proposal_meta.md proposal.md
	pandoc $^ -o $@ --pdf-engine=xelatex

checkpoint.pdf: checkpoint_meta.md checkpoint.md
	pandoc $^ -o $@ --pdf-engine=xelatex

clean:
	rm *.pdf

