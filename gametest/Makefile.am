AM_TESTS_ENVIRONMENT = \
  CHARON_PREFIX=$(CHARON_PREFIX) \
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

EXTRA_DIST = $(REGTESTS) $(TEST_LIBRARY)
TESTS = $(REGTESTS)
