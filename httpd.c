/* hunter's webserver
 * this is a simple webserver
 * created April 4th 2016 by hunter zhao
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdlib.h>
#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: httpd/0.1.0\r\n"

void accept_request(int);
void bad_request(int);
void cat(int ,FILE *);
void cannot_execute(int);
void error_die(const char*);
int  get_line(int ,char *,int);
void header(int,const char*);
void not_found(int);
void server_file(int ,const char*);
int startup(u_short *);
void unimplemented(int);

/*
 * * receive a call to accept
 * return void
 * parameters: the socket connected to the client
 */
void accept_request(int client){
   char buf[1024];
   int numchars;
   char method[255];
   char url[255];
   char path[512];
   size_t i,j;
   struct stat st;
   int cgi = 0;  
   char *query_string=NULL;

 //read from the socket return the len of content
   numchars = get_line(client,buf,sizeof(buf));
   i=0;j=0;
   //get method
   while(!ISspace(buf[j]) && (i <sizeof(method)-1))
   {
      method[i] =buf[j];
      i++;j++;
   }
   method[i]='\0';
   
   //neither post nor get return unimplementd
   if(strcasecmp(method,"GET") && strcasecmp(method,"POST"))
   {
     unimplemented(client);
     return;
   }

   if(strcasecmp(method,"POST") == 0)
   {
     cgi=1;
   }
   
   i=0;
   //skip the space
   while(ISspace(buf[j]) && j<sizeof(buf))
      j++;

   // get url
   while(!ISspace(buf[j]) && i<sizeof(url)-1 && j<sizeof(buf))
   { 
     url[i]= buf[j];
     i++;j++;
   }
   url[i]='\0';
   
   if(strcasecmp(method,"GET")==0)
   { 
      query_string = url;
      while((*query_string !='?') &&( *query_string!='\0' ))
         query_string++;
      
      //the query param follow the ?
      if(*query_string =='?')
      {
        cgi=1;
        *query_string='\0';
         query_string++;
      }
   }
   
   // if method ==get no query string in url now
   sprintf(path,"htdocs%s",url);
   if(path[strlen(path)-1]=='/')
     strcat(path,"index.html");
   
   //read file by filename save in buf
   if(stat(path,&st)==-1){
      //丢弃所有head文件
      while((numchars>0) && strcmp("\n",buf))
        numchars=get_line(client,buf,sizeof(buf));
      not_found(client);
   }
   //成功获取文件的属性
   else{
      //如果路径为文件夹 则默认访问文件夹中的html文件
      if((st.st_mode & S_IFMT) == S_IFDIR)
          strcat(path,"/index.html");
      //
      if((st.st_mode & S_IXUSR)||   //文件可读
         (st.st_mode & S_IXGRP)||   //文件可写
         (st.st_mode & S_IXOTH)  )  //文件其他用户可执行
      cgi =1;
      if(!cgi)
         server_file(client,path);
      else
         execute_cgi(client,path,method,query_string);
   }
   //close the connection to client   
   //close(client);
}
/*
 * infom the client that a request has made a mistake
 */
void bad_request(int client)
{
   char buf[1024];
   
   sprintf(buf,"HTTP/1.0 400 BAD REQUEST\r\n");
   send(client,buf,strlen(buf),0);
   sprintf(buf,"Content-type:text/html\r\n");
   send(client,buf,strlen(buf),0);
   sprintf(buf,"\r\n"); 
   send(client,buf,strlen(buf),0);
   sprintf(buf,"<p>your browser send a bad request, ");   
   send(client,buf,strlen(buf),0);
   sprintf(buf,"such as a POST without a content-Length.\r\n");
   send(client,buf,strlen(buf),0);
}

/*put the entire content of a file out on a socket
 * parameters: the client socket descriper
 *             FILE pointer for the file to cat
 *             FILE pointer for the file to cat
 */

void cat(int client ,FILE *resource)
{   
   char buf[1024];
   
   //read the content of file into socket
   fgets(buf,sizeof(buf),resource);
   while(!feof(resource))
   {
     send(client,buf,strlen(buf),0);
     fgets(buf,sizeof(buf),resource);
   }
   printf("send index.html\n");
}

/*
 * inform the client cgi could not run
 */

