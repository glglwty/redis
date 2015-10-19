char *libredis_read(char *cmdbuf);
char *libredis_write(char* cmdbuf);
void *libredis_new_instance(int argc, char **argv);
void libredis_set_instance(void* pinst);