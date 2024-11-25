#include<stdio.h>
#include<sys/stat.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>

struct ifm
{
    struct dirent *rdirent;
    struct stat buf__stat;

    struct ifm * next;
};


int main(int argc,char **argv)
{
    if(argc==1)
        argv[1] = ".";
    
    DIR * dir = opendir(argv[1]);
    struct dirent * rdirent;
    struct stat buf__stat;    // 开辟空间
    struct stat * statbuf = &buf__stat;

    if(dir == NULL)
    {
        printf("ls: 无法访问 '%s': 没有那个文件或目录\n",argv[1]);
        return 1;
    }
    
    struct ifm * ifmlist =  malloc(sizeof(struct ifm)); // head
    struct ifm * cur = ifmlist,*end;

    // name 和 statbuf存放

    while((cur->rdirent = readdir(dir))!=NULL)
    {
        char path[1000];
        sprintf(path,"%s/%s",argv[1],cur->rdirent->d_name);
        stat(path,&cur->buf__stat);


        cur->next = (struct ifm*)malloc(sizeof(struct ifm));
        end = cur;
        cur = cur->next;
    }
    
    free(end->next);
    end->next = NULL;

    struct ifm * readifm = ifmlist;
    while(readifm->next!=NULL)
    {
        if(!strcmp(rdirent->d_name,".")||!strcmp(rdirent->d_name,"..")||*rdirent->d_name=='.')
            continue;
        
    // print
        
        if(S_ISREG(readifm->buf__stat.st_mode)&&!(readifm->buf__stat.st_mode & S_IXUSR))printf("%s  ",readifm->rdirent->d_name);
        if(S_ISREG(readifm->buf__stat.st_mode)&& (readifm->buf__stat.st_mode & S_IXUSR))printf("\033[;1;32m%s  \033[0m",readifm->rdirent->d_name);
        if(S_ISDIR(readifm->buf__stat.st_mode))printf("\033[;1;34m%s  \033[0m",readifm->rdirent->d_name);
        readifm = readifm->next;
    }
    
    //释放

    struct ifm * ifm_free = ifmlist;
    while(ifm_free->next!=NULL)
    {
        struct ifm * temp = ifm_free;
        free(ifm_free->next);
        ifm_free = ifm_free->next;
        temp->next = NULL;
    }
    free(ifmlist);
    ifmlist = NULL;
    


    return  0;
}