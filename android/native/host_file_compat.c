#include "host_file_compat.h"
#include "iso_disc.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdatomic.h>

struct RecompHostFile { int fd; DIR *directory; char path[PATH_MAX]; int iso_file,iso_find; unsigned iso_cursor; uint64_t iso_position; };
struct IsoFile { char name[256]; uint64_t offset,size; };
static struct { int fd; unsigned count; char root[PATH_MAX]; struct IsoFile files[256]; } iso={.fd=-1};
static _Atomic uint64_t iso_read_bytes;
void recomp_iso_unmount(void){if(iso.fd>=0)close(iso.fd);memset(&iso,0,sizeof iso);iso.fd=-1;atomic_store(&iso_read_bytes,0);}
uint64_t recomp_iso_read_bytes(void){return atomic_load(&iso_read_bytes);}
int recomp_iso_mount(int fd,const char *root,unsigned count,const char *const *names,const uint64_t *offsets,const uint64_t *sizes){
    struct stat st;
    if(fd<0||!root||root[0]!='/'||strlen(root)>=sizeof iso.root||!count||count>256||!names||!offsets||!sizes||fstat(fd,&st)||st.st_size<0||lseek(fd,0,SEEK_CUR)<0){errno=EINVAL;return 0;}
    for(unsigned i=0;i<count;++i){
        const char *name=names[i];
        if(!name||!*name||strlen(name)>=sizeof iso.files[i].name||!strcmp(name,".")||!strcmp(name,"..")||offsets[i]>(uint64_t)st.st_size||sizes[i]>(uint64_t)st.st_size-offsets[i]){errno=EINVAL;return 0;}
        for(const unsigned char *c=(const unsigned char *)name;*c;++c)if(*c<32||*c>126||*c=='/'||*c=='\\'||*c==':'){errno=EINVAL;return 0;}
        for(unsigned j=0;j<i;++j)if(!strcasecmp(name,names[j])||(sizes[i]&&sizes[j]&&offsets[i]<offsets[j]+sizes[j]&&offsets[j]<offsets[i]+sizes[i])){errno=EINVAL;return 0;}
    }
    int copy=fcntl(fd,F_DUPFD_CLOEXEC,0);if(copy<0)return 0;
    recomp_iso_unmount();iso.fd=copy;iso.count=count;strcpy(iso.root,root);
    size_t n=strlen(iso.root);while(n>1&&iso.root[n-1]=='/')iso.root[--n]=0;
    for(unsigned i=0;i<count;++i){strcpy(iso.files[i].name,names[i]);iso.files[i].offset=offsets[i];iso.files[i].size=sizes[i];}
    fprintf(stderr,"ISO mounted directly: files=%u image_bytes=%llu root=%s\n",count,(unsigned long long)st.st_size,iso.root);return 1;
}
/* -1 means outside the mounted disc, -2 its root, -3 a missing disc path. */
static int iso_lookup(const char *path){
    if(iso.fd<0||!path)return -1;
    char normal[PATH_MAX];size_t length=strlen(path);if(length>=sizeof normal)return -1;
    strcpy(normal,path);for(char *c=normal;*c;++c)if(*c=='\\')*c='/';
    size_t root=strlen(iso.root);if(strncasecmp(normal,iso.root,root)|| (normal[root]&&normal[root]!='/'))return -1;
    const char *name=normal+root;while(*name=='/')++name;if(!*name)return -2;
    for(unsigned i=0;i<iso.count;++i)if(!strcasecmp(name,iso.files[i].name))return (int)i;
    return -3;
}
static _Thread_local DWORD last_error;
static BOOL fail(void)
{
    switch (errno) {
    case ENOENT: last_error = ERROR_FILE_NOT_FOUND; break;
    case ENOTDIR: last_error = ERROR_PATH_NOT_FOUND; break;
    case EEXIST: last_error = ERROR_ALREADY_EXISTS; break;
    case EACCES: case EPERM: case EROFS: last_error = ERROR_ACCESS_DENIED; break;
    case EBUSY: last_error = ERROR_SHARING_VIOLATION; break;
    default: last_error = 1; break;
    }
    return 0;
}
DWORD GetLastError(void) { return last_error; }

