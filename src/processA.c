#include "./../include/processA_utilities.h"
#include <bmpfile.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <strings.h> 
#include <sys/socket.h>
#include <unistd.h> 
#include <arpa/inet.h> 

#define WIDTH 1600
#define CENTERW WIDTH/2
#define HEIGHT 600
#define CENTERH HEIGHT/2
#define DEPTH 4
#define DIMFACTOR 20
#define RADIUS 20

#define SEM_PATH "/sem_image_1"

char *shm_name="/IMAGE";
int size=WIDTH*HEIGHT*sizeof(rgb_pixel_t);
int fd_shm;
rgb_pixel_t *ptr;

bmpfile_t *bmp;   

sem_t *sem1;

char *master_a="/tmp/master_a";
int fd_ma;

FILE* logfile;
time_t curtime;

void controller(int function,int line){

    time(&curtime);
    if(function==-1){
        fprintf(stderr,"Error: %s Line: %d\n",strerror(errno),line);
        fflush(stderr);
        fprintf(logfile,"TIME: %s Error: %s Line: %d\n",ctime(&curtime),strerror(errno),line);
        fflush(logfile);

        unlink(shm_name);
        close(fd_shm);
        munmap(ptr,size);
        bmp_destroy(bmp);
        sem_close(sem1);
        sem_unlink(SEM_PATH);
        //kill(getppid(),SIGKILL);

        fclose(logfile);
        exit(EXIT_FAILURE);
    }
}

void sa_function(int sig){

    if(sig==SIGTERM || sig==SIGINT){

        // closing all resources

        unlink(shm_name);
        close(fd_shm);
        munmap(ptr,size);
        bmp_destroy(bmp);
        sem_close(sem1);
        sem_unlink(SEM_PATH);

        fclose(logfile);

        exit(EXIT_SUCCESS);

    }
}

void bmp_circle(bmpfile_t *bmp,int w,int h){

    rgb_pixel_t p={0,255,0,0};

    for(int x = -RADIUS; x <= RADIUS; x++) {
        for(int y = -RADIUS; y <= RADIUS; y++) {
        // If distance is smaller, point is within the circle
            if(sqrt(x*x + y*y) < RADIUS) {
                /*
                * Color the pixel at the specified (x,y) position
                * with the given pixel values
                */
                bmp_set_pixel(bmp, w*DIMFACTOR + x, h*DIMFACTOR + y, p);  //multiply the coordinates by 20 to represent the circle on the bitmap
            }
        } 
    }
}

void delete(bmpfile_t *bmp){

    rgb_pixel_t pixel={0,0,255,0};   //BGRA

    for(int i=0;i<HEIGHT;i++)    
        for(int j=0;j<WIDTH;j++)           
            bmp_set_pixel(bmp,j,i,pixel);   //removing all pixels except for the red pixels
}

void static_conversion(bmpfile_t *bmp,rgb_pixel_t *ptr){
    
    rgb_pixel_t* p;

    for(int i=0;i<HEIGHT;i++)   
        for(int j=0;j<WIDTH;j++){   
            p=bmp_get_pixel(bmp,j,i);

            ptr[i+HEIGHT*j].alpha=p->alpha;      //passing the data from the bitmap to the shared memory treating it as an array
            ptr[i+HEIGHT*j].blue=p->blue;
            ptr[i+HEIGHT*j].green=p->green;
            ptr[i+HEIGHT*j].red=p->red;
        }
}

int menu(){

    int choice;

    printf("MENU\n"
    "1. Normal mode\n"
    "2. Client mode\n"
    "3. Server mode\n");

    fflush(stdout);

    scanf("\n%d",&choice);

    return choice;
}

void normal_mode(){

    int first_resize = TRUE;
    init_console_ui();

    int cmd=0;

    while(cmd!='e'){

        // Get input in non-blocking mode
        cmd = getch();
                // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
                }
            else {
                reset_console_ui();
                }
            }

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    time(&curtime);
                    bmp_save(bmp,"./screenshot/imageA.bmp");
                    fprintf(logfile,"TIME: %s Screenshot of the bitmap\n",ctime(&curtime));
                    fflush(logfile);
                }
            }
        }

                // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {

            move_circle(cmd);

            draw_circle();

            delete(bmp); //clean the bitmap

            time(&curtime);
            fprintf(logfile,"TIME: %s Cleaning of the bitmap\n",ctime(&curtime));
            fflush(logfile);

            bmp_circle(bmp,circle.x,circle.y);  //drawing the circle in the image with the coordinates of the konsole circle

            controller(sem_wait(sem1),__LINE__);  //wait sem

            static_conversion(bmp,ptr);

            time(&curtime);
            fprintf(logfile,"TIME: %s Conversion for shared memory \n",ctime(&curtime));
            fflush(logfile);

            controller(sem_post(sem1),__LINE__);  //signal sem
        }
    }

    reset_console_ui();
    endwin();

}

