//  multitsccc
//
//  Multi TS Continuity Counter Corrector
//  This program corrects MPEG2 TS continuity counter errors, across multiple files.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2014 - Jonathan G Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <vector>
#include <algorithm>

#define TS_PACKET_SIZE 188

struct pid_info {
	unsigned short pid;
	unsigned char lastcc;
	unsigned int correctioncount = 0;

	pid_info(unsigned int p, unsigned char l) : pid(p), lastcc(l) { }
};

//this must remain sorted at all times
std::vector<pid_info> pids;

void process_tsfile(unsigned char *data, size_t length, const char *filename) {
	unsigned int filecorrectioncount = 0;

	for(size_t offset = 0; offset < length; offset += TS_PACKET_SIZE) {
		if(data[offset] != 0x47) {
			fprintf(stderr, "'%s': TS packet sync byte not present at offset: %lld, got 0x%02X instead, aborting\n",
					filename, (long long int) offset, (unsigned int) data[offset]);
			exit(EXIT_FAILURE);
		}

		unsigned short pid = ((data[offset + 1] & 0x1F) << 8) | data[offset + 2];

		unsigned char cc = data[offset + 3] & 0x0F;
		bool haspayload = data[offset + 3] & 0x10;

		auto it = std::lower_bound(pids.begin(), pids.end(), pid, [](const pid_info &a, int b) {
			return a.pid < b;
		});

		if(it != pids.end() && it->pid == pid) {
			//existing pid
			unsigned char expected_cc = it->lastcc;
			if(haspayload) {
				expected_cc = (expected_cc + 1) & 0xF;
			}

			if(cc != expected_cc) {
				it->correctioncount++;
				filecorrectioncount++;
				data[offset + 3] &= 0xF0;
				data[offset + 3] |= expected_cc;
			}
			it->lastcc = expected_cc;
		}
		else {
			pids.emplace(it, pid, cc);
		}
	}

	fprintf(stderr, "'%s': %u continuity counters corrected\n", filename, filecorrectioncount);
}

int main(int argc, char *argv[]) {
	if(argc <= 1) {
		fprintf(stderr, "Usage: %s TSFILE1 [TSFILE2] ...\n", (argc > 0) ? argv[0] : "multitsccc");
		fprintf(stderr, "\n");
		fprintf(stderr, "This tool corrects MPEG2 TS continuity counter errors, across multiple files.\n");
		fprintf(stderr, "This ensures that concatenating the listed TS files would not lead to\n");
		fprintf(stderr, "continuity counter errors. This is useful for segmented delivery such as HLS.\n");
		fprintf(stderr, "TS files are modified in-place.\n");
		return 1;
	}

	for(int i = 1; i < argc; i++) {
		int fd = open(argv[i], O_RDWR);
		if(fd == -1) {
			fprintf(stderr, "'%s' could not be opened for read/write\n%s\n", argv[i], strerror(errno));
			exit(EXIT_FAILURE);
		}

		//This is to discourage simultaneous editing of the file
		flock(fd, LOCK_EX);

		struct stat fdinfo;
		int fstat_result = fstat(fd, &fdinfo);
		if(fstat_result == -1) {
			fprintf(stderr, "'%s' could not be fstat()ed\n%s\n", argv[i], strerror(errno));
			exit(EXIT_FAILURE);
		}

		if(!S_ISREG(fdinfo.st_mode)) {
			fprintf(stderr, "'%s' does not appear to be a regular file\n", argv[i]);
			exit(EXIT_FAILURE);
		}

		if(fdinfo.st_size == 0) {
			fprintf(stderr, "'%s' has a length of 0\n", argv[i]);
			exit(EXIT_FAILURE);
		}

		if((fdinfo.st_size % TS_PACKET_SIZE) != 0) {
			fprintf(stderr, "'%s' has a length (%lld), that is not a multiple of the TS packet size (%d)\n",
					argv[i], (long long int) fdinfo.st_size, TS_PACKET_SIZE);
			exit(EXIT_FAILURE);
		}

		void *mmap_result = mmap(nullptr, fdinfo.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if(mmap_result == MAP_FAILED) {
			fprintf(stderr, "'%s' could not be mmap()ed\n%s\n", argv[i], strerror(errno));
			exit(EXIT_FAILURE);
		}

		process_tsfile((unsigned char *) mmap_result, fdinfo.st_size, argv[i]);

		munmap(mmap_result, fdinfo.st_size);
		close(fd);
	}

	for(auto &it : pids) {
		fprintf(stderr, "PID: 0x%04X: %u continuity counters corrected\n", (unsigned int) it.pid, it.correctioncount);
	}

	return EXIT_SUCCESS;
}