/* Guest paths use case-insensitive Windows/Xbox separators. Resolve each
   existing component against the Android filesystem; new leaf names retain
   their requested spelling. No process working-directory change is needed. */
static BOOL native_path(const char *source, char result[PATH_MAX])
{
    if (!source || strlen(source) >= PATH_MAX) { errno = ENAMETOOLONG; return fail(); }
    char input[PATH_MAX];
    strcpy(input, source);
    for (char *p = input; *p; ++p) if (*p == '\\') *p = '/';
    result[0] = 0;
    if (input[0] == '/') strcpy(result, "/");
    char *save = NULL;
    for (char *part = strtok_r(input, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
        char name[256];
        if (strlen(part) >= sizeof name) { errno = ENAMETOOLONG; return fail(); }
        strcpy(name, part);
        DIR *dir = opendir(*result ? result : ".");
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir))) if (!strcasecmp(entry->d_name, part)) {
                strcpy(name, entry->d_name); break;
            }
            closedir(dir);
        }
        size_t length = strlen(result), count = strlen(name);
        if (length + count + 2 >= PATH_MAX) { errno = ENAMETOOLONG; return fail(); }
        if (length && result[length - 1] != '/') strcat(result, "/");
        strcat(result, name);
    }
    return 1;
}
static DWORD attributes(const struct stat *st)
{
    DWORD a = S_ISDIR(st->st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    if (!(st->st_mode & S_IWUSR)) a |= FILE_ATTRIBUTE_READONLY;
    return a;
}
static uint64_t timestamp(struct timespec value)
{
    return ((uint64_t)value.tv_sec + UINT64_C(11644473600)) * UINT64_C(10000000) + value.tv_nsec / 100;
}
static FILETIME filetime(struct timespec value)
{
    uint64_t t = timestamp(value);
    return (FILETIME){(DWORD)t, (DWORD)(t >> 32)};
}
HANDLE CreateFileA(const char *path, DWORD access, DWORD share, void *security, DWORD disposition, DWORD flags, HANDLE template)
{
    (void)share; (void)security; (void)flags; (void)template;
    int entry=iso_lookup(path);
    if(entry!=-1){
        if((access&(GENERIC_WRITE|GENERIC_ALL|FILE_WRITE_ATTRIBUTES|DELETE))||disposition!=OPEN_EXISTING){errno=EROFS;fail();return INVALID_HANDLE_VALUE;}
        if(entry<0){errno=entry==-2?EISDIR:ENOENT;fail();return INVALID_HANDLE_VALUE;}
        HANDLE h=calloc(1,sizeof *h);if(!h){errno=ENOMEM;fail();return INVALID_HANDLE_VALUE;}
        h->fd=-1;h->iso_file=entry+1;strcpy(h->path,path);last_error=ERROR_SUCCESS;return h;
    }
    char native[PATH_MAX];
    if (!native_path(path, native)) return INVALID_HANDLE_VALUE;
    int options = O_CLOEXEC;
    if (access & (GENERIC_WRITE | GENERIC_ALL)) options |= (access & (GENERIC_READ | GENERIC_ALL)) ? O_RDWR : O_WRONLY;
    else options |= O_RDONLY;
    switch (disposition) {
    case CREATE_NEW: options |= O_CREAT | O_EXCL; break;
    case CREATE_ALWAYS: options |= O_CREAT | O_TRUNC; break;
    case OPEN_ALWAYS: options |= O_CREAT; break;
    case TRUNCATE_EXISTING: options |= O_TRUNC; break;
    case OPEN_EXISTING: break;
    default: errno = EINVAL; fail(); return INVALID_HANDLE_VALUE;
    }
    int fd = open(native, options, 0600);
    if (fd < 0) { fail(); return INVALID_HANDLE_VALUE; }
    HANDLE h = calloc(1, sizeof *h);
    if (!h) { close(fd); errno = ENOMEM; fail(); return INVALID_HANDLE_VALUE; }
    h->fd = fd;
    strcpy(h->path, native);
    last_error = ERROR_SUCCESS;
    return h;
}
HANDLE CreateFileW(const wchar_t *path, DWORD access, DWORD share, void *security, DWORD disposition, DWORD flags, HANDLE template)
{
    char utf8[PATH_MAX]; size_t at = 0;
    for (; path && *path; ++path) {
        uint32_t c = (uint32_t)*path;
        if (at + 4 >= sizeof utf8 || c > 0x10ffffu || (c >= 0xd800u && c <= 0xdfffu)) {
            errno = EINVAL; fail(); return INVALID_HANDLE_VALUE;
        }
        if (c < 0x80u) utf8[at++] = (char)c;
        else if (c < 0x800u) { utf8[at++] = (char)(0xc0u | (c >> 6)); utf8[at++] = (char)(0x80u | (c & 63)); }
        else if (c < 0x10000u) { utf8[at++] = (char)(0xe0u | (c >> 12)); utf8[at++] = (char)(0x80u | ((c >> 6) & 63)); utf8[at++] = (char)(0x80u | (c & 63)); }
        else { utf8[at++] = (char)(0xf0u | (c >> 18)); utf8[at++] = (char)(0x80u | ((c >> 12) & 63)); utf8[at++] = (char)(0x80u | ((c >> 6) & 63)); utf8[at++] = (char)(0x80u | (c & 63)); }
    }
    utf8[at] = 0;
    return CreateFileA(utf8, access, share, security, disposition, flags, template);
}
BOOL CloseHandle(HANDLE h)
{
    if (!h || h == INVALID_HANDLE_VALUE) { errno = EBADF; return fail(); }
    int result = h->iso_file||h->iso_find ? 0 : h->directory ? closedir(h->directory) : close(h->fd);
    free(h);
    return result == 0 ? 1 : fail();
}
BOOL ReadFile(HANDLE h, void *buffer, DWORD count, DWORD *got, void *overlapped)
{
    (void)overlapped;
    if (!h || h == INVALID_HANDLE_VALUE || !got) { errno = EBADF; return fail(); }
    if(h->iso_file){
        const struct IsoFile *file=&iso.files[h->iso_file-1];*got=0;
        if(h->iso_position>=file->size||count==0)return 1;
        uint64_t remaining=file->size-h->iso_position;size_t wanted=remaining<count?(size_t)remaining:count;
        ssize_t n;do{n=pread(iso.fd,buffer,wanted,(off_t)(file->offset+h->iso_position));}while(n<0&&errno==EINTR);
        if(n<=0){if(n==0)errno=EIO;return fail();}
        *got=(DWORD)n;h->iso_position+=(uint64_t)n;atomic_fetch_add(&iso_read_bytes,(uint64_t)n);return 1;
    }
    ssize_t n; do { n = read(h->fd, buffer, count); } while (n < 0 && errno == EINTR);
    *got = n < 0 ? 0 : (DWORD)n;
    return n < 0 ? fail() : 1;
}
BOOL WriteFile(HANDLE h, const void *buffer, DWORD count, DWORD *got, void *overlapped)
{
    (void)overlapped;
    if (!h || h == INVALID_HANDLE_VALUE || !got) { errno = EBADF; return fail(); }
    if(h->iso_file||h->iso_find){*got=0;errno=EROFS;return fail();}
    ssize_t n; do { n = write(h->fd, buffer, count); } while (n < 0 && errno == EINTR);
    *got = n < 0 ? 0 : (DWORD)n;
    return n < 0 ? fail() : 1;
}
BOOL SetFilePointerEx(HANDLE h, LARGE_INTEGER distance, LARGE_INTEGER *position, DWORD origin)
{
    if (!h || h == INVALID_HANDLE_VALUE || origin != FILE_BEGIN) { errno = EINVAL; return fail(); }
    if(h->iso_file){if(distance.QuadPart<0){errno=EINVAL;return fail();}h->iso_position=(uint64_t)distance.QuadPart;if(position)position->QuadPart=distance.QuadPart;return 1;}
    off_t n = lseek(h->fd, (off_t)distance.QuadPart, SEEK_SET);
    if (position) position->QuadPart = n;
    return n < 0 ? fail() : 1;
}
BOOL GetFileSizeEx(HANDLE h, LARGE_INTEGER *size)
{
    struct stat st;
    if (!h || h == INVALID_HANDLE_VALUE || !size) { errno = EBADF; return fail(); }
    if(h->iso_file){size->QuadPart=(int64_t)iso.files[h->iso_file-1].size;return 1;}
    if (fstat(h->fd, &st)) return fail();
    size->QuadPart = st.st_size; return 1;
}
BOOL SetEndOfFile(HANDLE h)
{
    if (!h || h == INVALID_HANDLE_VALUE) { errno = EBADF; return fail(); }
    if(h->iso_file||h->iso_find){errno=EROFS;return fail();}
    off_t position = lseek(h->fd, 0, SEEK_CUR);
    return position >= 0 && ftruncate(h->fd, position) == 0 ? 1 : fail();
}
BOOL FlushFileBuffers(HANDLE h)
{
    if (!h || h == INVALID_HANDLE_VALUE) { errno = EBADF; return fail(); }
    if(h->iso_file||h->iso_find)return 1;
    return fsync(h->fd) == 0 ? 1 : fail();
}
DWORD GetFileAttributesA(const char *path)
{
    int entry=iso_lookup(path);if(entry!=-1){if(entry==-3){errno=ENOENT;fail();return INVALID_FILE_ATTRIBUTES;}return FILE_ATTRIBUTE_READONLY|(entry==-2?FILE_ATTRIBUTE_DIRECTORY:FILE_ATTRIBUTE_NORMAL);}
    char native[PATH_MAX]; struct stat st;
    if (!native_path(path, native) || stat(native, &st)) { fail(); return INVALID_FILE_ATTRIBUTES; }
    return attributes(&st);
}
BOOL CreateDirectoryA(const char *path, void *security)
{
    if(iso_lookup(path)!=-1){errno=EROFS;return fail();}
    (void)security; char native[PATH_MAX];
    return native_path(path, native) && mkdir(native, 0700) == 0 ? 1 : fail();
}
BOOL RemoveDirectoryA(const char *path)
{
    if(iso_lookup(path)!=-1){errno=EROFS;return fail();}
    char native[PATH_MAX]; return native_path(path, native) && rmdir(native) == 0 ? 1 : fail();
}
BOOL DeleteFileA(const char *path)
{
    if(iso_lookup(path)!=-1){errno=EROFS;return fail();}
    char native[PATH_MAX]; return native_path(path, native) && unlink(native) == 0 ? 1 : fail();
}
BOOL FindNextFileA(HANDLE h, WIN32_FIND_DATAA *data)
{
    if(h&&h!=INVALID_HANDLE_VALUE&&h->iso_find&&data){
        if(h->iso_cursor>=iso.count)return 0;
        const struct IsoFile *file=&iso.files[h->iso_cursor++];memset(data,0,sizeof *data);strcpy(data->cFileName,file->name);
        data->dwFileAttributes=FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_NORMAL;data->nFileSizeLow=(DWORD)file->size;data->nFileSizeHigh=(DWORD)(file->size>>32);return 1;
    }
    if (!h || h == INVALID_HANDLE_VALUE || !h->directory || !data) { errno = EINVAL; return fail(); }
    struct dirent *entry;
    while ((entry = readdir(h->directory))) {
        char path[PATH_MAX]; struct stat st;
        if (strlen(h->path) + strlen(entry->d_name) + 2 >= sizeof path) continue;
        strcpy(path, h->path); strcat(path, "/"); strcat(path, entry->d_name);
        if (stat(path, &st)) continue;
        memset(data, 0, sizeof *data);
        strcpy(data->cFileName, entry->d_name);
        data->dwFileAttributes = attributes(&st);
        data->nFileSizeLow = (DWORD)st.st_size; data->nFileSizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
        data->ftCreationTime = filetime(st.st_ctim);
        data->ftLastWriteTime = filetime(st.st_mtim); data->ftLastAccessTime = filetime(st.st_atim);
        return 1;
    }
    return 0;
}
HANDLE FindFirstFileA(const char *pattern, WIN32_FIND_DATAA *data)
{
    if(pattern&&strlen(pattern)<PATH_MAX){
        char parent[PATH_MAX];strcpy(parent,pattern);for(char *p=parent;*p;++p)if(*p=='\\')*p='/';
        char *leaf=strrchr(parent,'/');if(leaf)*leaf=0;
        int entry=iso_lookup(parent);if(entry!=-1){
            if(entry!=-2){errno=ENOENT;fail();return INVALID_HANDLE_VALUE;}
            HANDLE h=calloc(1,sizeof *h);if(!h){errno=ENOMEM;fail();return INVALID_HANDLE_VALUE;}h->fd=-1;h->iso_find=1;
            if(!FindNextFileA(h,data)){CloseHandle(h);return INVALID_HANDLE_VALUE;}return h;
        }
    }
    char native[PATH_MAX];
    if (!native_path(pattern, native)) return INVALID_HANDLE_VALUE;
    char *leaf = strrchr(native, '/');
    if (leaf) *leaf = 0; else strcpy(native, ".");
    DIR *directory = opendir(*native ? native : "/");
    if (!directory) { fail(); return INVALID_HANDLE_VALUE; }
    HANDLE h = calloc(1, sizeof *h);
    if (!h) { closedir(directory); errno = ENOMEM; fail(); return INVALID_HANDLE_VALUE; }
    h->directory = directory; h->fd = -1; strcpy(h->path, *native ? native : "/");
    if (!FindNextFileA(h, data)) { CloseHandle(h); return INVALID_HANDLE_VALUE; }
    return h;
}
BOOL FindClose(HANDLE h) { return CloseHandle(h); }
BOOL GetFileInformationByHandleEx(HANDLE h, int kind, void *out, DWORD size)
{
    struct stat st;
    if (!h || h == INVALID_HANDLE_VALUE || kind != FileBasicInfo || size < sizeof(FILE_BASIC_INFO)) { errno = EINVAL; return fail(); }
    if (fstat(h->iso_file||h->iso_find?iso.fd:h->fd, &st)) return fail();
    FILE_BASIC_INFO *info = out;
    info->CreationTime.QuadPart = timestamp(st.st_ctim); info->LastAccessTime.QuadPart = timestamp(st.st_atim);
    info->LastWriteTime.QuadPart = timestamp(st.st_mtim); info->ChangeTime.QuadPart = timestamp(st.st_ctim);
    info->FileAttributes = h->iso_file||h->iso_find?FILE_ATTRIBUTE_READONLY|FILE_ATTRIBUTE_NORMAL:attributes(&st); return 1;
}
BOOL SetFileInformationByHandle(HANDLE h, int kind, const void *in, DWORD size)
{
    struct stat st;
    if (!h || h == INVALID_HANDLE_VALUE || kind != FileBasicInfo || size < sizeof(FILE_BASIC_INFO)) { errno = EINVAL; return fail(); }
    if(h->iso_file||h->iso_find){errno=EROFS;return fail();}
    if (fstat(h->iso_file||h->iso_find?iso.fd:h->fd, &st)) return fail();
    const FILE_BASIC_INFO *info = in;
    if (info->FileAttributes) {
        mode_t mode = st.st_mode;
        if (info->FileAttributes & FILE_ATTRIBUTE_READONLY) mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
        else mode |= S_IWUSR;
        if (fchmod(h->fd, mode)) return fail();
    }
    struct timespec times[2] = {st.st_atim, st.st_mtim};
    const int64_t values[2] = {info->LastAccessTime.QuadPart, info->LastWriteTime.QuadPart};
    for (unsigned i = 0; i < 2; ++i) if (values[i] > 0) {
        times[i].tv_sec = values[i] / INT64_C(10000000) - INT64_C(11644473600);
        times[i].tv_nsec = (values[i] % INT64_C(10000000)) * 100;
    }
    return futimens(h->fd, times) == 0 ? 1 : fail();
}
HANDLE GetCurrentProcess(void) { return (HANDLE)(intptr_t)-2; }
BOOL TerminateProcess(HANDLE process, unsigned code) { (void)process; _exit((int)code); }
