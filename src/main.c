#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


/*  ENUM */

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
}MetaCommandResult;

typedef enum {PREPARE_SUCCESS, PREPARE_UNRECONGNIZED_STATEMENT} PrepareResult;

typedef enum {STATEMENT_INSERT, STATEMENT_SELECT} StatementType;


/* STRUCT */


typedef struct {
	char *buffer;
	size_t buffer_length;
	ssize_t input_length;
} InputBuffer;


typedef struct{
    StatementType type;
}Statement;


/* PROTOTYPE */


ssize_t getline(char **line_ptr, size_t *n, FILE *STREAM);
MetaCommandResult do_meta_command(InputBuffer *input_buffer);
PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement);
void execute_statement(Statement *statement);
InputBuffer *new_input_buffer();
void read_input(InputBuffer *input_buffer);
void close_input_buffer(InputBuffer *input_buffer);



/*   FUNC MIMC POSIX FUNC  */

ssize_t getline(char **line_ptr, size_t *n, FILE *STREAM){
    if(line_ptr == NULL || n == 0 || STREAM == NULL) return -1;
    
    size_t pos = 0;
    int c;

    if (*line_ptr == NULL || *n == 0){
        *n = 128;
        *line_ptr = malloc(*n);
        if(*line_ptr == NULL) return -1;
    }

    while((c = fgetc(STREAM)) != EOF){
        if(pos+1 >= *n){
            *n *= 2;
            char *new_ptr = realloc(*line_ptr , *n);
            if(!new_ptr) return -1;
            *line_ptr = new_ptr;
        }

        (*line_ptr)[pos++] = (char)c;

        if(c == '\n') break;
    }

    if(pos == 0 && c == EOF) return -1;

    (*line_ptr)[pos] = '\0';
    return pos;
}
        


/*   META COMMAND FUNC  */


MetaCommandResult do_meta_command(InputBuffer *input_buffer){
    if(strcmp(input_buffer->buffer, ".exit") == 0){
        close_input_buffer(input_buffer);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}


/*  STATEMENT  FUNCTIONS */

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement){
    if(strncmp(input_buffer->buffer, "insert", 6) == 0){
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }
    if(strncmp(input_buffer->buffer, "select", 6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECONGNIZED_STATEMENT;
}


void execute_statement(Statement *statement){
    switch(statement->type){
        case (STATEMENT_INSERT):
            printf("TODO statement insert \n");
            break;
        case (STATEMENT_SELECT):
            printf("TODO Statement Select \n");
            break;
    }
}



    
InputBuffer *new_input_buffer(){
	InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;
	return input_buffer;
}

void print_prompt() { printf(" db > "); }

void read_input(InputBuffer *input_buffer){
	ssize_t byte_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
	
	if(byte_read <= 0){
		printf("Error read line ");
		exit(EXIT_FAILURE);
	}
	
	input_buffer->buffer_length = byte_read - 1;
	input_buffer->buffer[byte_read - 1] = '\0';
}

void close_input_buffer(InputBuffer *input_buffer){
	free(input_buffer->buffer);
	free(input_buffer);
}

int main(int argc , char *argv[]){
	InputBuffer *input_buffer = new_input_buffer();
	while(true){
		print_prompt();
		read_input(input_buffer);

        if(input_buffer->buffer[0] == '.'){
            switch(do_meta_command(input_buffer)) {
                case (META_COMMAND_SUCCESS):
                    continue;
                case (META_COMMAND_UNRECOGNIZED_COMMAND):
                    printf("Unrecongnized command '%s' \n", input_buffer->buffer);
                    continue;
            }
        }

        Statement statement;
        switch(prepare_statement(input_buffer, &statement)) {
                case (PREPARE_SUCCESS):
                    break;
                case (PREPARE_UNRECONGNIZED_STATEMENT):
                    printf("Unrecongnized command '%s'\n", input_buffer->buffer);
                    continue;
                 }

    execute_statement(&statement);
    printf("executed \n");
	}

	return 0;
}