/*int make_client(){

    int sfd,port;
    char ip[16];
    struct sockaddr_in serv_addr, cli_addr;

    // socket create and verification
    controller(sfd = socket(AF_INET, SOCK_STREAM, 0),__LINE__);

    printf("IP Address: ");
    scanf("%s",ip);
    printf("Port: ");
    scanf("%d",&port);

    bzero(&serv_addr, sizeof(serv_addr));

    // assign IP, PORT
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8002);

    // connect the client socket to server socket
    controller(connect(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),__LINE__);
    printf("socket client: %d\n",sfd);
    
    return sfd;
}*/

void client_mode(int sfd){

    int first_resize = TRUE;
    init_console_ui();

    int cmd=0;

    char send[32];

    while(cmd!='e'){

        cmd=getch();

        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
                }
            else {
                reset_console_ui();
                }
            }

        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {

            sprintf(send,"%d",cmd);

            controller(write(sfd,send,sizeof(send)),__LINE__);
        }

        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    sprintf(send,"%d",cmd);

                    controller(write(sfd,send,sizeof(send)),__LINE__);
                }
            }
        }
    }

    reset_console_ui();
    endwin();
}

/*int make_server(){

    int sfd, cfd, client_size,port;
    struct sockaddr_in serv_addr, cli_addr;

    controller(sfd = socket(AF_INET, SOCK_STREAM, 0),__LINE__);

    bzero((char *)&serv_addr, sizeof(serv_addr));

    printf("Port: ");
    scanf("%d",&port);

    // assign IP, PORT
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8002);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    // Binding newly created socket to given IP and verification
    controller(bind(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),__LINE__);

    // Now server is ready to listen and verification
    controller(listen(sfd, 5),__LINE__);
    printf("Server is listening\n");

    client_size = sizeof(cli_addr);

    // Accept the data packet from client and verification
    controller(cfd = accept(sfd, (struct sockaddr *)&cli_addr, &client_size),__LINE__);
    printf("socket server: %d\n",sfd);
    printf("accept server: %d\n",cfd);

    return sfd;

}*/

void server_mode(int cfd){

    int first_resize = TRUE;
    init_console_ui();

    int exit=0,cmd;
    char receive[32];

    while(exit!='e'){
        
        exit=getch();

        //printf("ok before\n");
        controller(read(cfd,receive,sizeof(receive)),__LINE__);
        //printf("ok after\n");

        cmd=atoi(receive);
        
        // If user resizes screen, re-draw UI...
        if(cmd == KEY_RESIZE) {
            if(first_resize) {
                first_resize = FALSE;
                }
            else {
                reset_console_ui();
                }
            }

        // Else, if user presses print button...
        else if(cmd == KEY_MOUSE) {
            if(getmouse(&event) == OK) {
                if(check_button_pressed(print_btn, &event)) {
                    time(&curtime);
                    bmp_save(bmp,"./screenshot/imageA.bmp");
                    fprintf(logfile,"TIME: %s Screenshot of the bitmap\n",ctime(&curtime));
                    fflush(logfile);
                }
            }
        }

                // If input is an arrow key, move circle accordingly...
        else if(cmd == KEY_LEFT || cmd == KEY_RIGHT || cmd == KEY_UP || cmd == KEY_DOWN) {

            move_circle(cmd);

            draw_circle();

            delete(bmp); //clean the bitmap

            time(&curtime);
            fprintf(logfile,"TIME: %s Cleaning of the bitmap\n",ctime(&curtime));
            fflush(logfile);

            bmp_circle(bmp,circle.x,circle.y);  //drawing the circle in the image with the coordinates of the konsole circle

            controller(sem_wait(sem1),__LINE__);  //wait sem

            static_conversion(bmp,ptr);

            time(&curtime);
            fprintf(logfile,"TIME: %s Conversion for shared memory \n",ctime(&curtime));
            fflush(logfile);

            controller(sem_post(sem1),__LINE__);  //signal sem
        }
    }

    reset_console_ui();
    endwin();

}

