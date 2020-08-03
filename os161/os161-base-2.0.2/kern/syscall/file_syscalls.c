#include "syscall.h"
#include "lib.h"
#include "kern/unistd.h"
#include "types.h"
#include "limits.h"
#include "vfs.h"
#include "uio.h"
#include "vnode.h"
#define openMax 128


int minFd=3; //0 1 2 -> stdin stdout stderr
struct vnode* fileTable [openMax];
int fileTableIndex [openMax]={0};

int sys_write(int fd,char* buf,int count){
    int i,result;
    struct uio userio;
    struct iovec iov;
    //Scrittura su file
    if (fd!=STDOUT_FILENO && fd!= STDERR_FILENO){
         kprintf("fd da scrivere %d\n",fd);
        //verifico se il file è aperto
        for (i=0;i<openMax;i++){
            if (fileTableIndex[i]==fd){
                uio_kinit(&iov,&userio,buf,count,0,UIO_WRITE);
                result=VOP_WRITE(fileTable[i],&userio);
                kprintf("%d momo\n",result);
                if (result)
                    return -1;
                return count;
            }

        }
        kprintf("Scrittura su file andata male fratellì\n");
        return -1;
    }
    // Scrittura su stdout o stderr
    if (fd==STDOUT_FILENO || fd==STDERR_FILENO)
        for (i=0;i<count;i++)
            putch(buf[i]);
    
    return count;
}

int sys_read(int fd,char* buf,int count){
    int ch,i,result;
    struct uio userio;
    struct iovec iov;
    if (fd!=STDIN_FILENO){
        //verifico se il file è aperto
        for (i=0;i<openMax;i++){
            if (fileTableIndex[i]==fd){
                uio_kinit(&iov,&userio,buf,count,0,UIO_READ);
                result=VOP_READ(fileTable[i],&userio);
                if (result)
                    return -1;
                return count;
            }

        }
        kprintf("Lettura da file andata male fratellì\n");
        return -1;
    }
    for (i=0;i<count;i++){
        ch=getch();
        if (ch<0)
            return i;
        buf[i]=ch;
    }
    return count;
}

//return a new fd or -1 if error
int sys_open(char* pathname, int flags){
    struct vnode *v;
    int result;
    int i;
    /* Open the file. */
    result = vfs_open(pathname, flags, 0, &v);
    kprintf(" risultato vfs_open=%d\n",result);
    if (result) {
        return -1;
    }
    //cerco un file descriptor valido da ritornare
    for (i=0;i<openMax;i++){
        if(fileTableIndex[i]==0){
            fileTable[i]=v;
            fileTableIndex[i]=minFd+i;
            kprintf("Fd del file %s è %d\n",pathname,minFd+i);
            return minFd+i;
        }
    }
    kprintf("errore apertura file\n");
    return -1;
}

int sys_close(int fd){
    //verifico se il file è effettivamente aperto
    int i;
    for (i=0;i<openMax;i++)
        if (fileTableIndex[i]==fd){
        fileTable[i]=NULL;
        fileTableIndex[i]=0;
        return 0;

    }
    //se file non è aperto ritorno -1 errore
    return -1;
}
int sys_remove(char* file){
    kprintf("%s",file);
    return 1;
}