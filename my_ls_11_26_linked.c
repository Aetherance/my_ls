#include<stdio.h>
#include<sys/stat.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>
#include<getopt.h>
#include<ctype.h>

// ifm
struct ifm
{
    struct dirent *rdirent;
    struct stat buf__stat;

    struct ifm * next;
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
    
    struct ifm * ifmlist =  malloc(sizeof(struct ifm)); // head
    struct ifm * cur = ifmlist,*end;

    // name 和 statbuf存放

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


        cur->next = (struct ifm*)malloc(sizeof(struct ifm));
        end = cur;
        cur = cur->next;
    }
    
    free(end->next);
    end->next = NULL;

    // sort

    


    // read from ifm
    struct ifm * readifm = ifmlist;
    while(readifm!=NULL)
    {
        if(!optable['a'])
            if(!strcmp(readifm->rdirent->d_name,".")||!strcmp(readifm->rdirent->d_name,"..")||*readifm->rdirent->d_name=='.')
            {
                readifm = readifm->next;
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
        
        
        readifm = readifm->next;
    }
    
    // free

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
    

    printf("\n");
    return  0;
}