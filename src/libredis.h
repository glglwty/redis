#pragma once
typedef struct
{
	char* buf;
	int len;
} reply_t;
reply_t libredis_call(const char *cmdbuf, int len);
//create a new instance, load data from file if load_filename is not null.
//return 0 on failure, otherwise return the tls environment
void *libredis_new_instance(const char* load_filename, int argc, char **argv);
//set the tls environment.
void libredis_set_instance(void* pinst);
//cleanup the reply returned by libredis_call
void libredis_drop_reply(reply_t *reply);
//save the database to the specified file. return non zero on error.
int libredis_save(const char* filename);
