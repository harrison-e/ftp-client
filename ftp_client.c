// makes getaddrinfo work for some reason
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "ftp_client.h"
#include "harry_util.h"

// main function
int main(int argc, char** argv) {
  operation_t* op;
  if (NULL == (op = parse_args(argc, argv))) {
    return -1;
  }

  int op_exec_status = 0;
  if (0 != (op_exec_status = operation_execute(op))) {
    return op_exec_status;
  }

  return 0;
}

operation_t* parse_args(int argc, char** argv) {
  if (argc < 3 || argc > 4) {
    HU_eprintf("invaid arguments\nsee \"Usage\" in README.md");
    return NULL;
  }
  
  operation_t* op = (operation_t *) malloc(sizeof(operation_t));
  op->type = OP_COUNT;  // dummy value
  op->param1 = NULL;
  op->param2 = NULL;

  // argv[0] is program name, irrelevant
  // argv[1] is [operation]
  for (int i = 0; i < OP_COUNT; ++i) {
    if (0 == strcmp(argv[1], OPERATIONS[i])) {
      op->type = i; // properly indexes into enum
    }
  }

  if (op->type == OP_COUNT) {
    HU_eprintf("invalid operation\nsee \"Usage\" in README.md");
    free(op);
    return NULL;
  }

  // parse params 
  op->param1 = parse_param(argv[2]);
  if (NULL == op->param1->path) {
    free(op->param1);
    free(op);
    return NULL;
  }
  if (argc > 3) {
    op->param2 = parse_param(argv[3]);
    if (NULL == op->param2->path) {
      free(op->param1);
      free(op->param2);
      free(op);
      return NULL;
    }
  }

  return op;
}

param_t* parse_param(char* param_str) {
  if (param_str == NULL) {
    return NULL;
  }
  
  param_t* param = (param_t*) malloc(sizeof(param_t));
  memset(param, 0, sizeof(param_t));

  if (0 == (strncmp(param_str, "ftp://", 6))) {
    param->ftp = true;
    strcpy(param->port, "21");
    strcpy(param->user, "anonymous");
  } else {
    // local path case
    param->ftp = false;
    strcpy(param->path, param_str);
    return param;
  }
  
  // discard "ftp://"
  char url[788] = { 0 };
  strcpy(url, param_str + 6);

  // separate path from connection info
  char connect[656] = { 0 };
  char* end_of_connect = strchr(url, '/');
  // handle bad input
  if (end_of_connect == NULL) {
    HU_eprintf("path required in FTP url");
    free(param);
    return NULL;
  }

  strcpy(param->path, end_of_connect);
  size_t connect_len = strlen(url) - strlen(end_of_connect);
  strncpy(connect, url, connect_len);

  // separate login info from host info 
  char* host_and_port = strchr(connect, '@');
  if (host_and_port != NULL) {
    // in this case, there is at least a username
    size_t login_len = strlen(connect) - strlen(host_and_port);
    char login[513] = { 0 }; // login contains user (and pass)
    strncpy(login, connect, login_len);

    // parse user (and pass)
    char* parsed_pass = strchr(login, ':');
    if (parsed_pass != NULL) {
      size_t user_len = strlen(login) - strlen(parsed_pass);
      strncpy(param->user, login, user_len);
      param->user[user_len] = '\0';  // make sure anonymous doesnt trail into short user 
      strcpy(param->pass, parsed_pass + 1);  // discard ':'
    } else {
      strcpy(param->user, login);
      param->user[login_len] = '\0';
    }
    
    // parse host (and port)
    // first make sure that there is a host 
    if (strlen(host_and_port) == 0) {
      HU_eprintf("host required in FTP url");
      free(param);
      return NULL;
    }

    // increment to discard '@'
    char* parsed_port = strchr(++host_and_port, ':');
    // if port is provided
    if (parsed_port != NULL) {
      size_t host_len = strlen(host_and_port) - strlen(parsed_port);
      strncpy(param->host, host_and_port, host_len);
      strcpy(param->port, parsed_port + 1);
    } else {
      // no port, whole thing is host 
      strcpy(param->host, host_and_port);
    }
  }

  // if `host_and_port` is NULL, `connect` is the host and port 
  else {
    char* parsed_port = strchr(connect, ':');
    // if port is provided 
    if (parsed_port != NULL) {
      size_t host_len = strlen(connect) - strlen(parsed_port);
      strncpy(param->host, connect, host_len);
      strcpy(param->port, parsed_port + 1);
    } else {
      // no port, only host, so check that host exists
      if (strlen(connect) == 0) {
        HU_eprintf("host required in FTP url");
        free(param);
        return NULL;
      }
      // bare minimum case 
      strcpy(param->host, connect);
    }
  }

  return param;
}


