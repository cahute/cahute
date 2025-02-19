# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS ?=
SPHINXBUILD ?= python -m sphinx
SPHINXAUTOOPTS ?= --ignore "*.kate-swp" --ignore "**/openapi.json"
SPHINXAUTOBUILD ?= python -m sphinx_autobuild
SOURCEDIR = .
BUILDDIR = _build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

install:
	python -m pip install -r requirements.txt

livehtml:
	@$(SPHINXAUTOBUILD) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXAUTOOPTS) $(O)

preview: livehtml

.PHONY: help preview livehtml Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
