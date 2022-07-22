#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>

void *memset(void *d, int c, size_t len)
{
	for (size_t i = 0; i < len; ++i)
		((uint8_t*)d)[i] = c;
	return d;
}

void *memcpy(void *d, const void *s, size_t n)
{
	for (size_t i = 0; i < n; ++i)
		((uint8_t*)d)[i] = ((uint8_t*)s)[i];
	return d;
}

void *memccpy(void *d, const void *s, int c, size_t n)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (((uint8_t*)s)[i] == c)
		{
			((uint8_t*)d)[i] = c;
			return d + i + 1;
		}
		((uint8_t*)d)[i] = ((uint8_t*)s)[i];
		i++;
	}
	return NULL;
}

void *memmove(void *d, const void *s, size_t n)
{
	if (!n)
		return d;
	if (d < s)
	{
		for (size_t i = 0; i < n; ++i)
			((uint8_t*)d)[i] = ((uint8_t*)s)[i];
	}
	else
	{
		for (size_t i = n - 1; i > 0; ++i)
			((uint8_t*)d)[i] = ((uint8_t*)s)[i];
		*((uint8_t*)d) = *((uint8_t*)s);
	}
	return d;
}

void *memchr(const void *s, int c, size_t n)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (((uint8_t*)s)[i] == (uint8_t)c)
			return (uint8_t*)s + i;
	}
	return NULL;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	for (size_t i = 0; i < n; ++i)
	{
		if (((uint8_t*)s1)[i] != ((uint8_t*)s2)[i])
			return (((uint8_t*)s1)[i] - ((uint8_t*)s2)[i]);
	}
	return 0;
}

size_t strlen(const char *s)
{
	size_t i = 0;
	while (s[i])
		i++;
	return i;
}

char *strcpy(char *d, const char *s)
{
	size_t i;
	for (i = 0; s[i]; ++i)
		d[i] = s[i];
	d[i] = '\0';
	return d;
}

char *strncpy(char *d, const char *s, size_t n)
{
	size_t i;
	for (i = 0; s[i] && i < n; ++i)
		d[i] = s[i];
	for (; i < n; ++i)
		d[i] = '\0';
	return d;
}

size_t strlcpy(char *d, const char *s, size_t n)
{
	size_t i;
	for (i = 0; s[i] && i < n - 1; ++i)
		d[i] = s[i];
	d[i] = '\0';
	while (s[i])
		++i;
	return i;
}

char *strcat(char *d, const char *s)
{
	size_t i = 0;
	while (d[i])
		++i;
	size_t j = 0;
	while (s[j])
	{
		d[i] = s[j];
		++i;
		++j;
	}
	d[i] = '\0';
	return d;
}

char *strncat(char *d, const char *s, size_t n)
{
	size_t i = 0;
	while (d[i])
		++i;
	size_t j = 0;
	while (s[j] && j < n)
	{
		d[i] = s[j];
		++i;
		++j;
	}
	d[i] = '\0';
	return d;
}

size_t strlcat(char *d, const char *s, size_t n)
{
	size_t i = 0;
	while (d[i])
		++i;
	size_t j = 0;
	while (s[j])
		++j;
	size_t ret = j + (n < i ? n : i);
	j = 0;
	while (s[j] && i < n - 1)
	{
		d[i] = s[j];
		i++;
		j++;
	}
	d[i] = '\0';
	return ret;
}

char *strchr(const char *s, int c)
{
	while (*s)
	{
		if (*s == (char)c)
			return (char*)s;
		s++;
	}
	if (!(char)c)
		return (char*)s;
	return NULL;
}

char *strchrnul(const char *s, int c)
{
	while (*s)
	{
		if (*s == (char)c)
			return (char*)s;
		s++;
	}
	return (char*)s;
}

char *strrchr(const char *s, int c)
{
	char *ret = NULL;
	while (*s)
	{
		if (*s == (char)c)
			ret = (char*)s;
		s++;
	}
	if (!(char)c)
		return (char*)s;
	return ret;
}

char *strstr(const char *s1, const char *s2)
{
	if (!*s2)
		return (char*)s1;
	for (size_t i = 0; s1[i]; ++i)
	{
		size_t j;
		for (j = 0; s1[i + j] == s2[j]; ++i)
		{
			if (!s2[j])
				return (char*)s1 + i;
		}
		if (!s2[j])
			return (char*)s1 + i;
	}
	return NULL;
}

char *strnstr(const char *s1, const char *s2, size_t n)
{
	if (!*s2)
		return (char*)s1;
	for (size_t i = 0; s1[i] && i < n; ++i)
	{
		for (size_t j = 0; (s1[i + j] == s2[j] || !s2[j]); ++j)
		{
			if (!s2[j])
				return (char*)s1 + i;
			if (i + j >= n)
				return NULL;
		}
	}
	return NULL;
}

int strcmp(const char *s1, const char *s2)
{
	size_t i = 0;
	while (s1[i] && s2[i] && s1[i] == s2[i])
		i++;
	return (((uint8_t*)s1)[i] - ((uint8_t*)s2)[i]);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	size_t i;
	for (i = 0; i < n && s1[i] && s2[i]; ++i)
	{
		uint8_t d = ((uint8_t*)s1)[i] - ((uint8_t*)s2)[i];
		if (d)
			return d;
	}
	if (i == n)
		return 0;
	return ((uint8_t*)s1)[i] - ((uint8_t*)s2)[i];
}