int operation_execute(operation_t* op) {
  if (op == NULL) {
    HU_eprintf("could not execute operation");
    return -1;
  }

  // always open a control socket
  int ctrl_fd;
  param_t* ftp_param;
  param_t* local_param;
  if (op->param1->ftp == true) {
    ctrl_fd = socket_open(op->param1->host, op->param1->port);
    ftp_param = op->param1;
    if (op->param2->path != NULL) {
      local_param = op->param2;
    }
  } else if (op->param2->ftp == true) {
    ctrl_fd = socket_open(op->param2->host, op->param2->port);
    ftp_param = op->param2;
    if (op->param1->path != NULL) {
      local_param = op->param1;
    }
  } else {
    HU_eprintf("could not open control socket");
    return -1;
  }
  if (ctrl_fd == -1) {
    HU_eprintf("could not open control socket");
    return -1;
  }
  int hello = 0;
  char hello_buf[256];
  hello = recv(ctrl_fd, hello_buf, 255, 0);
  hello_buf[hello] = '\0';
  printf("%s", hello_buf);
  sleep(2);

  int request_result = 0;
  send_request(ctrl_fd, USER, ftp_param->user);
  sleep(2);
  if (ftp_param->pass[0] != '\0') {
    send_request(ctrl_fd, PASS, ftp_param->pass);
    sleep(2);
  }
  send_request(ctrl_fd, TYPE, "I");
  send_request(ctrl_fd, MODE, "S");
  send_request(ctrl_fd, STRU, "F");
  sleep(2); // sleep so pasv doesnt read wrong response
  
  // stuff for data socket
  int data_fd;
  char buf[256] = { 0 };
  size_t count;
  int result;

  switch (op->type) {
    case op_ls:
      if (-1 == (data_fd = send_request(ctrl_fd, PASV, NULL))) {
        HU_eprintf("could not open data socket");
        break;
      }
      sleep(1);
      send_request(ctrl_fd, LIST, ftp_param->path); 
      while (0 < (count = recv(data_fd, buf, 255, 0))) {
        buf[count] = '\0';
        printf("%s", buf);
      }
      printf("\n");
      socket_close(data_fd);
      break;
    case op_mkdir:
      send_request(ctrl_fd, MKD, ftp_param->path);
      break;
    case op_rm:
      send_request(ctrl_fd, DELE, ftp_param->path);
      break;
    case op_rmdir:
      send_request(ctrl_fd, RMD, ftp_param->path);
      break;
    case op_cp:
      if (-1 == (data_fd = send_request(ctrl_fd, PASV, NULL))) {
        HU_eprintf("could not open data socket");
        break;
      }
      sleep(1);
      if (op->param1->ftp) {
        send_request(ctrl_fd, RETR, op->param1->path);
        if (0 == (result = recv_to_file(data_fd, op->param2->path))) {
          HU_dprintf("successful read");
        } 
      } else {
        send_request(ctrl_fd, STOR, op->param2->path);
        if (0 == (result = send_from_file(data_fd, op->param1->path))) {
          HU_dprintf("successful write");
        }
      }
      close(data_fd);
      break;
    case op_mv:
      if (-1 == (data_fd = send_request(ctrl_fd, PASV, NULL))) {
        HU_eprintf("could not open data socket");
        break;
      }
      sleep(1);
      if (op->param1->ftp) {
        send_request(ctrl_fd, RETR, op->param1->path);
        if (0 == (result = recv_to_file(data_fd, op->param2->path))) {
          HU_dprintf("successful read");
          send_request(ctrl_fd, DELE, op->param1->path);
        } 
      } else {
        send_request(ctrl_fd, STOR, op->param2->path);
        if (0 == (result = send_from_file(data_fd, op->param1->path))) {
          HU_dprintf("successful write");
          remove(op->param1->path);
        }
      }
      close(data_fd);
      break;
    default:
      HU_eprintf("operation was not set correctly\nthis is not user's fault");
      break;
  }

  // send QUIT msg
  send_request(ctrl_fd, QUIT, NULL);
  socket_close(ctrl_fd);
  
  // free structs
  if (NULL != op->param1->path) {
    free(op->param1);
  }
  if (NULL != op->param2->path) {
    free(op->param2);
  }
  free(op);

  return 0;
}

int socket_open(char* address, char* port) {
  // get address info
  struct addrinfo details = { 0 };
  details.ai_family = AF_UNSPEC;
  details.ai_socktype = SOCK_STREAM;
  struct addrinfo* server_info;
  int retval = 0;
  if (0 != (retval = getaddrinfo(address, port, &details, &server_info))) {
    HU_eprintf("getaddrinfo");
    return -1;
  }

  // creating socket
  int sock_fd = socket(server_info->ai_family, 
    server_info->ai_socktype,
    server_info->ai_protocol);
  
  // attempt to connect, and return
  int connection_result = connect(sock_fd, 
    server_info->ai_addr, 
    server_info->ai_addrlen);
  if (connection_result == 0) {
    HU_dprintf("Successful connection!");
    freeaddrinfo(server_info);
    return sock_fd;
  } else {
    HU_eprintf("Could not connect");
    freeaddrinfo(server_info);
    return -1;
  }
}

