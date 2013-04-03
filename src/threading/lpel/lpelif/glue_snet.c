#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "glue_snet.h"
#include "debug.h"

/* S-Net threading backend interface */
#include "threading.h"

#include "lpel.h"

/* provisional assignment module */
#include "assign.h"

/* monitoring module */
#include "mon_snet.h"

/* put this into assignment/monitoring module */
#include "distribution.h"

#include <lpel/monitor.h>

static int num_cpus = 0;
static int num_workers = 0;
static int cpu_wrapper = 0;
static int cpu_sosi = 0;
static int cpu_workers = 0;

static FILE *mapfile = NULL;
static int mon_flags = 0;

/*
 *  scheduling cycle = number of records to written in one task execution
 *  Default = 20 (equals to buffer size in old lpel)
 */
static int rec_lim = 20;


/**
 * support source/sink
 */
static bool using_sosi = false;

static bool sep_wrapper = false;


static size_t GetStacksize(snet_entity_descr_t descr)
{
	size_t stack_size;

	switch(descr) {
	case ENTITY_parallel:
	case ENTITY_star:
	case ENTITY_split:
	case ENTITY_fbcoll:
	case ENTITY_fbdisp:
	case ENTITY_fbbuf:
	case ENTITY_sync:
	case ENTITY_filter:
	case ENTITY_nameshift:
	case ENTITY_collect:
		stack_size = 256*1024; /* 256KB, HIGHLY EXPERIMENTAL! */
		break;
	case ENTITY_box:
	case ENTITY_other:
		stack_size = 8*1024*1024; /*8MB*/
		break;
	default:
		/* we do not want an unhandled case here */
		assert(0);
	}

	return( stack_size);
}


static void *EntityTask(void *arg)
{
	snet_entity_t *ent = (snet_entity_t *)arg;

	SNetEntityCall(ent);
	SNetEntityDestroy(ent);

	return NULL;
}

int SNetThreadingInit(int argc, char **argv)
{
	lpel_config_t config;
	int i, res;
	char *mon_elts = NULL;
	memset(&config, 0, sizeof(lpel_config_t));

	char fname[20+1];
	int priorf = 4;

	config.flags = LPEL_FLAG_NONE | LPEL_FLAG_PINNED;		// pinned by default

	for (i=0; i<argc; i++) {
		if(strcmp(argv[i], "-m") == 0 && i + 1 <= argc) {
			/* Monitoring level */
			i = i + 1;
			mon_elts = argv[i];
		} else if (strcmp(argv[i], "-np") == 0) {	// no pinned
			config.flags ^= LPEL_FLAG_PINNED;
		} else if(strcmp(argv[i], "-excl") == 0 ) {
			/* Assign realtime priority to workers*/
			config.flags |= LPEL_FLAG_EXCLUSIVE;
		} else if(strcmp(argv[i], "-cwp") == 0 && i + 1 <= argc) {
			/* Number of cores for others */
			i = i + 1;
			cpu_wrapper = atoi(argv[i]);
			sep_wrapper = true;
		} else if(strcmp(argv[i], "-w") == 0 && i + 1 <= argc) {
			/* Number of workers */
			i = i + 1;
			num_workers = atoi(argv[i]);
		} else if (strcmp(argv[i], "-cw") == 0 && i + 1 <= argc) {
			/* number of cores for workers */
			i = i + 1;
			cpu_workers = atoi(argv[i]);
		}else if(strcmp(argv[i], "-css") == 0 && i + 1 <= argc) {
			/* number of cores for source/sink */
			config.flags |= LPEL_FLAG_SOSI;
			using_sosi = true;
			i = i + 1;
			cpu_sosi = atoi(argv[i]);
		}else if(strcmp(argv[i], "-pf") == 0 && i + 1 <= argc) {
			i = i + 1;
			priorf = atoi(argv[i]);
		}else if(strcmp(argv[i], "-rl") == 0 && i + 1 <= argc) {
			i = i + 1;
			rec_lim = atoi(argv[i]);
		}
	}

	LpelTaskSetPriorFunc(priorf);

#ifdef USE_LOGGING
	if (mon_elts != NULL) {
		if (strchr(mon_elts, MON_ALL_FLAG) != NULL) {
			mon_flags = (1<<7) - 1;
		} else {
			if (strchr(mon_elts, MON_MAP_FLAG) != NULL) mon_flags |= SNET_MON_MAP;
			if (strchr(mon_elts, MON_TIME_FLAG) != NULL) mon_flags |= SNET_MON_TIME;
			if (strchr(mon_elts, MON_WORKER_FLAG) != NULL) mon_flags |= SNET_MON_WORKER;
			if (strchr(mon_elts, MON_TASK_FLAG) != NULL) mon_flags |= SNET_MON_TASK;
			if (strchr(mon_elts, MON_STREAM_FLAG) != NULL) mon_flags |= SNET_MON_STREAM;
			if (strchr(mon_elts, MON_MESSAGE_FLAG) != NULL) mon_flags |= SNET_MON_MESSAGE;
			if (strchr(mon_elts, MON_LOAD_FLAG) != NULL) mon_flags |= SNET_MON_LOAD;
		}



		if ( mon_flags & SNET_MON_MAP) {
			snprintf(fname, 20, "n%02d_tasks.map", SNetDistribGetNodeId() );
			/* create a map file */
			mapfile = fopen(fname, "w");
			assert( mapfile != NULL);
			(void) fprintf(mapfile, "%s%c", LOG_FORMAT_VERSION, END_LOG_ENTRY);
		}
	}

#endif

  /* determine number of cpus */
	if ( 0 != LpelGetNumCores( &num_cpus) ) {
		SNetUtilDebugFatal("Could not determine number of cores!\n");
		assert(0);
	}

	if (num_workers == 0) {
		config.num_workers = num_cpus - cpu_wrapper - cpu_sosi; // including master
	} else {
		config.num_workers = num_workers;
	}
	if (cpu_workers == 0)
		config.proc_workers = num_cpus - cpu_wrapper - cpu_sosi;
	else
		config.proc_workers = cpu_workers;

	config.proc_wrapper = cpu_wrapper;
	config.proc_sosi = cpu_sosi;

#ifdef USE_LOGGING
	/* initialise monitoring module */
	SNetThreadingMonInit(&config.mon, SNetDistribGetNodeId(), mon_flags);
#endif

	res = LpelInit(&config);
	if (res != LPEL_ERR_SUCCESS) {
		SNetUtilDebugFatal("Could not initialize LPEL!\n");
	}
	LpelStart();

	return 0;
}




