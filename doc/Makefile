all: proposal.pdf checkpoint.pdf final.pdf

proposal.pdf: proposal_meta.md proposal.md
	pandoc $^ -o $@ --pdf-engine=xelatex

checkpoint.pdf: checkpoint_meta.md checkpoint.md
	pandoc $^ -o $@ --pdf-engine=xelatex

final.pdf: final_meta.md final.md
	pandoc $^ -o $@ --pdf-engine=xelatex

clean:
	rm *.pdf
