#ifdef REDIS_RDSN_REPLICATION
#include "redis.h"
#include "libredis.h"
#include "malloc.h"
void initServerConfig();
void initServer();

void *libredis_new_instance(const char* load_filename, int argc, char **argv)
{
    if (load_filename == NULL)
    {
        printf("new instance!!!\n");
    } else
    {
        printf("load instance %s\n", load_filename);
    }
    
	//init malloc
	g_malloc = malloc;
	g_calloc = calloc;
	g_realloc = realloc;
	g_free = free;
	g_msize = _msize;
	//init server
	instance_state_t *instance = tls_instance_state = (instance_state_t*)malloc(sizeof(instance_state_t));
    if (instance == NULL)
    {
        printf("fatal! cannot allocate repis serer\n");
        return NULL;
    }
	memset(tls_instance_state, 0, sizeof(instance_state_t));
	struct timeval tv;
	/* We need to initialize our libraries, and the server configuration. */
	zmalloc_enable_thread_safeness();
	srand((unsigned int)time(NULL) ^ getpid());
	gettimeofday(&tv, NULL);
	dictSetHashFunctionSeed(tv.tv_sec^tv.tv_usec^getpid());;
    initServerConfig();
    server.sentinel_mode = 0;

	if (argc >= 2) {
		int j = 1; /* First option to parse in argv[] */
		sds options = sdsempty();
		char *configfile = NULL;
		/* First argument is the config file name? */
		if (argv[j][0] != '-' || argv[j][1] != '-')
			configfile = argv[j++];
		/* All the other options are parsed and conceptually appended to the
		* configuration file. For instance --port 6380 will generate the
		* string "port 6380\n" to be parsed after the actual file name
		* is parsed, if any. */
		while (j != argc) {
			if (argv[j][0] == '-' && argv[j][1] == '-') {
				/* Option name */
				if (sdslen(options)) options = sdscat(options, "\n");
				options = sdscat(options, argv[j] + 2);
				options = sdscat(options, " ");
			}
			else {
				/* Option argument */
				options = sdscatrepr(options, argv[j], strlen(argv[j]));
				options = sdscat(options, " ");
			}
			j++;
		}
		if (configfile) server.configfile = getAbsolutePath(configfile);

		resetServerSaveParams();
		loadServerConfig(configfile, options);
		sdsfree(options);
	}
	else {
		redisLog(REDIS_WARNING, "Warning: no config file specified, using the default config. In order to specify a config file use %s /path/to/%s.conf", argv[0], server.sentinel_mode ? "sentinel" : "redis");
	}

	initServer();

	if (load_filename != NULL)
	{
		if (rdbLoad((char*)load_filename) != REDIS_OK) {
			return NULL;
		}
	}
	return (void*)instance;
}

void libredis_set_instance(void*pinst)
{
	tls_instance_state = (instance_state_t*)pinst;
}

reply_t libredis_call(const char *cmdbuf, int len)
{
	redisClient *c = server.lua_client;
	sdsclear(c->querybuf);
	c->querybuf = sdscatlen(c->querybuf, (void*)cmdbuf, len);
	processInputBuffer(c);
	sds reply;
	reply = sdsnewlen(c->buf, c->bufpos);
	c->bufpos = 0;
	while (listLength(c->reply)) {
		robj *o = listNodeValue(listFirst(c->reply));

		reply = sdscatlen(reply, o->ptr, sdslen(o->ptr));
		listDelNode(c->reply, listFirst(c->reply));
	}
	reply_t result;
	c->reply_bytes = 0;
	result.buf = reply;
	result.len = sdslen(reply);
	return result;
}

void libredis_drop_reply(reply_t *reply)
{
	sdsfree(reply->buf);
}

int rdbSaveRio(rio *rdb, int *error);

int libredis_save(const char* filename)
{
	FILE *fp;
	rio rdb;
	if ((fp = fopen(filename, "wb")) == NULL)
	{
        return  -1;
	}
	rioInitWithFile(&rdb, fp);
	int error;
	if (rdbSaveRio(&rdb, &error) == REDIS_ERR)
    {

        fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}

#endif
