// ftp_client.h 
// describes functionality and data structures for the FTP client
#include <stdbool.h>

/**
 *  Functions and data structures to parse arguments into operations
 */

// enum describing all possible operation inputs
typedef enum OperationType {
  op_ls,
  op_mkdir,
  op_rm,
  op_rmdir,
  op_cp,
  op_mv,
  OP_COUNT
} OperationType;

// string list of operations
const char* const OPERATIONS[] = {
  "ls",
  "mkdir",
  "rm",
  "rmdir",
  "cp",
  "mv"
};

// struct to abstract a path or FTP url 
typedef struct OperationParam {
  bool ftp;         // is this param an FTP url?
  char path[128];
  char host[128];   // only valid for FTP url
  char port[16];    // ^
  char user[256];   // ^
  char pass[256];   // ^
} param_t;

// struct to abstract an operation
typedef struct Operation {
  OperationType type;
  param_t* param1;  // reference to string (from argv)
  param_t* param2;  // ^
} operation_t;


// parses arguments and returns an operation struct 
// NOTE: this function mallocs an Operation struct, make sure to free!
// returns NULL if arguments are not valid
operation_t* parse_args(int argc, char** argv);
// parses params into FTP url or local path
// NOTE: this function mallocs an OperationParam struct, make sure to free!
// returns NULL if param is not valid
param_t* parse_param(char* param_str);
// executes a given operation
// NOTE: this function frees `op` and its params 
// returns 0 on success, nonzero on failure
int operation_execute(operation_t* op);



/**
 *  Functions and data structures to deal with basic socket connectivity
 */

// open a control socket to address, at port
// returns socket fd on success, -1 on failure
int socket_open(char* address, char* port);
// close control socket 
void socket_close(int sock_fd);
// send data over socket at data_fd, from path_to_file
// return -1 on failure, 0 on success 
int send_from_file(int data_fd, char* path_to_file);
// recv data over socket at data_fd, to path_to_file 
// return -1 on failure, 0 on success
int recv_to_file(int data_fd, char* path_to_file);



/**
 *  Functions and data structures to send and interpret FTP requests and responses
 */

// all possible requests that can be sent to server
typedef enum RequestType {
  USER,
  PASS,
  TYPE,
  MODE,
  STRU,
  LIST,
  DELE,
  MKD,
  RMD,
  STOR,
  RETR,
  QUIT,
  PASV,
  REQUEST_COUNT
} request_t;

const char* REQUESTS[] = {
  "USER",
  "PASS",
  "TYPE",
  "MODE",
  "STRU",
  "LIST",
  "DELE",
  "MKD",
  "RMD",
  "STOR",
  "RETR",
  "QUIT",
  "PASV"
};

// send an ftp request to ctrl socket, of specified type with param
// returns -1 on failure, 0 on success, or data socket fd in case of PASV
int send_request(int ctrl_fd, request_t type, char* param);

// struct to wrap ip and port number
typedef struct IPandPort {
  char ip[64];
  char port[8];
} ip_and_port_t;

// parse an IP and port from a server response to a PASV request 
ip_and_port_t* parse_pasv_msg(char* server_response);

