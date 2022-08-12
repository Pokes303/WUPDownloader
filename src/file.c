/***************************************************************************
 * This file is part of NUSspli.                                           *
 * Copyright (c) 2019-2020 Pokes303                                        *
 * Copyright (c) 2020-2022 V10lator <v10lator@myway.de>                    *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *
 ***************************************************************************/

#include <wut-fixups.h>

#include <file.h>
#include <ioQueue.h>
#include <staticMem.h>
#include <tmd.h>
#include <utils.h>

#include <crypto.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <coreinit/filesystem.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memory.h>
#include <coreinit/time.h>

void writeVoidBytes(NUSFILE* fp, uint32_t len)
{
	uint8_t bytes[len];
	OSBlockSet(bytes, 0, len);
	addToIOQueue(bytes, 1, len, fp);
}

void writeCustomBytes(NUSFILE *fp, const char *str)
{
	if(str[0] == '0' && str[1] == 'x')
		str += 2;
	
	size_t size = strlen(str) >> 1;
	uint8_t bytes[size];
	hexToByte(str, bytes);
	addToIOQueue(bytes, 1, size, fp);
}

void writeRandomBytes(NUSFILE* fp, uint32_t len)
{
    uint8_t bytes[len];
    osslBytes(bytes, len);
	addToIOQueue(bytes, 1, len, fp);
}

/*
 * The header is meaned to be human (hex editor) and machine readable. The format is:
 *  - Magic 32 bit value (defined by Nintendo)
 *  - 0x00010203040506070809 As a magic value / metadata detection
 *  - 2 bit spacing
 *  - 64 bit "NUSspli\0" string
 *  - 64 bit padding
 *  - 64 bit version string ending with 0x00
 *  - 64 bit padding
 *  - Max 64 bit file type string ending with 0x00
 
 *  - Padding till 0x000000EE / area reserved for future use
 *  - uint8_t storing the meta version number (currently 1, so 00000001 or 0x01)
 *  - 128 + 32 random bits marking the end of the header usable area
 *  - 256 + 128 + 64 + 32 bit padding (defined by Nintendo / end of header)
 */
void writeHeader(NUSFILE *fp, FileType type)
{
	writeCustomBytes(fp, "0x00010004000102030405060708090000"); // Magic 32 bit value + our magic value + padding
	writeCustomBytes(fp, "0x4E555373706C69"); // "NUSspli"
	writeVoidBytes(fp, 0x9);
	int vl = strlen(NUSSPLI_VERSION);
	addToIOQueue(NUSSPLI_VERSION, 1, vl, fp);
	
	writeVoidBytes(fp, 0x10 - vl);
	char *cb;
	int v;
	switch(type)
	{
		case FILE_TYPE_TIK:
			cb = "0x5469636B6574"; // Ticket
			v = 0xB9;
			break;
		case FILE_TYPE_CERT:
			cb = "0x4365727469666963617465"; // "Certificate"
			v = 0xB4;
			break;
		default:
			cb = "00";
			v = 0xBE;
	}
	
	writeCustomBytes(fp, cb);
	writeVoidBytes(fp, v);
	writeCustomBytes(fp, "0x01"); // TODO: Don't hardcode in here
	writeRandomBytes(fp, 0x14);
	writeVoidBytes(fp, 0x3C);
}

bool fileExists(const char *path)
{
	struct stat fs;
	return stat(path, &fs) == 0;
}

bool dirExists(const char *path)
{
	struct stat ds;
	return stat(path, &ds) == 0 && S_ISDIR(ds.st_mode);
}

void removeDirectory(const char *path)
{
	size_t len = strlen(path);
	char *newPath = getStaticPathBuffer(0);
	strcpy(newPath, path);
	
	if(newPath[len - 1] != '/')
	{
		newPath[len] = '/';
		newPath[++len] = '\0';
	}
	
	char *inSentence = newPath + len;
	OSTime t = OSGetTime();
	DIR *dir = opendir(newPath);
	if(dir != NULL)
	{
		for(struct dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir))
		{
			strcpy(inSentence, entry->d_name);
			if(entry->d_type & DT_DIR)
				removeDirectory(newPath);
			else
			{
				debugPrintf("Removing %s", newPath);
				remove(newPath);
			}
		}
		closedir(dir);
		newPath[len  -  1] = '\0';
		debugPrintf("Removing %s", newPath);
		remove(newPath);
	}
	else
		debugPrintf("Path \"%s\" not found!", newPath);

    t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));
}

