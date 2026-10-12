#include "host_file_compat.h"
#include "iso_disc.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void){
    char image[160];snprintf(image,sizeof image,"/data/local/tmp/doa3-iso-test-%d",(int)getpid());
    int fd=open(image,O_CREAT|O_EXCL|O_RDWR,0600);assert(fd>=0);
    uint64_t base=UINT64_C(6)*1024*1024*1024;assert(ftruncate(fd,(off_t)(base+4096))==0);
    const char *names[]={"Default.xbe","loadfile.afs"};uint64_t offsets[]={128,base},sizes[]={6,10};
    assert(pwrite(fd,"ABCDEF",6,128)==6&&pwrite(fd,"0123456789",10,(off_t)base)==10);
    assert(recomp_iso_mount(fd,"/virtual-disc",2,names,offsets,sizes));
    const char *duplicates[]={"a","A"};assert(!recomp_iso_mount(fd,"/virtual-disc",2,duplicates,offsets,sizes));
    const char *traversal[]={"../a","b"};assert(!recomp_iso_mount(fd,"/virtual-disc",2,traversal,offsets,sizes));
    uint64_t outside[]={128,base+4096};assert(!recomp_iso_mount(fd,"/virtual-disc",2,names,outside,sizes));
    uint64_t overlap[]={128,130};assert(!recomp_iso_mount(fd,"/virtual-disc",2,names,overlap,sizes));
    close(fd); /* The mount owns its descriptor, as with a closed Android PFD. */
    assert((GetFileAttributesA("/virtual-disc")&FILE_ATTRIBUTE_DIRECTORY)!=0);
    assert(GetFileAttributesA("/virtual-disc/missing")==INVALID_FILE_ATTRIBUTES);
    HANDLE a=CreateFileA("/virtual-disc/DEFAULT.XBE",GENERIC_READ,0,0,OPEN_EXISTING,0,0);
    HANDLE b=CreateFileW(L"/virtual-disc\\loadFILE.afs",GENERIC_READ,0,0,OPEN_EXISTING,0,0);
    HANDLE other=CreateFileA("/virtual-disc/default.xbe",GENERIC_READ,0,0,OPEN_EXISTING,0,0);
    assert(a!=INVALID_HANDLE_VALUE&&b!=INVALID_HANDLE_VALUE&&other!=INVALID_HANDLE_VALUE);
    LARGE_INTEGER length,pos={.QuadPart=7};assert(GetFileSizeEx(b,&length)&&length.QuadPart==10);
    char buffer[40]={0};DWORD got=0;assert(ReadFile(a,buffer,2,&got,0)&&got==2&&!memcmp(buffer,"AB",2));
    assert(ReadFile(other,buffer,6,&got,0)&&got==6&&!memcmp(buffer,"ABCDEF",6));
    assert(SetFilePointerEx(b,pos,0,FILE_BEGIN)&&ReadFile(b,buffer,sizeof buffer,&got,0)&&got==3&&!memcmp(buffer,"789",3));
    assert(ReadFile(b,buffer,sizeof buffer,&got,0)&&got==0);
    pos.QuadPart=1000;assert(SetFilePointerEx(a,pos,0,FILE_BEGIN)&&ReadFile(a,buffer,4,&got,0)&&got==0);
    pos.QuadPart=-1;assert(!SetFilePointerEx(a,pos,0,FILE_BEGIN));
    FILE_BASIC_INFO info;assert(GetFileInformationByHandleEx(a,FileBasicInfo,&info,sizeof info)&&(info.FileAttributes&FILE_ATTRIBUTE_READONLY));
    assert(!WriteFile(a,"bad",3,&got,0)&&GetLastError()==ERROR_ACCESS_DENIED);
    assert(!SetEndOfFile(a)&&!SetFileInformationByHandle(a,FileBasicInfo,&info,sizeof info));
    assert(CreateFileA("/virtual-disc/default.xbe",GENERIC_WRITE,0,0,OPEN_EXISTING,0,0)==INVALID_HANDLE_VALUE);
    assert(CreateFileA("/virtual-disc/new",GENERIC_READ,0,0,CREATE_ALWAYS,0,0)==INVALID_HANDLE_VALUE);
    assert(!DeleteFileA("/virtual-disc/default.xbe")&&!CreateDirectoryA("/virtual-disc/new",0));
    WIN32_FIND_DATAA data;HANDLE find=FindFirstFileA("/virtual-disc\\*",&data);assert(find!=INVALID_HANDLE_VALUE&&!strcmp(data.cFileName,"Default.xbe"));
    assert(FindNextFileA(find,&data)&&!strcmp(data.cFileName,"loadfile.afs")&&data.nFileSizeLow==10&&!FindNextFileA(find,&data));assert(FindClose(find));
    CloseHandle(a);CloseHandle(other);CloseHandle(b);
    HANDLE real=CreateFileA(image,GENERIC_READ|GENERIC_WRITE,0,0,OPEN_EXISTING,0,0);assert(real!=INVALID_HANDLE_VALUE);
    pos.QuadPart=0;assert(SetFilePointerEx(real,pos,0,FILE_BEGIN)&&WriteFile(real,"ok",2,&got,0)&&got==2);CloseHandle(real);
    assert(recomp_iso_read_bytes()==11);
    recomp_iso_unmount();assert(unlink(image)==0);
    puts("ISO VFS: >4 GiB offsets, independent cursors, case/separators, EOF, directory/attributes, invalid extents and read-only protection passed.");return 0;
}
