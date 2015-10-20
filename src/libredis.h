
typedef struct
{
	char* buf;
	int len;
} reply_t;
reply_t libredis_call(const char *cmdbuf, int len);
void *libredis_new_instance(int argc, char **argv);
void libredis_set_instance(void* pinst);
void libredis_drop_reply(reply_t *reply);