NUSFS_ERR moveDirectory(const char *src, const char *dest)
{
	size_t len = strlen(src);
	char *newSrc = getStaticPathBuffer(0);
	OSBlockMove(newSrc, src, ++len, false);

	char *inSrc = newSrc + --len;
	if(*--inSrc != '/')
		*++inSrc = '/';
	++inSrc;

	OSTime t = OSGetTime();
	DIR *dir = opendir(src);
	if(dir == NULL)
		return NUSFS_ERR_DONTEXIST;

	NUSFS_ERR err = createDirectory(dest, 0777);
	if(err != NUSFS_ERR_NOERR)
		return err;

	len = strlen(dest);
	char *newDest = getStaticPathBuffer(1);
	OSBlockMove(newDest, dest, ++len, false);

	char *inDest = newDest + --len;
	if(*--inDest != '/')
		*++inDest = '/';
	++inDest;

	for(struct dirent *entry = readdir(dir); entry != NULL; entry = readdir(dir))
	{
		len = strlen(entry->d_name);
		OSBlockMove(inSrc, entry->d_name, ++len, false);
		OSBlockMove(inDest, entry->d_name, len, false);

		if(entry->d_type & DT_DIR)
		{
			debugPrintf("\tmoveDirectory('%s', '%s')", newSrc, newDest);
			moveDirectory(newSrc, newDest);
		}
		else
		{
			debugPrintf("\trename('%s', '%s')", newSrc, newDest);
			rename(newSrc, newDest);
		}
	}

	closedir(dir);
	remove(src);
    t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));
	return NUSFS_ERR_NOERR;
}

// There are no files > 4 GB on the Wii U, so size_t should be more than enough.
size_t getFilesize(FILE *fp)
{
	OSTime t = OSGetTime();

	struct stat info;
	if(fstat(fileno(fp), &info) == -1)
		return -1;

	t = OSGetTime() - t;
	addEntropy(&t, sizeof(OSTime));

	return (size_t)(info.st_size);
}

size_t readFile(const char *path, void **buffer)
{
	FILE *file = fopen(path, "rb");
	if(file != NULL)
	{
		size_t filesize = getFilesize(file);
		if(filesize != -1)
		{
			*buffer = MEMAllocFromDefaultHeapEx(FS_ALIGN(filesize), 0x40);
			if(*buffer != NULL)
			{
				if(fread(*buffer, filesize, 1, file) == 1)
				{
					fclose(file);
					return filesize;
				}

				debugPrintf("Error reading %s!", path);
				MEMFreeToDefaultHeap(*buffer);
			}
			else
				debugPrintf("Error creating buffer!");
		}
		else
			debugPrintf("Error getting filesize for %s!", path);

		fclose(file);
	}
	else
		debugPrintf("Error opening %s!", path);

	*buffer = NULL;
	return 0;
}

