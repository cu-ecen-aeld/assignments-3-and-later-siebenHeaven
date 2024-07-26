#include "syslog.h"
#include "stdlib.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {
	openlog(NULL, 0, LOG_USER);
	if ((argc < 3) || (strlen(argv[1]) == 0) || (strlen(argv[2]) == 0)) {
		syslog(LOG_ERR, "Invalid parameters");
		return 1;
	}
	char* path = argv[1];
	char* word = argv[2];

	syslog(LOG_DEBUG, "Writing %s to %s", word, path);

	FILE* fp = fopen(path, "w");

	if (!fp) {
		int err = errno;
		syslog(LOG_ERR, "Opening %s failed: %s", path, strerror(err));
		return err;
	}

	do {
		word += fwrite(word, sizeof(word[0]), strlen(word), fp);
	} while((word != argv[2] + strlen(argv[2])) && errno == EIO);

	if (word != argv[2] + strlen(argv[2])) {
		int err = errno;
		syslog(LOG_ERR, "Writing to %s failed: %s", path, strerror(err));
		return err;
	}

	fclose(fp);

	return 0;
}
