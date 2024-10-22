#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>

using namespace std;

const int basesize=1024;
const int argvnum=64;
const int envnum=64;
//全局的命令行参数表
char*gargv[argvnum];
int gargc=0;
int lastcode=0;
//环境变量
char*genv[envnum];

//全局的工作路径
char pwd[basesize];
char pwdenv[basesize];


string GetPwd(){
    if(nullptr==getcwd(pwd,sizeof(pwd)))return "None";
    snprintf(pwdenv,sizeof(pwdenv),"PWD=%s",pwd);
    putenv(pwdenv);
    return pwd;

}
string GetEnvVar(const char* varname) {
    const char* value = getenv(varname);
    return value ? string(value) : "None";
}

string GetHostName() {
    return GetEnvVar("HOSTNAME");
}

string GetUserName() {
    return GetEnvVar("USER");
}
//从父shell中获取环境变量
void InitEnv(){
    extern char**environ;
    int index=0;
    while(environ[index]){
        genv[index]=(char*)malloc(strlen(environ[index])+1);
        strncpy(genv[index],environ[index],strlen(environ[index])+1);
        index++;
    }
    genv[index]=nullptr;
}

string LastDir(){
    string cuur = GetPwd();
    if(cuur=="/"||cuur=="None")return cuur;
    size_t pos=cuur.rfind("/");
    if(pos==std::string::npos)return cuur;
    return cuur.substr(pos+1);
}

void AddEnv(const char*item){
    int index=0;
    while(genv[index])index++;
    genv[index]=(char*)malloc(strlen(item)+1);
    strncpy(genv[index],item, strlen(item)+1);
    genv[++index]=nullptr;
}



void debug(){
    printf("argc: %d\n",gargc);
    for(int i=0;gargv[i];i++){
        printf("argv[%d]: %s\n",i,gargv[i]);
    }
}

bool GetCommandLine(char command_buffer[], int size){//获取命令
    char*result=fgets(command_buffer,size,stdin);
    if(!result){
        return false;
    }
    command_buffer[strlen(command_buffer)-1]=0;
    if(strlen(command_buffer)==0)return false;
    return true;
}

void ParseCommandLine(char command_buffer[], int len){//分析命令
    (void)len;
    memset(gargv,0,sizeof(gargv));
    gargc=0;
    const char*sep=" ";
    gargv[gargc++]=strtok(command_buffer,sep);
    while((bool)(gargv[gargc++]=strtok(nullptr,sep)));
    gargc--;
}

bool ExecuteCommand(){//执行命令
    //子进程进行
    pid_t id=fork();
    if(id<0)return false;
    if(id==0)//子进程
    {
        execvpe(gargv[0],gargv,genv);
        exit(1);
    }
    int status=0; 
    pid_t rid=waitpid(id,&status,0);
    if(id>0){
        if(WIFEXITED(status)){
            lastcode=WEXITSTATUS(status);
        }
        else{
            lastcode=100;
        }
        return true;
    }
    return false;
}

string MakeCommandLine(){
    char command_line[basesize];
    snprintf(command_line,basesize,"[%s@%s %s]# ",GetUserName().c_str(),GetHostName().c_str(),LastDir().c_str());
    return command_line;
}

void PrintCommandLine(){
    printf("%s",MakeCommandLine().c_str());
    fflush(stdout);
}

//shell自己执行命令 不创建进程
bool CheckAndExecBuiltCommand(){
    //内建命令
    if(strcmp(gargv[0],"cd")==0){
        if(gargc==2){
            chdir(gargv[1]);
            lastcode=0;
        }
        else{
            lastcode=1;
        }
        return true;
    }
    else if(strcmp(gargv[0],"export")==0){
        if(gargc==2){
            AddEnv(gargv[1]);
            lastcode=0;
        }
        else{
            lastcode=2;
        }
        return true;
    }
    else if(strcmp(gargv[0],"env")==0){
        for(int i=0;genv[i];i++){
            printf("%s\n",genv[i]);
        }
        lastcode=0;
        return true;
    }
    else if(strcmp("echo",gargv[0])==0)
    {
        if(gargc == 2) 
        {
            if(gargv[1][0] == '$') 
            { 
                if(gargv[1][1] == '?') 
                { 
                    printf("%d\n", lastcode); 
                    lastcode = 0; 
                }
            }
            else 
            { 
                printf("%s\n", gargv[1]); 
                lastcode = 0; 
            } 
        } 
        else
        { 
            lastcode = 3; 
        } 
        return true; 
    } 
    return false;
}



int main()
{
    InitEnv();
    char command_buffer[basesize];
    while(true)
    {
        PrintCommandLine();//命令提示符
                         //command_buffer->output
        if(!GetCommandLine(command_buffer,basesize))//获取用户命令
        {
            continue;
        }
        ParseCommandLine(command_buffer,strlen(command_buffer));//分析命令

        if(CheckAndExecBuiltCommand())
        {
            continue;
        }

        ExecuteCommand();//执行命令

    }
    return 0;
}