unsigned long SNetThreadingGetId()
{
	/* FIXME more convenient way */
	/* returns the thread id */
	return (unsigned long) LpelTaskSelf();
}



int SNetThreadingStop(void)
{
	/* send a stop signal to LPEL */
	LpelStop();
	/* following call will block until the workers have finished */
	LpelCleanup();
	return 0;
}


int SNetThreadingCleanup(void)
{
	SNetAssignCleanup();

#ifdef USE_LOGGING
	/* Cleanup monitoring module */
	SNetThreadingMonCleanup();
	if (mapfile) {
		(void) fclose(mapfile);
	}
#endif

	return 0;
}


/**
 * Signal an user event
 */
void SNetThreadingEventSignal(snet_entity_t *ent, snet_moninfo_t *moninfo)
{
	(void) ent; /* NOT USED */
	lpel_task_t *t = LpelTaskSelf();
	assert(t != NULL);
	mon_task_t *mt = LpelTaskGetMon(t);
	if (mt != NULL) {
		SNetThreadingMonEvent(mt, moninfo);
	}
}



/*****************************************************************************
 * Spawn a new task
 ****************************************************************************/
/*
 * TODO: extend to have dynamic record limit/scheduling cycle
 */
static void setTaskRecLimit(snet_entity_descr_t type, lpel_task_t *t){
	LpelTaskSetRecLimit(t, rec_lim);
}


int SNetThreadingSpawn(snet_entity_t *ent)
/*
  snet_entity_type_t type,
  snet_locvec_t *locvec,
  int location,
  const char *name,
  snet_entityfunc_t func,
  void *arg
  )
 */
{
	int map = LPEL_MAP_MASTER;
	snet_entity_descr_t type = SNetEntityDescr(ent);
	//int location = SNetEntityNode(ent);
	const char *name = SNetEntityName(ent);

	if (using_sosi) {
		if (strstr(name, SOURCE_BOX_ENTITY) == name)	// if SOURCE_BOX_ENTITY is prefix of name
			map = LPEL_MAP_SOURCE;
		if (strstr(name, SINK_BOX_ENTITY) == name)	// if SINK_BOX_ENTITY is prefix of name
			map = LPEL_MAP_SINK;
	}

	/* wrapper task is mapped to separate thread only in exclusive mode */
	if (sep_wrapper && type == ENTITY_other)
		map = LPEL_MAP_WRAPPER;

	lpel_task_t *t = LpelTaskCreate(
			map,
			//(lpel_taskfunc_t) func,
			EntityTask,
			ent,
			GetStacksize(type)
	);

	/** set limit number of records a task can process in once
	 * apply for only tasks controlled by the master
	 */
	if (map == LPEL_MAP_MASTER)
		setTaskRecLimit(type, t);

#ifdef USE_LOGGING
	if (mon_flags & SNET_MON_TASK){
		mon_task_t *mt = SNetThreadingMonTaskCreate(
				LpelTaskGetId(t),
				name
		);
		LpelTaskMonitor(t, mt);
		/* if we monitor the task, we make an entry in the map file */
	}

	if ((mon_flags & SNET_MON_MAP) && mapfile) {
		int tid = LpelTaskGetId(t);
		(void) fprintf(mapfile, "%d%s%c", tid, SNetEntityStr(ent), END_LOG_ENTRY);
	}


#endif


	LpelTaskStart(t);
	return 0;
}