void cannot_execute(int client)
{ 
   char buf[1024];
   
   sprintf(buf,"HTTP/1.0 500 Internal Server Error\r\n");
   send(client,buf,strlen(buf),0);
   sprintf(buf,"Content-type: text/html\r\n");
   send(client,buf,strlen(buf),0);
   sprintf(buf,"\r\n");
   send(client,buf,strlen(buf),0);
   sprintf(buf,"<p>Error prohibited CGI execution</p>\r\n");
   send(client,buf,strlen(buf),0);
}

/*
 * print out the error message whith perror()
 */

void error_die(const char *sc)
{
   perror(sc);
   exit(1);
}

/*
 *execute the CGI script will need ro set environment variable as appropriate
Parameters:client socket descriptor
 path to the CGI script
 */
void execute_cgi(int client,const char *path,const char *method,const char *query_string)
{
    char buf[1024];
    int cgi_output[2];
    int cgi_input[2];
    // int cgi_pip[2];
    pid_t pid;
    int status;
    int i;
    char c;
    int numchars=1;
    int content_length=-1;
    
    buf[0]='A';buf[1]='\0';
    if(strcasecmp(method,"GET")==0)
        //将所有的http header 读取并丢弃
        while((numchars>0) && strcmp("\n",buf)){
            //memset( buf, 0, strlen( buf ) );
            numchars=get_line(client,buf,strlen(buf));
          }
    else
    {
       //post 找出content-length
       numchars=get_line(client,buf,strlen(buf));
       while((numchars >0) && strcmp("\n",buf))
       {
          //利用\0进行分割 获取15个字符长度的字符串
          buf[15]='\0';
          if(strcasecmp(buf,"Content-Length:")==0){
              content_length=atoi(&buf[16]);
          }
          numchars =get_line(client,buf,strlen(buf));
       }
       //没有找到content_length
       if(content_length==-1){
          bad_request(client);
          return;
       }
    }
    
    //correct 
    sprintf(buf,"HTTP/1.0 200 OK\r\n");//已经清空了buf的内容
    send(client,buf,strlen(buf),0);
    // sprintf(buf,"Content-type: text/html\r\n");
    // send(client,buf,strlen(buf),0);
    // sprintf(buf,"\r\n");
    // send(client,buf,strlen(buf),0);

    //传送系统的缓冲区空间不够保存需传送的数据，除非套接口处于非阻塞I/O方式，否则send()将阻塞 所以导致一直处于pending的状态

    //close(client);  关闭了连接 导致发出reset信号
    //建立管道
    if(pipe(cgi_output)<0){
       cannot_execute(client);
       return;
    }
    
    if(pipe(cgi_input)<0){
       cannot_execute(client);
       return;
    }
    
    if((pid=fork())<0){
      cannot_execute(client);
      return;
    }
   
    if(pid==0) //子进程
    {
      char meth_env[255];
      char query_env[255];
      char length_env[255];

      dup2(cgi_output[1],1);//stdout 重定向到cgi_out的写入端
      dup2(cgi_input[0],0);//stdin 重定向到cgi_input的读取端
      close(cgi_output[0]);
      close(cgi_input[1]);
      sprintf(meth_env,"REQUEST_METHOD=%s",method);
      putenv(meth_env);
      if(strcasecmp(method,"GET")==0){
         snprintf(query_env,"QUERY_STRING=%s",query_string);
         putenv(query_env);
      }
      else{ //post
         sprintf(length_env,"CONTENT_LENGTH=%d",content_length);
      } 
      if(execl(path,path,NULL)<0)
      {
         printf("execl error\n");
      }
      printf("<p>come on cgi</p>\n");
      exit(0);
    }else{ //父进程
      // close(cgi_output[1]);
      // close(cgi_input[0]);
      if(strcasecmp(method,"POST")==0)
             //接受post过来的数据
             for(i=0;i<content_length;i++){
               recv(client,&c,1,0);
               //将post 数据写入cgi_input,现在重定向到子进程的stdin
               write(cgi_input[1],&c,1);
             }
      while(read(cgi_output[0],&c,1)>0) //从子进程的标准输出中读取数据
             send(client,&c,1,0);
      close(cgi_output[0]);
      close(cgi_input[1]);
      //暂停目前的进程执行，直到有信号来到或子进程结束
      waitpid(pid,&status,0);
    }
}

void not_found(int client)
{
    char buf[1024];

    /* 404 页面 */
    sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
    send(client, buf, strlen(buf), 0);
    /*服务器信息*/
    sprintf(buf, SERVER_STRING);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<HTML><TITLE>Not Found</TITLE>\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "<BODY><P>The server could not fulfill\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "your request because the resource specified\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "is unavailable or nonexistent.\r\n");
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "</BODY></HTML>\r\n");
    send(client, buf, strlen(buf), 0);
}

