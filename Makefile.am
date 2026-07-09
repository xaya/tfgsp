ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -Im4
SUBDIRS = proto proto2 database src

# Determinism gate: forbid raw float/double in the consensus code (src/ +
# database/), which would risk a 1-ULP cross-node divergence -> chain fork.
# Runs as part of `make check` (the Docker build gate), after the subdir suites.
check-local:
	$(PYTHON) $(srcdir)/contrib/check-consensus-determinism.py $(srcdir)

EXTRA_DIST = contrib/check-consensus-determinism.py