int main(int argc, char *argv[])
{   

    logfile=fopen("./logfiles/ProcessA_logfile","w");

    //pid_t a=getpid();
    int count=0;

    struct sigaction sa;
    
    memset(&sa,0,sizeof(sa));
    sa.sa_flags=SA_RESTART;
    sa.sa_handler=&sa_function;
    controller(sigaction(SIGTERM,&sa,NULL),__LINE__);
    controller(sigaction(SIGINT,&sa,NULL),__LINE__);

    int server_fd, client_fd,connection_fd, client_size,port,choice;
    struct sockaddr_in serv_addr, cli_addr;
    char ip[16];

    while(1){

        pid_t a=getpid();

        bmp = bmp_create(WIDTH, HEIGHT, DEPTH);   //creating bitmap
        if(bmp==NULL){
            exit(EXIT_FAILURE);
        }
            
        controller(fd_shm=shm_open(shm_name, O_CREAT|O_RDWR,0666),__LINE__);   //open the file descriptor of the shared memory

        controller(ftruncate(fd_shm,size),__LINE__);

        ptr=(rgb_pixel_t*)mmap(0,size,PROT_READ|PROT_WRITE,MAP_SHARED,fd_shm,0);
        if(ptr==MAP_FAILED){
            fprintf(stderr,"Error %s Line: %d\n",strerror(errno),__LINE__);
            exit(EXIT_FAILURE);
        }

        sem1=sem_open(SEM_PATH,O_CREAT,S_IRUSR | S_IWUSR,1);   // open the semaphore
        if(sem1==SEM_FAILED){
            fprintf(stderr,"Error: %s Line: %d\n",strerror(errno),__LINE__);
            fflush(stderr);
            unlink(shm_name);
            munmap(ptr,size);
            bmp_destroy(bmp);
            sem_unlink(SEM_PATH);
            exit(EXIT_FAILURE);
        }

        controller(sem_init(sem1,1,1),__LINE__);   //initialize the semaphore

        //opening pipe, writing and closing

        if(count==0){

            controller(fd_ma=open(master_a,O_WRONLY),__LINE__);

            controller(write(fd_ma,&a,sizeof(a)),__LINE__);

            close(fd_ma);

            count++;
        }
        //ending


        // Utility variable to avoid trigger resize event on launch
        //int first_resize = TRUE;
        
        // Initialize UI
        //init_console_ui();

        time(&curtime);
        delete(bmp); //clean the bitmap

        fprintf(logfile,"TIME: %s Cleaning of the bitmap\n",ctime(&curtime));
        fflush(logfile);

        bmp_circle(bmp,CENTERW/DIMFACTOR,CENTERH/DIMFACTOR);  //drawing a circle in the center of the image
        
        controller(sem_wait(sem1),__LINE__);  //wait sem

        static_conversion(bmp,ptr);    //passing data from the bitmap to the shared memory

        time(&curtime);
        fprintf(logfile,"TIME: %s Conversion for shared memory \n",ctime(&curtime));
        fflush(logfile);

        controller(sem_post(sem1),__LINE__);  //signal sem

        choice=menu();
        printf("choice %d\n",choice);
        switch(choice){

            case 1:
                    normal_mode();
                    break;
            case 2:
                    //client_fd=make_client();
                    //printf("main client: %d\n",client_fd);
                    

                    // socket create and verification
                    controller(client_fd=socket(AF_INET, SOCK_STREAM, 0),__LINE__);

                    printf("IP Address: ");
                    fflush(stdout);
                    scanf("%s",ip);
                    printf("Port: ");
                    fflush(stdout);
                    scanf("%d",&port);

                    bzero(&serv_addr, sizeof(serv_addr));

                    // assign IP, PORT
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_addr.s_addr = inet_addr(ip);
                    serv_addr.sin_port = htons(port); 

                    // connect the client socket to server socket
                    controller(connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),__LINE__);
                    printf("connection\n");
                    fflush(stdout);
                    printf("socket client: %d\n",client_fd);
                    client_mode(client_fd);
                    close(client_fd);
                    break;
            case 3:
                    //server_fd=make_server();
                    //printf("main server: %d\n",server_fd);

                    controller(server_fd=socket(AF_INET, SOCK_STREAM, 0),__LINE__);

                    bzero((char *)&serv_addr, sizeof(serv_addr));

                    printf("IP Address: ");
                    fflush(stdout);
                    scanf("%s",ip);

                    int address=atoi(ip);
                    printf("Port: ");
                    fflush(stdout);
                    scanf("%d",&port);

                    // assign IP, PORT
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(port);
                    serv_addr.sin_addr.s_addr = htonl(address);
                    
                    // Binding newly created socket to given IP and verification
                    controller(bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)),__LINE__);

                    // Now server is ready to listen and verification
                    controller(listen(server_fd, 5),__LINE__);
                    printf("Server is listening\n");
                    fflush(stdout);

                    client_size = sizeof(cli_addr);

                    // Accept the data packet from client and verification
                    controller(connection_fd = accept(server_fd, (struct sockaddr *)&cli_addr, &client_size),__LINE__);
                    printf("Connection accepted\n");
                    server_mode(connection_fd);
                    //close(server_fd);
                    close(server_fd);
                    break;
        }
    
       
        //endwin();

        
        unlink(shm_name);
        close(fd_shm);
        controller(munmap(ptr,size),__LINE__);
        bmp_destroy(bmp);

        controller(sem_close(sem1),__LINE__);
        controller(sem_unlink(SEM_PATH),__LINE__);

        //fclose(logfile);

    }
    
    fclose(logfile);
    return 0;
}
