#include <sys/file.h>

void file_incref(struct file *file)
{
	file->refcount++;
}

void file_decref(struct file *file)
{
	file->refcount--;
	/* XXX do something */
}