int get_line(int sock, char *buf, int size)
{
 int i = 0;
 char c = '\0';
 int n;

 while ((i < size - 1) && (c != '\n'))
 {
  n = recv(sock, &c, 1, 0);
  /* DEBUG printf("%02X\n", c); */
  if (n > 0)
  {
   if (c == '\r')
   {
    n = recv(sock, &c, 1, MSG_PEEK);
    /* DEBUG printf("%02X\n", c); */
    if ((n > 0) && (c == '\n'))
     recv(sock, &c, 1, 0);
    else
     c = '\n';
   }
   buf[i] = c;
   i++;
  }
  else
   c = '\n';
 }
 buf[i] = '\0';
 
 return(i);
}

void header(int client,const char *filename)
{
   char buf[1024];
   (void)filename;

   strcpy(buf,"HTTP/1.0 200 OK\r\n");
   send(client,buf,strlen(buf),0);
   //服务器信息
   strcpy(buf,SERVER_STRING);
   send(client,buf,strlen(buf),0);
   strcpy(buf,"Content-Type: text/html\r\n");
   send(client,buf,strlen(buf),0);
   strcpy(buf,"\r\n");
   send(client,buf,strlen(buf),0);
   printf("send head\n");
}

void server_file(int client,const char *filename)
{
   FILE *resource =NULL;
   int numchars= 1;
   char buf[1024];
   
   //丢弃header
   buf[0]='A'; buf[1]='\0';
   while((numchars >0) && strcmp("\n", buf) )
       numchars=get_line(client,buf,sizeof(buf));
   //打开server的文件
   resource = fopen(filename,"r");
   if(resource ==NULL)
      not_found(client);
   else
   {
      header(client,filename);
      cat(client,resource);
      
   }
   fclose(resource);
}

int startup(u_short *port)
{
  int httpd = 0;
  struct sockaddr_in name;

  httpd=socket(PF_INET,SOCK_STREAM,0);
  if(httpd== -1)
      error_die("socket");
  memset(&name,0,sizeof(name));
  name.sin_family =AF_INET;
  name.sin_port = htons(*port);
  name.sin_addr.s_addr=htonl(INADDR_ANY);
  if(bind(httpd,(struct sockaddr *)&name,sizeof(name))<0)
      error_die("bind");
  
  if(*port ==0 )
  {
     int namelen=sizeof(name);
     if(getsockname(httpd,(struct sockaddr*)& name , &namelen)==-1)
        error_die("getsockname");
     *port=ntohs(name.sin_port);
  }
  
  if(listen(httpd,5)<0)
     error_die("listen");
  return(httpd);
}

void unimplemented(int client)
{
  char buf[1024];
  sprintf(buf,"HTTP/1.0 501 Method Not Implemented\r\n");
  send(client,buf,strlen(buf),0);
  
  sprintf(buf,SERVER_STRING);
  send(client,buf,strlen(buf),0);
  sprintf(buf,"Content-Type: Text/html\r\n");
  send(client,buf,strlen(buf),0);
  sprintf(buf,"\r\n");
  send(client,buf,strlen(buf),0);
  sprintf(buf,"<HTML><HEAD><TITLE>Method Not Implemented\r\n");
  send(client,buf,strlen(buf),0);
  sprintf(buf,"</TITLE></HEAD>\r\n");
  send(client,buf,strlen(buf),0);
  sprintf(buf,"<BODY> <p>HTTP request method not  supported.\r\n");
  send(client,buf,strlen(buf),0);
  sprintf(buf,"</BODY></HTML>\r\n");
  send(client,buf,strlen(buf),0);
}

int main(void)
{
  int server_sock =-1;
  u_short port=0;
  int client_sock=-1;
  struct sockaddr_in client_name;
  int client_name_len=sizeof(client_name);
  pthread_t newthread;
  
  server_sock =startup(&port);
  printf("http running on port %d\n",port);

  while(1)
  {
     client_sock =accept(server_sock,(struct sockaddr *)&client_name,&client_name_len);
     if(client_sock ==-1)
         error_die("accept");
     // accept_request(client_sock);
     if(pthread_create(&newthread,NULL,accept_request,client_sock)!=0)
         perror("pthread_create");
     //close(client_sock);
  }
  
  close(server_sock);
  return(0);
}