void socket_close(int ctrl_fd) {
  close(ctrl_fd);
}

int send_request(int ctrl_fd, request_t type, char* param) {
  HU_dprintf("sending request");

  // buffers
  char request[1096]  = { 0 };
  char response[8096] = { 0 };
  size_t response_size = 8096;
  const char space[2] = " ";
  char prefix[5] = { 0 };
  const char end_of_request[3] = "\r\n";
  
  // handle nonexistent request
  if (type == REQUEST_COUNT) {
    HU_eprintf("error processing FTP request");
    return -1;
  }
  
  // add the appropriate request string
  strcpy(prefix, REQUESTS[type]);
  strcat(request, prefix);
  strcat(request, space);

  switch (type) {
    case USER:
    case PASS:
    case LIST:
    case DELE:
    case MKD:
    case RMD:
    case STOR:
    case RETR:
    case TYPE:
    case MODE:
    case STRU:
      strcat(request, param);
      break;
  }
  strcat(request, end_of_request);
  int send_len = send(ctrl_fd, request, strlen(request), 0); 
  int recv_len = recv(ctrl_fd, response, response_size - 1, 0);
  response[recv_len] = '\0';
  printf("%s", response);
  
  // if PASV, make sure to parse response for data socket ip and port 
  if (type == PASV) {
    ip_and_port_t* data_socket_info;
    if (NULL != (data_socket_info = parse_pasv_msg(response))) {
      int data_fd = socket_open(data_socket_info->ip, data_socket_info->port);
      free(data_socket_info);
      return data_fd;
    }
    return -1;
  }

  return 0;
}

ip_and_port_t* parse_pasv_msg(char* server_response) {
  if (0 != strncmp(server_response, "2", 1)) {
    //HU_eprintf(server_response);
    HU_eprintf("could not parse IP and port from server");
    return NULL;
  }

  // setup struct
  ip_and_port_t* ip_and_port = (ip_and_port_t*) malloc(sizeof(ip_and_port_t));
  memset(ip_and_port, 0, sizeof(ip_and_port_t));

  // begin parsing string
  char* connection_info = strchr(server_response, '(');
  char ip_and_port_str[72] = { 0 };
  strncpy(ip_and_port_str, connection_info + 1, strlen(connection_info) - 5);
  
  const char dot[2]   = ".";
  char token_begin[72];
  strcpy(token_begin, ip_and_port_str);
  char* token_end;

  // tokenize string and parse ip
  token_end = strchr(token_begin, ',');
  for (int i = 0; i < 3; ++i) {
    strncat(ip_and_port->ip, token_begin, strlen(token_begin) - strlen(token_end));
    strcat(ip_and_port->ip, dot);
    strcpy(token_begin, token_end + 1);
    token_end = strchr(token_begin, ',');
  }
  strncat(ip_and_port->ip, token_begin, strlen(token_begin) - strlen(token_end));
  strcpy(token_begin, token_end + 1);
  token_end = strchr(token_begin, ',');

  // parse port 
  unsigned long port_number = 0;
  char port_buf[4] = { 0 };
  char* endptr = port_buf + 3;
  strncpy(port_buf, token_begin, strlen(token_begin) - strlen(token_end));
  port_number += strtoul(port_buf, &(endptr), 10) << 8;
  strcpy(token_begin, token_end + 1);
  strcpy(port_buf, token_begin);
  port_number += strtoul(port_buf, &(endptr), 10);
  sprintf(ip_and_port->port, "%lu", port_number);

  return ip_and_port;
}

int send_from_file(int data_fd, char* path_to_file) {
  HU_dprintf("sending data");
  // open file 
  int file_fd = open(path_to_file, O_RDONLY);
  if (file_fd == -1) {
    HU_eprintf("failed to open file for sending");
    return -1;
  }
  
  char send_buf[128] = { 0 };
  size_t count;

  while (0 < (count = read(file_fd, send_buf, 127))) {
    send_buf[count] = 0;
    send(data_fd, send_buf, count, 0);
  }
  
  return 0;
}

int recv_to_file(int data_fd, char* path_to_file) {
  HU_dprintf("receiving data");
  // open file
  int file_fd = open(path_to_file, O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (file_fd == -1) {
    HU_eprintf("failed to open file for receiving");
    return -1;
  }

  char recv_buf[128] = { 0 };
  size_t count;

  while (0 < (count = recv(data_fd, recv_buf, 127, 0))) {
    recv_buf[count] = '\0';
    write(file_fd, recv_buf, count);
  }

  return 0;
}