// This uses informations from https://github.com/Maschell/nuspacker
bool verifyTmd(const TMD *tmd, size_t size)
{
	if(size >= sizeof(TMD))
	{
		if(tmd->num_contents > 0)
		{
			if(size == (sizeof(TMD) - sizeof(TMD_CONTENT)) + (sizeof(TMD_CONTENT) * tmd->num_contents) ||
					size == (sizeof(TMD) - sizeof(TMD_CONTENT)) + (sizeof(TMD_CONTENT) * tmd->num_contents) + 0x700)
			{
				uint32_t hash[8];
				uint8_t *ptr = ((uint8_t *)tmd) + (sizeof(TMD) - sizeof(TMD_CONTENT));
				if(getSHA256(ptr, sizeof(TMD_CONTENT) * tmd->num_contents, (uint8_t *)hash))
				{
					for(int i = 0; i < 8; ++i)
					{
						if(hash[i] != tmd->content_infos[0].hash[i])
						{
							debugPrintf("Invalid title.tmd file (content hash mismatch)");
							return false;
						}
					}

					ptr -= sizeof(TMD_CONTENT_INFO) * 64;
					if(getSHA256(ptr, sizeof(TMD_CONTENT_INFO) * 64, (uint8_t *)hash))
					{
						for(int i = 0; i < 8; ++i)
						{
							if(hash[i] != tmd->hash[i])
							{
								debugPrintf("Invalid title.tmd file (tmd hash mismatch)");
								return false;
							}
						}

						for(int i = 0; i < tmd->num_contents; ++i)
						{
							if(tmd->contents[i].index != i)
							{
								debugPrintf("Invalid title.tmd file (content: %d, index: %u)", i, tmd->contents[i].index);
								return false;
							}
							if(tmd->contents[i].size > (uint64_t)1024 * 1024 * 1024 * 4)
							{
								debugPrintf("Invalid title.tmd file (content: %d, size: %llu)", i, tmd->contents[i].size);
								return false;
							}
						}
						return true;
					}
					else
						debugPrintf("Error calculating tmd hash for title.tmd file!");
				}
				else
					debugPrintf("Error calculating content hash for title.tmd file!");
			}
			else
				debugPrintf("Wrong title.tmd filesize (num_contents: %u, filesize: 0x%X)", tmd->num_contents, size);
		}
		else
			debugPrintf("Invalid title.tmd file (num_contents: %u)", tmd->num_contents);
	}
	else
		debugPrintf("Wrong title.tmd filesize: 0x%X", size);

	return false;
}

TMD *getTmd(const char *dir)
{
	size_t s = strlen(dir);
	char *path = MEMAllocFromDefaultHeap(s + (strlen("title.tmd") + 1));
	if(path != NULL)
	{
		OSBlockMove(path, dir, s, false);
		strcpy(path + s, "title.tmd");

		TMD *tmd;
		s = readFile(path, (void **)&tmd);
		MEMFreeToDefaultHeap(path);

		if(tmd != NULL)
		{
			if(verifyTmd(tmd, s))
				return tmd;

			MEMFreeToDefaultHeap(tmd);
		}
	}

	return NULL;
}

NUSFS_ERR createDirectory(const char *path, mode_t mode)
{
	OSTime t = OSGetTime();
    if(mkdir(path, mode) == 0)
	{
		t = OSGetTime() - t;
		addEntropy(&t, sizeof(OSTime));
		return NUSFS_ERR_NOERR;
	}

	int ie = errno;
	switch(ie)
	{
		case EROFS:
		case -19:
			return NUSFS_ERR_LOCKED;
		case ENOSPC:
			return NUSFS_ERR_FULL;
        case FS_ERROR_MAX_FILES:
		case FS_ERROR_MAX_DIRS:
			return NUSFS_ERR_LIMITS;
		default:
			// STUB
			break;
	}

	if(ie >= 0)
		ie += 1000;

	return (NUSFS_ERR)ie;
}

bool createDirRecursive(const char *dir)
{
	size_t len = strlen(dir);
	char d[++len];
	OSBlockMove(d, dir, len, false);

	char *needle = d;
	if(strncmp(NUSDIR_SD, d, strlen(NUSDIR_SD)) == 0)
		needle += strlen(NUSDIR_SD);
	else
		needle += strlen(NUSDIR_MLC);

	do
	{
		needle = strchr(needle, '/');
		if(needle == NULL)
			return dirExists(d) ? true : createDirectory(d, 777) == NUSFS_ERR_NOERR;

		*needle = '\0';
		if(!dirExists(d) && createDirectory(d, 777) != NUSFS_ERR_NOERR)
			return false;

		*needle = '/';
		++needle;
	}
	while(*needle != '\0');

	return true;
}

const char *translateNusfsErr(NUSFS_ERR err)
{
	switch(err)
	{
		case NUSFS_ERR_LOCKED:
			return "SD card write locked!";
		case NUSFS_ERR_FULL:
			return "No space left on device!";
		case NUSFS_ERR_LIMITS:
			return "Filesystem limits reached!";
		case NUSFS_ERR_DONTEXIST:
			return "Not found!";
        default:
            return err > 1000 ? errnoToString(err - 1000) : NULL;
	}
}
