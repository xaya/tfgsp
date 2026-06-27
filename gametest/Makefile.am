AM_TESTS_ENVIRONMENT = \
  PYTHONPATH=$(PYTHONPATH):$(top_srcdir)

TEST_LIBRARY = \
  pxtest.py

REGTESTS = \
  godmode.py \
  accounts.py \
  burnsale.py \
  pending.py \
  specialtournamentleave.py \
  specialtournament.py

EXTRA_DIST = $(REGTESTS) $(TEST_LIBRARY) run-test.sh
TESTS = $(REGTESTS)

# Skip the Python integration tests (exit 77) when the Xaya regtest
# environment (xayagametest + xayad) is not available, instead of failing.
LOG_COMPILER = $(srcdir)/run-test.sh
