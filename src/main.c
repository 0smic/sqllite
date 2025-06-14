#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define size_of_attribute(Struct , Attribute) sizeof(((Struct*)0)->Attribute)



/*  ENUM */

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
}MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_NEGATIVE_ID,
    PREPARE_STRING_TOO_LONG,
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECONGNIZED_STATEMENT
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;


typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL
} ExecuteResult;

/* STRUCT */


typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;


typedef struct {
	char *buffer;
	size_t buffer_length;
	ssize_t input_length;
} InputBuffer;


typedef struct{
    StatementType type;
    Row row_to_insert;  // only be used by insert statement
}Statement;



/*  CONSTANTS  */
const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);

const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

const uint32_t PAGE_SIZE = 4096;
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;



/*   STRUCT  */

typedef struct {
    uint32_t num_rows;
    void *pages[TABLE_MAX_PAGES];
} Table;



/* PROTOTYPE */


ssize_t getline(char **line_ptr, size_t *n, FILE *STREAM);
MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table);
PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement);
ExecuteResult execute_statement(Statement *statement, Table *table);
InputBuffer *new_input_buffer();
void read_input(InputBuffer *input_buffer);
void close_input_buffer(InputBuffer *input_buffer);

void serialize_row(Row *source, void *destination);
void deserialize_row(void *source, Row *destination);

void *row_slot(Table *table ,uint32_t row_num);
Table *new_table();

ExecuteResult execute_insert(Statement *statement, Table *table);
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


        
/* SERIALIZIND AND DESERIALIZING FUNC   */

void serialize_row(Row *source, void *destination){
    memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET , &(source->username), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);

}

void deserialize_row(void *source, Row *destination){
    memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), source + USERNAME_OFFSET , USERNAME_SIZE);
    memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}


void *row_slot(Table *table ,uint32_t row_num){
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void *page = table->pages[page_num];
    
    if(page == NULL){
        // Allocate memory only if when we use that page
        page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;

    return byte_offset + page;
}



Table *new_table(){
    Table *table = (Table *)malloc(sizeof(Table));
    table->num_rows = 0;
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        table->pages[i] = NULL;
    }
    return table;
}


/*  FREEING FUNCTIONS   */

void close_input_buffer(InputBuffer *input_buffer){
	free(input_buffer->buffer);
	free(input_buffer);
}

void free_table(Table *table){
    for(int i = 0; table->pages[i]; i++){
        free(table->pages[i]);
    }
    free(table);
}


/*  PRINTING FUNCTIONS */

void print_row(Row *row){
    printf("(%d , %s , %s)\n", row->id , row->username, row->email);
}

void print_prompt() { printf("db > "); }

/*   META COMMAND FUNC  */


MetaCommandResult do_meta_command(InputBuffer *input_buffer, Table *table){
    if(strcmp(input_buffer->buffer, ".exit") == 0){
        close_input_buffer(input_buffer);
        free_table(table);
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}


/*  STATEMENT  FUNCTIONS */


PrepareResult prepare_insert(InputBuffer *input_buffer ,Statement *statement){
    statement->type = STATEMENT_INSERT;
    char *keyword = strtok(input_buffer->buffer, " ");
    char *id_string = strtok(NULL, " ");
    char *username = strtok(NULL, " ");
    char *email = strtok(NULL, " ");

    if (id_string == NULL || username == NULL || email == NULL ){
        return PREPARE_SYNTAX_ERROR;
    }

    int id = atoi(id_string);
    if(id < 0) return PREPARE_NEGATIVE_ID;

    if(strlen(username) > COLUMN_USERNAME_SIZE) return PREPARE_STRING_TOO_LONG;
    if(strlen(email) > COLUMN_EMAIL_SIZE) return PREPARE_STRING_TOO_LONG;

    statement->row_to_insert.id = atoi(id_string);
    strcpy(statement->row_to_insert.username, username);
    strcpy(statement->row_to_insert.email ,email);
    return PREPARE_SUCCESS;
}

PrepareResult prepare_statement(InputBuffer *input_buffer, Statement *statement){
    if(strncmp(input_buffer->buffer, "insert", 6) == 0){
        return prepare_insert(input_buffer, statement);
    }
    if(strncmp(input_buffer->buffer, "select", 6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }
    return PREPARE_UNRECONGNIZED_STATEMENT;
}


/*  EXECUTE FUNCTIONS  */

ExecuteResult execute_insert(Statement *statement, Table *table){
    if(table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }

    Row *row_to_insert = &(statement->row_to_insert);

    serialize_row(row_to_insert, row_slot(table, table->num_rows));
    table->num_rows++;
    return EXECUTE_SUCCESS;
}


ExecuteResult execute_select(Statement *statement, Table *table){
    Row row;
    for(uint32_t i = 0; i<table->num_rows; i++){
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
}

ExecuteResult execute_statement(Statement *statement, Table *table){
    switch(statement->type){
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            return execute_select(statement, table);
    }
}

    
InputBuffer *new_input_buffer(){
	InputBuffer *input_buffer = (InputBuffer *)malloc(sizeof(InputBuffer));
	input_buffer->buffer = NULL;
	input_buffer->buffer_length = 0;
	input_buffer->input_length = 0;
	return input_buffer;
}


void read_input(InputBuffer *input_buffer){
	ssize_t byte_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);
	
	if(byte_read <= 0){
		printf("Error read line ");
		exit(EXIT_FAILURE);
	}
	
	input_buffer->input_length = byte_read - 1;
	input_buffer->buffer[byte_read - 1] = '\0';
}


int main(int argc , char *argv[]){
    Table *table = new_table();
    	InputBuffer *input_buffer = new_input_buffer();
	while(true){
		print_prompt();
		read_input(input_buffer);

        if(input_buffer->buffer[0] == '.'){
            switch(do_meta_command(input_buffer, table)) {
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
                case (PREPARE_SYNTAX_ERROR):
                    printf("Syntax Error: Could not parser the statement\n");
                    continue;
                case (PREPARE_NEGATIVE_ID):
                    printf("ID must be positive\n");
                    continue;
                case (PREPARE_STRING_TOO_LONG):
                    printf("String is too long\n");
                    continue;
                case (PREPARE_UNRECONGNIZED_STATEMENT):
                    printf("Unrecongnized command '%s'\n", input_buffer->buffer);
                    continue;
                 }
    switch(execute_statement(&statement, table)){
        case (EXECUTE_SUCCESS):
            printf("Executed successfully\n");
            break;
        case (EXECUTE_TABLE_FULL):
            printf("Error: Table full\n");
            break;
    }
    
	}

	return 0;
}