char *strdup(const char *s)
{
	size_t len = strlen(s);
	char *ret = malloc(len + 1, 0);
	if (!ret)
		return NULL;
	memcpy(ret, s, len + 1);
	return ret;
}

char *strndup(const char *s, size_t n)
{
	size_t len = strlen(s);
	if (n < len)
		len = n;
	char *ret = malloc(len + 1, 0);
	if (!ret)
		return NULL;
	memcpy(ret, s, len);
	ret[len] = '\0';
	return ret;
}

static const char *g_err_str[] =
{
	[E2BIG          ] = "Argument list too long",
	[EACCES         ] = "Permission denied",
	[EADDRINUSE     ] = "Address in use",
	[EADDRNOTAVAIL  ] = "Address not available",
	[EAFNOSUPPORT   ] = "Address family not supported",
	[EAGAIN         ] = "Resource unavailable, try again",
	[EALREADY       ] = "Connection already in progress",
	[EBADF          ] = "Bad file descriptor",
	[EBADMSG        ] = "Bad message",
	[EBUSY          ] = "Device or resource busy",
	[ECANCELED      ] = "Operation canceled",
	[ECHILD         ] = "No child processes",
	[ECONNABORTED   ] = "Connection aborted",
	[ECONNREFUSED   ] = "Connection refused",
	[ECONNRESET     ] = "Connection reset",
	[EDEADLK        ] = "Resource deadlock would occur",
	[EDESTADDRREQ   ] = "Destination address required",
	[EDOM           ] = "Mathematics argument out of domain of function",
	[EDQUOT         ] = "Reserved",
	[EEXIST         ] = "File exists",
	[EFAULT         ] = "Bad address",
	[EFBIG          ] = "File too large",
	[EHOSTUNREACH   ] = "Host is unreachable",
	[EIDRM          ] = "Identifier removed",
	[EILSEQ         ] = "Illegal byte sequence",
	[EINPROGRESS    ] = "Operation in progress",
	[EINTR          ] = "Interrupted function",
	[EINVAL         ] = "Invalid argument",
	[EIO            ] = "I/O error",
	[EISCONN        ] = "Socket is connected",
	[EISDIR         ] = "Is a directory",
	[ELOOP          ] = "Too many levels of symbolic links",
	[EMFILE         ] = "File descriptor value too large",
	[EMLINK         ] = "Too many links",
	[EMSGSIZE       ] = "Message too large",
	[EMULTIHOP      ] = "Reserved",
	[ENAMETOOLONG   ] = "Filename too long",
	[ENETDOWN       ] = "Network is down",
	[ENETRESET      ] = "Connection aborted by network",
	[ENETUNREACH    ] = "Network unreachable",
	[ENFILE         ] = "Too many files open in system",
	[ENOBUFS        ] = "No buffer space available",
	[ENODATA        ] = "No message is available on the STREAM head read queue",
	[ENODEV         ] = "No such device",
	[ENOENT         ] = "No such file or directory",
	[ENOEXEC        ] = "Executable file format error",
	[ENOLCK         ] = "No locks available",
	[ENOLINK        ] = "Reserved",
	[ENOMEM         ] = "Not enough space",
	[ENOMSG         ] = "No message of the desired type",
	[ENOPROTOOPT    ] = "Protocol not available",
	[ENOSPC         ] = "No space left on device",
	[ENOSR          ] = "No STREAM resources",
	[ENOSTR         ] = "Not a STREAM",
	[ENOSYS         ] = "Functionality not supported",
	[ENOTCONN       ] = "The socket is not connected",
	[ENOTDIR        ] = "Not a directory or a symbolic link to a directory",
	[ENOTEMPTY      ] = "Directory not empty",
	[ENOTRECOVERABLE] = "State not recoverable",
	[ENOTSOCK       ] = "Not a socket",
	[ENOTTY         ] = "Inappropriate I/O control operation",
	[ENXIO          ] = "No such device or address",
	[EOPNOTSUPP     ] = "Operation not supported on socket",
	[EOVERFLOW      ] = "Value too large to be stored in data type",
	[EOWNERDEAD     ] = "Previous owner died",
	[EPERM          ] = "Operation not permitted",
	[EPIPE          ] = "Broken pipe",
	[EPROTO         ] = "Protocol error",
	[EPROTONOSUPPORT] = "Protocol not supported",
	[EPROTOTYPE     ] = "Protocol wrong type for socket",
	[ERANGE         ] = "Result too large",
	[EROFS          ] = "Read-only file system",
	[ESPIPE         ] = "Invalid seek",
	[ESRCH          ] = "No such process",
	[ESTALE         ] = "Reserved",
	[ETIME          ] = "Stream ioctl() timeout",
	[ETIMEDOUT      ] = "Connection timed out",
	[ETXTBSY        ] = "Text file busy",
	[EXDEV          ] = "Cross-device link",
};

char *strerror(int errnum)
{
	if (errnum <= 0 || (unsigned)errnum >= sizeof(g_err_str) / sizeof(*g_err_str))
		return "Unknown error";
	char *ret = (char*)g_err_str[errnum];
	if (ret)
		return ret;
	return "Unknown error";
}
