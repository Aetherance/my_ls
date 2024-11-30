#include<stdio.h>
#include<sys/stat.h>
#include<string.h>
#include<dirent.h>
#include<getopt.h>
#include<ctype.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>

#define LIST_SIZE 20970
#define PATH_SIZE 40480
#define FILE_PATH_SIZE 2048
#define FILE_COUNT_MAX 25600

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
char optTable[256] = {};  // 01 table
char filepath[FILE_COUNT_MAX][FILE_PATH_SIZE] = {"."};
int FileNameCount = 1;
int FileNameRead = 0;
char fatherPath[10000];

// sort
int order = 1;

int sort_init(const void * ptr1, const void * ptr2)
{
    struct ifm * pos  = (struct ifm*)ptr1, * aftpos = (struct ifm*)ptr2;
    
    if(*(pos->rdirent->d_name)=='.'||(*(aftpos->rdirent->d_name)=='.'&&*(pos->rdirent->d_name+1)!='\0'))
    {
        if(strstr(pos->rdirent->d_name+1,aftpos->rdirent->d_name+1)!=NULL)
            return 1 * order;
        if(strstr(aftpos->rdirent->d_name+1,pos->rdirent->d_name+1)!=NULL)
            return -1 * order;
        
        return strcmp(pos->rdirent->d_name+1,aftpos->rdirent->d_name+1) * order; 
    }
    // 如果前面的部分一样，把长的排在后面
    if(strstr(pos->rdirent->d_name,aftpos->rdirent->d_name)!=NULL)
        return 1 * order;
    if(strstr(aftpos->rdirent->d_name,pos->rdirent->d_name)!=NULL)
        return -1 * order;

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

int main(int argc,char **argv)
{   
    //optTable[OPT__R_] = 1;
    strcpy(fatherPath,filepath[0]);
    // getopt
    while((opt = getopt(argc,argv,"alRtris"))!=-1)
    {
        optTable[opt] = 1;
        opt_count_sum += optTable[opt];
    }

    char ** arcu = argv+1;

    if(argc>1&&opt_count_sum<argc-1)FileNameCount--;

    while(*arcu!=NULL)
    {
        if(**arcu != '-')
        {
            strcpy(filepath[FileNameCount++],*arcu);
        }
        arcu++;
    }

    while (FileNameRead<FileNameCount)
    {
        if(FileNameCount>1||optTable[OPT__R_])
            printf("%s:\n",filepath[FileNameRead]);
        // open
        DIR * dir = opendir(filepath[FileNameRead]);
        struct dirent * rdirent;
        struct stat buf__stat;    // 静态开辟空间
        struct stat * statbuf = &buf__stat;

        if(dir == NULL)
        {
            printf("ls: 无法访问 '%s': 没有那个文件或目录\n",filepath[FileNameRead]);
            if(optTable[OPT__R_])
            {
                FileNameRead ++;
                continue;
            }
            return 1;
        }
        
        static struct ifm ifmlist[LIST_SIZE];// list
        struct ifm * cur = ifmlist,*end;
        
        int all_name_count = 0;
        int total_name_len = 0;
        int temp_line_len = 0;
        int max_len = 0;     //最长文件名
        int line_print_now = 0;
        int divide_count = 0;
        size_t blockSum = 0;
        int fileSizeLenMax = 0;

        // 读目录

        while((cur->rdirent = readdir(dir))!=NULL)
        {
            // get_max_len
            if(strlen(cur->rdirent->d_name)>max_len)
                max_len = strlen(cur->rdirent->d_name)+1;

            //max_len = max_len>7?7:max_len;

            char path[PATH_SIZE];
            sprintf(path,"%s/%s",filepath[FileNameRead],cur->rdirent->d_name);
            stat(path,&cur->buf__stat);

            total_name_len += strlen(cur->rdirent->d_name) + 2; // 确定总长度 判断是否需要切换输出模式
            //total_name_len-=2;  // 减去最后的两个空格
            if(optTable[OPT__s_]) total_name_len += 3;
            if(optTable[OPT__i_]) total_name_len += 8;

            if(optTable[OPT__a_])blockSum += cur->buf__stat.st_blocks/2; // why
            else if((cur->rdirent->d_name)[0]!='.')blockSum += cur->buf__stat.st_blocks/2;

            int temp = cur->buf__stat.st_size;
            int fileSizeLen = 0;
            while(temp)
                temp/=10,fileSizeLen ++;
            if(fileSizeLen>fileSizeLenMax)
                fileSizeLenMax = fileSizeLen;
            
            all_name_count++;
            cur++;
        }


        // -r
        order = optTable[OPT__r_] ? -1 : 1 ;
        

        // get_sort_mode

        FP sort_mode = sort_init;
        if(optTable[OPT__t_])sort_mode = sort_by_change_time;
        qsort(ifmlist,all_name_count,sizeof(struct ifm),sort_mode);



        // get_print_format

        static struct winsize win;
        ioctl(STDIN_FILENO,TIOCGWINSZ,&win);
        divide_count = total_name_len / win.ws_col + 1;
            

        // opt_s using

        if(optTable[OPT__l_]||optTable[OPT__s_])printf("总计 %zu\n",blockSum);

        // read from ifm
        struct ifm * readifm = ifmlist;
        while(readifm!=cur)
        {
            if(!optTable[OPT__a_])
                if
                (!strcmp(readifm->rdirent->d_name,".")||!strcmp(readifm->rdirent->d_name,"..")
                ||*readifm->rdirent->d_name=='.'){
                        readifm++;
                        continue;
                    }

        // print

        void PrintList()
        {

            if(S_ISREG(readifm->buf__stat.st_mode)
            &&!(readifm->buf__stat.st_mode & S_IXUSR))
                printf("%s",readifm->rdirent->d_name);
                
            if(S_ISREG(readifm->buf__stat.st_mode)
            &&(readifm->buf__stat.st_mode & S_IXUSR))
                printf("\033[1;32m%s\033[0m",readifm->rdirent->d_name);
                
            if(S_ISDIR(readifm->buf__stat.st_mode))
                printf("\033[1;34m%s\033[0m",readifm->rdirent->d_name);
                
            if(S_ISLNK(readifm->buf__stat.st_mode))
                printf("\033[1;44m%s\033[0m",readifm->rdirent->d_name);
                
                printf("  ");
        }

            // 输出格式
            if(!optTable[OPT__l_]&&line_print_now == ((all_name_count/divide_count-1))/(optTable[OPT__R_]?2:1))
            {
                printf("\b\b");
                printf("\n");
                temp_line_len = 0;
                line_print_now = 0;
            }

            if(optTable[OPT__i_])
            {
                printf("%lu ",readifm->rdirent->d_ino);
            }

            if(optTable[OPT__s_])
            {
                printf("%2lu ",readifm->buf__stat.st_blocks/2);     // why
            }

            if(optTable[OPT__l_])    // -l
            {
                // 第一列
                if(S_ISDIR(readifm->buf__stat.st_mode))printf("d");
                else if(S_ISLNK(readifm->buf__stat.st_mode))printf("l");
                else if(S_ISBLK(readifm->buf__stat.st_mode))printf("d");
                else if(S_ISCHR(readifm->buf__stat.st_mode))printf("c");
                else if(S_ISCHR(readifm->buf__stat.st_mode))printf("c");
                else printf("-");

                // 所有者权限
                printf(readifm->buf__stat.st_mode & S_IRUSR?"r":"-");
                printf(readifm->buf__stat.st_mode & S_IWUSR?"w":"-");
                printf(readifm->buf__stat.st_mode & S_IXUSR?"x":"-");
                //所属组权限
                printf(readifm->buf__stat.st_mode & S_IRGRP?"r":"-");
                printf(readifm->buf__stat.st_mode & S_IWGRP?"w":"-");
                printf(readifm->buf__stat.st_mode & S_IXGRP?"x":"-");

                //其他用户权限
                printf(readifm->buf__stat.st_mode & S_IROTH?"r":"-");
                printf(readifm->buf__stat.st_mode & S_IWOTH?"w":"-");
                printf(readifm->buf__stat.st_mode & S_IXOTH?"x":"-");

                printf(" %lu",readifm->buf__stat.st_nlink);
                printf(" %s",getpwuid(readifm->buf__stat.st_uid)->pw_name);
                printf(" %s",getgrgid(readifm->buf__stat.st_gid)->gr_name);
                printf("%*lu ",fileSizeLenMax+1,readifm->buf__stat.st_size);
                
                struct tm * time = localtime(&readifm->buf__stat.st_mtime);
                char time_buf[64];
                strftime(time_buf, 64, "%m月 %d %H:%M",time);
                printf("%s ",time_buf);

                PrintList();

                printf("\n");
            }
            else if(!optTable[OPT__R_]&&total_name_len<=win.ws_col)
            {
                PrintList();
            }
            else
            {
                temp_line_len += strlen(readifm->rdirent->d_name)+2;
                temp_line_len += strlen(readifm->rdirent->d_name)+2;
                if((readifm->buf__stat.st_mode & __S_IFLNK))
                    printf("%-*s",max_len,readifm->rdirent->d_name);

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
        
        if(!optTable[OPT__l_])printf("\n");
        FileNameRead ++;
        if(FileNameCount>1&&FileNameCount!=FileNameRead)printf("\n");
        
        if(optTable[OPT__R_])
        {
            if(FileNameCount<2048){
                strcpy(fatherPath,filepath[0]);
                for(struct ifm * RfileNameRead = ifmlist;RfileNameRead<readifm;RfileNameRead++)
                {
                    if(S_ISDIR(RfileNameRead->buf__stat.st_mode)&&*RfileNameRead->rdirent->d_name!='.')
                    {
                        sprintf(filepath[FileNameCount++],"%s/%s",fatherPath,RfileNameRead->rdirent->d_name);
                    }
                }
            }
            for(int i = 1;i<FileNameCount;i++)
            {
                strcpy(filepath[i-1],filepath[i]);
            }
            //strcpy(fatherPath,filepath[0]);

            FileNameCount--;
            FileNameRead = 0;
        }
    }
    return  0;
}