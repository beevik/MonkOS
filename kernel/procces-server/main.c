
#define MAX_MESSAGE_QUEUE_SIZE 5

//TODO: implement pid 
struct kmessage{
    int sender_pid;
    char* message_body;
    int reciever_pid;
} ;

//the message queue used for ipc (inter proccessing conmunication)
 struct kmessage_queue {
    kmessage[MAX_MESSAGE_QUEUE_SIZE];
};


int main(){
    
}
