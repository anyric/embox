/**
 * @file
 * @brief TODO
 *
 * @date 10.03.2010
 * @author Eldar Abusalimov
 */

#include <errno.h>
#include <common.h>

#include <embox/runlevel.h>
#include <embox/mod.h>

#define __RUNLEVEL_MOD_DEF(op, nr) \
	MOD_DEF(runlevel##nr##_##op, generic, "runlevel"#nr"_"#op); \
	MOD_DATA_DEF(runlevel##nr##_##op, nr); \
	MOD_OPS_DEF(runlevel##nr##_##op, &__runlevel_##op##_mod_ops)

#define RUNLEVEL_DEF(nr) \
	__RUNLEVEL_MOD_DEF(init, nr); \
	__RUNLEVEL_MOD_DEF(fini, nr)

#define RUNLEVEL(nr) { \
		.init_mod = MOD_PTR(runlevel##nr##_##init), \
		.fini_mod = MOD_PTR(runlevel##nr##_##fini), \
	}

static int init_mod_enable(void *data);
static int init_mod_disable(void *data);
static int fini_mod_enable(void *data);
static int fini_mod_disable(void *data);

struct mod_ops __runlevel_init_mod_ops = { .enable = init_mod_enable,
		.disable = init_mod_disable };
struct mod_ops __runlevel_fini_mod_ops = { .enable = fini_mod_enable,
		.disable = fini_mod_disable };

RUNLEVEL_DEF(0);
RUNLEVEL_DEF(1);
RUNLEVEL_DEF(2);
RUNLEVEL_DEF(3);

static const struct runlevel runlevels[RUNLEVEL_NRS_TOTAL] = { RUNLEVEL(0),
		RUNLEVEL(1), RUNLEVEL(2), RUNLEVEL(3), };

static runlevel_nr_t init_level = -1, fini_level = -1;

static int init_mod_enable(void *data) {
	int ret;
	int level = (runlevel_nr_t) data;

	if (runlevel_nr_valid(level - 1) && 0 != (ret = mod_enable(runlevels[level
			- 1].init_mod))) {
		return ret;
	}
	init_level = level;
	TRACE("runlevel: init level is %d\n", init_level);

	return 0;
}

static int init_mod_disable(void *data) {
	int ret;
	int level = (runlevel_nr_t) data;

	if (runlevel_nr_valid(level + 1) && 0 != (ret = mod_disable(runlevels[level
			+ 1].init_mod))) {
		return ret;
	}
	init_level = level - 1;
	TRACE("runlevel: init level is %d\n", init_level);

	return 0;
}

static int fini_mod_enable(void *data) {
	int ret;
	int level = (runlevel_nr_t) data;

	if (runlevel_nr_valid(level - 1) && 0 != (ret = mod_enable(runlevels[level
			- 1].fini_mod))) {
		return ret;
	}
	fini_level = level;

	return 0;
}

static int fini_mod_disable(void *data) {
	int ret;
	int level = (runlevel_nr_t) data;

	if (runlevel_nr_valid(level + 1) && 0 != (ret = mod_disable(runlevels[level
			+ 1].fini_mod))) {
		return ret;
	}
	fini_level = level - 1;

	return 0;
}

runlevel_nr_t runlevel_get_entered(void) {
	return init_level;
}

runlevel_nr_t runlevel_get_leaved(void) {
	return fini_level;
}

int runlevel_set(runlevel_nr_t level) {
	int ret;

	if (!runlevel_nr_valid(level)) {
		return -EINVAL;
	}

	if (0 != (ret = runlevel_enter(level)) || (runlevel_nr_valid(level + 1)
			&& 0 != (ret = runlevel_leave(level + 1)))) {
		return ret;
	}
	return 0;
}

int runlevel_enter(runlevel_nr_t level) {
	int ret = 0;

	if (!runlevel_nr_valid(level)) {
		return -EINVAL;
	}

	while (init_level < level) {
		if (0 != (ret = mod_enable(runlevels[init_level + 1].init_mod))) {
			break;
		}
	}

	return ret;
}

int runlevel_leave(runlevel_nr_t level) {
	int ret = 0;

	if (!runlevel_nr_valid(level)) {
		return -EINVAL;
	}

	while (fini_level > level) {
		if (0 != (ret = mod_disable(runlevels[fini_level - 1].fini_mod))) {
			break;
		}
	}

	return ret;
}
