#include<stdio.h>
#include<sys/stat.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>
#include<getopt.h>
#include<ctype.h>
typedef int(*FP)(const void *, const void *);

// ifm
struct ifm
{
    struct dirent *rdirent;
    struct stat buf__stat;
};

int opt_count_sum = 0;
int opt;    // temp
char optable[256] = {};  // 01 table
char filepath[128] = ".";

int main(int argc,char **argv)
{
    // getopt
    while((opt = getopt(argc,argv,"a"))!=-1)
    {
        optable[opt] = 1;
        //opt_count_sum += optable[opt];
    }

    char ** arcu = argv+1;
    while(*arcu!=NULL)
    {
        if(strcmp(*arcu,"-a"))
        {
            strcpy(filepath,*arcu);
            break; 
        }
        arcu++;
    }


    // open

    DIR * dir = opendir(filepath);
    struct dirent * rdirent;
    struct stat buf__stat;    // 开辟空间
    struct stat * statbuf = &buf__stat;

    if(dir == NULL)
    {
        printf("ls: 无法访问 '%s': 没有那个文件或目录\n",filepath);
        return 1;
    }
    
    struct ifm ifmlist[25565];// list
    struct ifm * cur = ifmlist,*end;

    // name 和 statbuf存放

    int count = 0;
    int maxlen = 0;     //最长文件名
    while((cur->rdirent = readdir(dir))!=NULL)
    {
        // get_max_len
        if(strlen(cur->rdirent->d_name)>maxlen)
            maxlen = strlen(cur->rdirent->d_name)+1;

        maxlen = maxlen>7?7:maxlen;


        char path[1000];
        sprintf(path,"%s/%s",filepath,cur->rdirent->d_name);
        stat(path,&cur->buf__stat);

        count++;
        cur++;
    }
    

    // sort

    int sort_init(const void * ptr1, const void * ptr2)
    {
        struct ifm * pos  = (struct ifm*)ptr1, * aftpos = (struct ifm*)ptr2;
        if(strcmp(pos->rdirent->d_name,aftpos->rdirent->d_name)>0)
            return 1;
        if(strcmp(pos->rdirent->d_name,aftpos->rdirent->d_name)<0)
            return -1;
        return 0;
    }

    // FP sort_mode = sort_init;
    // qsort(ifmlist,count,sizeof(struct ifm),sort_mode);






    // read from ifm
    struct ifm * readifm = ifmlist;
    while(readifm!=cur)
    {
        if(!optable['a'])
            if(!strcmp(readifm->rdirent->d_name,".")||!strcmp(readifm->rdirent->d_name,"..")||*readifm->rdirent->d_name=='.')
            {
                readifm++;
                continue;
            }

    // print
        
        if(S_ISREG(readifm->buf__stat.st_mode)
        &&!(readifm->buf__stat.st_mode & S_IXUSR))
            printf("%-*s",maxlen,readifm->rdirent->d_name);
        
        if(S_ISREG(readifm->buf__stat.st_mode)
        &&(readifm->buf__stat.st_mode & S_IXUSR))
            printf("\033[1;32m%-*s\033[0m",maxlen,readifm->rdirent->d_name);
        
        if(S_ISDIR(readifm->buf__stat.st_mode))
            printf("\033[1;34m%-*s\033[0m",maxlen,readifm->rdirent->d_name);
        
        printf(" ");
        
        
        readifm ++;
    }
    

    printf("\n");
    return  0;
}