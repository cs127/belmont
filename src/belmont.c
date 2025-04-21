#define _GNU_SOURCE

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <shlwapi.h>
#define strcasestr StrStrIA
#endif

#include <libopenmpt/libopenmpt.h>


#define print(s)        fputs(s, stdout)
#define printf(f, ...)  fprintf(stdout, f, __VA_ARGS__)
#define eprint(s)       fputs(s, stderr)
#define eprintf(f, ...) fprintf(stderr, f, __VA_ARGS__)

#define assertx(c, r, m) \
do {if (!(c)) {ret = r; eprint(m); goto end;}} while (false)
#define assertxf(c, r, f, ...) \
do {if (!(c)) {ret = r; eprintf(f, __VA_ARGS__); goto end;}} while (false)

#define destroy(o, f) do {if (o) {f(o); o = NULL;}} while (false)


static long int fsize(FILE* f)
{
	int seek;
	long int cur, size;

	cur = ftell(f);
	if (cur == -1) return -1;

	seek = fseek(f, 0, SEEK_END);
	if (seek == -1) return -1;

	size = ftell(f);

	fseek(f, cur, SEEK_SET);

	return size;
}

int main(int argc, char** argv)
{
	int ret = 0;
	FILE* fp = NULL;
	void* buf = NULL;
	long int size;
	openmpt_module* mod = NULL;
	size_t i, j, n;
	const char* errmsg = NULL, * text = NULL;

	assertxf(argc >= 3, 1, "usage: %s FILEPATH KEYWORD1 ...\n", argv[0]);

	fp = fopen(argv[1], "rb");
	assertxf(fp, 2, "%s: could not open file\n", argv[1]);

	size = fsize(fp);
	assertxf(size >= 1, 3, "%s: file is empty\n", argv[1]);
	assertxf(size <= SIZE_MAX, 3, "%s: file too large\n", argv[1]);

	buf = malloc(size);
	assertxf(buf, 18, "%s: not enough memory to read file\n", argv[1]);

	fread(buf, size, 1, fp);
	destroy(fp, fclose);

	mod = openmpt_module_create_from_memory2
	(
		buf,
		size,
		NULL,     // log function
		NULL,     // log function arg
		NULL,     // error function
		NULL,     // error function arg
		NULL,     // error code destination
		&errmsg,  // error message destination
		NULL      // initial ctls
	);
	assertxf(mod, 4, "%s: could not initialize: %s\n", argv[1], errmsg);

	destroy(buf, free);

	// song message

	text = openmpt_module_get_metadata(mod, "message_raw");
	assertxf(text, 17, "%s: not enough memory\n", argv[1]);

	for (j = 2; j < argc; j++)
	{
		if (strcasestr(text, argv[j]))
			printf("%s: found `%s' in message\n",
			       argv[1], argv[j]);
	}

	// sample names

	n = openmpt_module_get_num_samples(mod);
	for (i = 0; i < n; i++)
	{
		text = openmpt_module_get_sample_name(mod, i);
		assertxf(text, 17, "%s: not enough memory\n", argv[1]);

		for (j = 2; j < argc; j++)
		{
			if (strcasestr(text, argv[j]))
				printf("%s: found `%s' in sample %zu: %s\n",
				       argv[1], argv[j], i + 1, text);
		}

		destroy(text, openmpt_free_string);
	}

	// instrument names

	n = openmpt_module_get_num_instruments(mod);
	for (i = 0; i < n; i++)
	{
		text = openmpt_module_get_instrument_name(mod, i);
		assertxf(text, 17, "%s: not enough memory\n", argv[1]);

		for (j = 2; j < argc; j++)
		{
			if (strcasestr(text, argv[j]))
				printf("%s: found `%s' in instrument %zu: %s\n",
				       argv[1], argv[j], i + 1, text);
		}

		destroy(text, openmpt_free_string);
	}

end:
	destroy(fp, fclose);
	destroy(buf, free);
	destroy(mod, openmpt_module_destroy);
	destroy(errmsg, openmpt_free_string);
	destroy(text, openmpt_free_string);

	return ret;
}
