// 加入了 -s -i

#include<stdio.h>
#include<sys/stat.h>
#include<string.h>
#include<dirent.h>
#include<getopt.h>
#include<ctype.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<assert.h>

#define LIST_SIZE 2097152
#define PATH_SIZE 1000
#define FILE_PATH_SIZE 128

#define OPT__a_ 'a'
#define OPT__l_ 'l'
#define OPT__t_ 't'
#define OPT__r_ 'r'
#define OPT__R_ 'R'
#define OPT__i_ 'i'
#define OPT__s_ 's'

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
char filepath[FILE_PATH_SIZE] = ".";

int main(int argc,char **argv)
{
    // getopt
    while((opt = getopt(argc,argv,"alRtris"))!=-1)
    {
        optable[opt] = 1;
        //opt_count_sum += optable[opt];
    }

    char ** arcu = argv+1;
    while(*arcu!=NULL)
    {
        if(**arcu != '-')
        {
            strcpy(filepath,*arcu);
            break; 
        }
        arcu++;
    }

    // open
    DIR * dir = opendir(filepath);
    struct dirent * rdirent;
    struct stat buf__stat;    // 静态开辟空间
    struct stat * statbuf = &buf__stat;

    if(dir == NULL)
    {
        printf("ls: 无法访问 '%s': 没有那个文件或目录\n",filepath);
        return 1;
    }
    
    static struct ifm ifmlist[LIST_SIZE];// list
    struct ifm * cur = ifmlist,*end;

    // name 和 statbuf存放

    int all_name_count = 0;
    static int total_name_len = 0;
    static int temp_line_len = 0;
    static int max_len = 0;     //最长文件名
    static int line_print_now = 0;
    static int divide_count = 0;
    static size_t blockSum = 0;


    // 读目录

    while((cur->rdirent = readdir(dir))!=NULL)
    {
        // get_max_len
        if(strlen(cur->rdirent->d_name)>max_len)
            max_len = strlen(cur->rdirent->d_name)+1;

        //max_len = max_len>7?7:max_len;


        char path[PATH_SIZE];
        sprintf(path,"%s/%s",filepath,cur->rdirent->d_name);
        stat(path,&cur->buf__stat);

        total_name_len += strlen(cur->rdirent->d_name) + 2; // 确定总长度 判断是否需要切换输出模式
        //total_name_len-=2;  // 减去最后的两个空格
        if(optable[OPT__s_]) total_name_len += 3;
        if(optable[OPT__i_]) total_name_len += 8;

        if(optable[OPT__a_])blockSum += cur->buf__stat.st_blocks/2; // why
        else if((cur->rdirent->d_name)[0]!='.')blockSum += cur->buf__stat.st_blocks/2;

        all_name_count++;
        cur++;
    }


    // -r
    
    int order = optable[OPT__r_] ? -1 : 1 ;

    // sort

    int sort_init(const void * ptr1, const void * ptr2)
    {
        struct ifm * pos  = (struct ifm*)ptr1, * aftpos = (struct ifm*)ptr2;
        
        // 如果前面的部分一样，把长的排在后面
        if(strstr(pos->rdirent->d_name,aftpos->rdirent->d_name)!=NULL)
            return 1;
        if(strstr(aftpos->rdirent->d_name,pos->rdirent->d_name)!=NULL)
            return -1;

        if(*(pos->rdirent->d_name)=='.'&&(*(pos->rdirent->d_name+1)!='.'&&*(pos->rdirent->d_name+1)!='\0'))
        {
            if(strstr(pos->rdirent->d_name,aftpos->rdirent->d_name)!=NULL)
                return 1;
            if(strstr(aftpos->rdirent->d_name,pos->rdirent->d_name)!=NULL)
                return -1;
            
            return strcmp(pos->rdirent->d_name+1,aftpos->rdirent->d_name+1) * order; 
        }
        return strcmp(pos->rdirent->d_name,aftpos->rdirent->d_name) * order;
    }
    
    int sort_by_change_time(const void * ptr1, const void * ptr2)
    {
        struct ifm * pos  = (struct ifm*)ptr1, * aftpos = (struct ifm*)ptr2;
        if(pos->buf__stat.st_ctime<aftpos->buf__stat.st_ctime)
            return 1 * order;
        if(pos->buf__stat.st_ctime>aftpos->buf__stat.st_ctime)
            return -1 * order;
        return 0;

    }

    // get_sort_mode

    FP sort_mode = sort_init;
    if(optable[OPT__t_])sort_mode = sort_by_change_time;
    qsort(ifmlist,all_name_count,sizeof(struct ifm),sort_mode);



    // get_print_format

    static struct winsize win;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&win);
    divide_count = total_name_len / win.ws_col + 1;
        

    // opt_s using

    if(optable[OPT__s_])printf("总计 %zu\n",blockSum);

    // read from ifm
    struct ifm * readifm = ifmlist;
    while(readifm!=cur)
    {
        if(!optable[OPT__a_])
            if(    !strcmp(readifm->rdirent->d_name,".")||!strcmp(readifm->rdirent->d_name,"..")
            ||*readifm->rdirent->d_name=='.')
                {
                    readifm++;
                    continue;
                }

    // print

        // 输出格式
        if(!optable[OPT__l_]&&line_print_now == all_name_count/divide_count-1)
        {
            printf("\b\b");
            printf("\n");
            temp_line_len = 0;
            line_print_now = 0;
        }

        if(optable[OPT__i_])
        {
            printf("%lu ",readifm->rdirent->d_ino);
        }

        if(optable[OPT__s_])
        {
            printf("%2lu ",readifm->buf__stat.st_blocks/2);     // why
        }

        if(optable[OPT__l_])    // -l
        {
            // 第一列
            if(S_ISDIR(readifm->buf__stat.st_mode))printf("d");
            else if(S_ISLNK(readifm->buf__stat.st_mode))printf("l");
            else if(S_ISBLK(readifm->buf__stat.st_mode))printf("d");
            else if(S_ISCHR(readifm->buf__stat.st_mode))printf("c");
            else if(S_ISCHR(readifm->buf__stat.st_mode))printf("c");
            else printf("-");

            // 所有者权限

            



            //所属组权限





            //其他用户权限





            printf("\n");
        }
        else if(total_name_len<=win.ws_col)
        {

            if(S_ISREG(readifm->buf__stat.st_mode)
            &&!(readifm->buf__stat.st_mode & S_IXUSR))
                printf("%s",readifm->rdirent->d_name);
            
            if(S_ISREG(readifm->buf__stat.st_mode)
            &&(readifm->buf__stat.st_mode & S_IXUSR))
                printf("\033[1;32m%s\033[0m",readifm->rdirent->d_name);
            
            if(S_ISDIR(readifm->buf__stat.st_mode))
                printf("\033[1;34m%s\033[0m",readifm->rdirent->d_name);
            
            printf("  ");
        }
        else
        {
            temp_line_len += strlen(readifm->rdirent->d_name)+2;


            if(S_ISREG(readifm->buf__stat.st_mode)
            &&!(readifm->buf__stat.st_mode & S_IXUSR))
                printf("%-*s",max_len,readifm->rdirent->d_name);
            
            if(S_ISREG(readifm->buf__stat.st_mode)
            &&(readifm->buf__stat.st_mode & S_IXUSR))
                printf("\033[1;32m%-*s\033[0m",max_len,readifm->rdirent->d_name);
            
            if(S_ISDIR(readifm->buf__stat.st_mode))
                printf("\033[1;34m%-*s\033[0m",max_len,readifm->rdirent->d_name);
            
            printf(" ");
        }
        
        
        line_print_now ++;
        readifm ++;
    }
    

    if(!optable[OPT__l_])printf("\n");
    return  0;
}