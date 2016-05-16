/************************************************************
	Project#1:	CLP & DDL
 ************************************************************/

#include "db.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fstream>
#include <sys/types.h>
#include <iostream>
#include <string>
//time stuff
#include <ctime>
#include <locale>

using namespace std;

#define MAX_ROWS = 100;


int main(int argc, char** argv)
{

	int rc = 0;
	token_list *tok_list=NULL, *tok_ptr=NULL, *tmp_tok_ptr=NULL;

	if ((argc != 2) || (strlen(argv[1]) == 0))
	{
		printf("Usage: db \"command statement\"");
		return 1;
	}

	rc = initialize_tpd_list();

  if (rc)
  {
		printf("\nError in initialize_tpd_list().\nrc = %d\n", rc);
  }
	else
	{
    rc = get_token(argv[1], &tok_list);

		/* Test code */
		tok_ptr = tok_list;
		// while (tok_ptr != NULL)
		// {
		// 	printf("%16s \t%d \t %d\n",tok_ptr->tok_string, tok_ptr->tok_class,
		// 		      tok_ptr->tok_value);
		// 	tok_ptr = tok_ptr->next;
		// }
    
		if (!rc)
		{
			rc = do_semantic(tok_list, argv[1]);
		}

		if (rc)
		{
			tok_ptr = tok_list;
			while (tok_ptr != NULL)
			{
				if ((tok_ptr->tok_class == error) ||
					  (tok_ptr->tok_value == INVALID))
				{
					printf("\nError in the string: %s\n", tok_ptr->tok_string);
					printf("rc=%d\n", rc);
					break;
				}
				tok_ptr = tok_ptr->next;
			}
		}

    /* Whether the token list is valid or not, we need to free the memory */
		tok_ptr = tok_list;
		while (tok_ptr != NULL)
		{
      tmp_tok_ptr = tok_ptr->next;
      free(tok_ptr);
      tok_ptr=tmp_tok_ptr;
		}
	}
	
	return rc;
}

long long int getTimestamp()
{
	locale::global(locale(""));
    time_t t = time(NULL);
    char mbstr[16];
    if (strftime(mbstr, sizeof(mbstr), "%Y%m%d%H%M%S", localtime(&t))) {
    }
    return atoll(mbstr);
}

/************************************************************* 
	This is a lexical analyzer for simple SQL statements
 *************************************************************/
int get_token(char* command, token_list** tok_list)
{
	int rc=0,i,j;
	char *start, *cur, temp_string[MAX_TOK_LEN];
	bool done = false;
	
	start = cur = command;
	while (!done)
	{
		bool found_keyword = false;

		/* This is the TOP Level for each token */
	  memset ((void*)temp_string, '\0', MAX_TOK_LEN);
		i = 0;

		/* Get rid of all the leading blanks */
		while (*cur == ' ')
			cur++;

		if (cur && isalpha(*cur))
		{
			// find valid identifier
			int t_class;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((isalnum(*cur)) || (*cur == '_'));

			if (!(strchr(STRING_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				   is not a blank, (, ), or a comma, then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{

				// We have an identifier with at least 1 character
				// Now check if this ident is a keyword
				for (j = 0, found_keyword = false; j < TOTAL_KEYWORDS_PLUS_TYPE_NAMES; j++)
				{
					if ((stricmp(keyword_table[j], temp_string) == 0))
					{
						found_keyword = true;
						break;
					}
				}

				if (found_keyword)
				{
				  if (KEYWORD_OFFSET+j < K_CREATE)
						t_class = type_name;
					else if (KEYWORD_OFFSET+j >= F_SUM)
            t_class = function_name;
          else
					  t_class = keyword;

					add_to_list(tok_list, temp_string, t_class, KEYWORD_OFFSET+j);
				}
				else
				{
					if (strlen(temp_string) <= MAX_IDENT_LEN)
					  add_to_list(tok_list, temp_string, identifier, IDENT);
					else
					{
						add_to_list(tok_list, temp_string, error, INVALID);
						rc = INVALID;
						done = true;
					}
				}

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if (isdigit(*cur))
		{
			// find valid number
			do 
			{
				temp_string[i++] = *cur++;
			}
			while (isdigit(*cur));

			if (!(strchr(NUMBER_BREAK, *cur)))
			{
				/* If the next char following the keyword or identifier
				   is not a blank or a ), then append this
					 character to temp_string, and flag this as an error */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
			else
			{
				add_to_list(tok_list, temp_string, constant, INT_LITERAL);

				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
				}
			}
		}
		else if ((*cur == '(') || (*cur == ')') || (*cur == ',') || (*cur == '*')
		         || (*cur == '=') || (*cur == '<') || (*cur == '>'))
		{
			/* Catch all the symbols here. Note: no look ahead here. */
			int t_value;
			switch (*cur)
			{
				case '(' : t_value = S_LEFT_PAREN; break;
				case ')' : t_value = S_RIGHT_PAREN; break;
				case ',' : t_value = S_COMMA; break;
				case '*' : t_value = S_STAR; break;
				case '=' : t_value = S_EQUAL; break;
				case '<' : t_value = S_LESS; break;
				case '>' : t_value = S_GREATER; break;
			}

			temp_string[i++] = *cur++;

			add_to_list(tok_list, temp_string, symbol, t_value);

			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
		}
    else if (*cur == '\'')
    {
      /* Find STRING_LITERRAL */
			int t_class;
      cur++;
			do 
			{
				temp_string[i++] = *cur++;
			}
			while ((*cur) && (*cur != '\''));

      temp_string[i] = '\0';

			if (!*cur)
			{
				/* If we reach the end of line */
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
      else /* must be a ' */
      {
        add_to_list(tok_list, temp_string, constant, STRING_LITERAL);
        cur++;
				if (!*cur)
				{
					add_to_list(tok_list, "", terminator, EOC);
					done = true;
        }
      }
    }
		else
		{
			if (!*cur)
			{
				add_to_list(tok_list, "", terminator, EOC);
				done = true;
			}
			else
			{
				/* not a ident, number, or valid symbol */
				temp_string[i++] = *cur++;
				add_to_list(tok_list, temp_string, error, INVALID);
				rc = INVALID;
				done = true;
			}
		}
	}
			
  return rc;
}

void add_to_list(token_list **tok_list, char *tmp, int t_class, int t_value)
{
	token_list *cur = *tok_list;
	token_list *ptr = NULL;
	ptr = (token_list*)calloc(1, sizeof(token_list));
	strcpy(ptr->tok_string, tmp);
	ptr->tok_class = t_class;
	ptr->tok_value = t_value;
	ptr->next = NULL;

  if (cur == NULL)
		*tok_list = ptr;
	else
	{
		while (cur->next != NULL)
			cur = cur->next;

		cur->next = ptr;
	}
	return;
}

int do_semantic(token_list *tok_list, char *command)
{
	// g_tpd_list->db_flags = 0/1;
	int rc = 0, cur_cmd = INVALID_STATEMENT;
	bool unique = false;	
  	token_list *cur = tok_list;
	if ((cur->tok_value == K_CREATE) &&
			((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		if (g_tpd_list->db_flags == 0)
		{
			logCommand(command);
		}
		printf("CREATE TABLE statement\n");
		cur_cmd = CREATE_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_DROP) &&
					((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		if (g_tpd_list->db_flags == 0)
		{
			logCommand(command);
		}
		printf("DROP TABLE statement\n");
		cur_cmd = DROP_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
					((cur->next != NULL) && (cur->next->tok_value == K_TABLE)))
	{
		printf("LIST TABLE statement\n");
		cur_cmd = LIST_TABLE;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_LIST) &&
					((cur->next != NULL) && (cur->next->tok_value == K_SCHEMA)))
	{
		printf("LIST SCHEMA statement\n");
		cur_cmd = LIST_SCHEMA;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_INSERT) &&
					((cur->next != NULL) && (cur->next->tok_value == K_INTO)))
	{
		if (g_tpd_list->db_flags == 0)
		{
			logCommand(command);
		}
		printf("INSERT statement\n");
		cur_cmd = INSERT;
		cur = cur->next->next;
	}
	else if ((cur->tok_value == K_SELECT) &&
					((cur->next != NULL)))
	{
		printf("SELECT statement\n");
		cur_cmd = SELECT;
		cur = cur->next;
	}
	else if ((cur->tok_value == K_DELETE) &&
					((cur->next != NULL) && (cur->next->tok_value == K_FROM)))
	{
		if (g_tpd_list->db_flags == 0)
		{
			logCommand(command);
		}
		printf("DELETE statement\n");
		cur_cmd = DELETE;
		cur = cur->next;
	}
	else if ((cur->tok_value == K_UPDATE) &&
					(cur->next != NULL))
	{
		if (g_tpd_list->db_flags == 0)
		{
			logCommand(command);
		}
		printf("UPDATE TABLE statement\n");
		cur_cmd = UPDATE;
		cur = cur->next;
	}
	else if ((cur->tok_value == K_BACKUP) &&
					(cur->next->tok_value == K_TO)
					&& 	(cur->next->next != NULL))
	{
		if (g_tpd_list->db_flags == 0)
		{
			// logCommand();
		}
		printf("BACKUP statement\n");
		cur_cmd = BACKUP;
		//To <filename>
		cur = cur->next->next;
	}
	else
  	{
		printf("Invalid statement\n");
		rc = cur_cmd;
	}

	if (cur_cmd != INVALID_STATEMENT)
	{
		switch(cur_cmd)
		{
			case CREATE_TABLE:
						rc = sem_create_table(cur);
						break;
			case DROP_TABLE:
						rc = sem_drop_table(cur);
						break;
			case LIST_TABLE:
						rc = sem_list_tables();
						break;
			case LIST_SCHEMA:
						rc = sem_list_schema(cur);
						break;
			case INSERT:
						rc = sem_insert_data(cur);
						break;
			case SELECT:
						rc = sem_select_data(cur);
						break;
			case DELETE:
						rc = sem_delete_data(cur);
						break;
			case UPDATE:
						rc = sem_update_data(cur);
						break;
			case BACKUP:
						rc = sem_backup_data(cur);
						break;
			default:
					; /* no action */
		}
	}
	
	return rc;
}

int fileExists(char *filename)
{
	FILE *fhandle = NULL;

	if((fhandle = fopen(filename, "r")) != NULL)
	{
		fflush(fhandle);
		fclose(fhandle);
		return 1;
	}
	else return 0;
}

int backup_data(char *filename) {
	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	char extension[5] = ".tab";
	int i;
	char *table_file;
	table_file_header *tbh;
	FILE *fhandle = NULL;

	if(fileExists(filename))
	{
		return FILE_ALREADY_EXISTS;
	}

	//---
	char *file_ptr;

	//Copy All of file to a pointer: http://stackoverflow.com/questions/29915545/reading-from-text-file-to-char-array
	ifstream fin("dbfile.bin");
	// get pointer to associated buffer object
	filebuf* pbuf = fin.rdbuf();
	 // get file size using buffer's members
	size_t size = pbuf->pubseekoff (0,fin.end,fin.in);
	pbuf->pubseekpos (0,fin.in);
	  // allocate memory to contain file data
	char* buffer= (char*) calloc(1, size);
	  // get file data
	pbuf->sgetn (buffer,size);
	fin.close();


 	if((fhandle = fopen(filename, "a")) == NULL)
	{
		return FILE_OPEN_ERROR;
	}
	else
	{
		//write original dbfile.bin to the backup file
		fwrite(buffer, size, 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
		return 0;
		for (i = 0; i < num_tables; i++)
		{
			//create table file name with extensions
			table_file = (char*)calloc(1, sizeof(cur_tpd->table_name) + sizeof(extension));
			strcpy(table_file,cur_tpd->table_name);
			strcat(table_file,extension);
			//get table file header aka full file
			tbh = getDataTable(table_file);
			//write the length
			fwrite(&(tbh->file_size), sizeof(int), 1, fhandle);
			//write the wholefile
			fwrite(tbh, tbh->file_size, 1, fhandle);
			free(table_file);
			cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
		}
	}
}	

int sem_backup_data(token_list *t_list)
{
	//Starts at the <File Name>
	int rc;
	token_list *cur = t_list;
	//Check if file name is valid
	if (cur->tok_class == keyword || (cur->tok_class == identifier) || (cur->tok_class == type_name))
	{
		if ((cur->next != NULL) && (cur->next->tok_value == EOC))
		{
			rc = backup_data(cur->tok_string);
			if (rc)
			{
				cur->tok_value = INVALID;
			}
		}
		else 
		{
			rc = EXTRA_TEXT_AFTER_FILENAME;
			cur->tok_value = INVALID;
			return rc;
		}
		
	}
	else 
	{
		rc = INVALID_BACKUP_FILENAME;
		cur->tok_value = INVALID;
		return rc;
	}
	return rc;
}

int logCommand(char *command)
{
	FILE *fhandle = NULL;
	long long int timestamp = getTimestamp();
	printf("%lld\n", timestamp);

 	if((fhandle = fopen("db.log", "a")) == NULL)
	{
		return FILE_OPEN_ERROR;
	}
	else
	{
		//append file
		fprintf(fhandle,"%lld \"%s\"\n", timestamp, command);
		fflush(fhandle);
		fclose(fhandle);
		return 0;
	}
}

//Creates a file with size and file name * 100
int create_file(int size, char *filename, table_file_header *file_header)
{
	int rc = 0;
	char *empty = "";
	FILE *fhandle = NULL;

 	if((fhandle = fopen(filename, "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
	else
	{
		fwrite(file_header, sizeof(table_file_header), 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
	}
    return rc;
}

int sem_create_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry tab_entry;
	tpd_entry *new_entry = NULL;
	bool column_done = false;
	int cur_id = 0;
	cd_entry	col_entry[MAX_NUM_COL];
	int record_size = 0;


	memset(&tab_entry, '\0', sizeof(tpd_entry));
	cur = t_list;
	if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if ((new_entry = get_tpd_from_list(cur->tok_string)) != NULL)
		{
			rc = DUPLICATE_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			//Currently at table name
			strcpy(tab_entry.table_name, cur->tok_string);
			//Go to Left Paren
			cur = cur->next;
			if (cur->tok_value != S_LEFT_PAREN)
			{
				//Error
				rc = INVALID_TABLE_DEFINITION;
				cur->tok_value = INVALID;
			}
			else
			{
				//Nulls out first 36 bytes of col_entry
				memset(&col_entry, '\0', (MAX_NUM_COL * sizeof(cd_entry)));

				/* Now build a set of column entries */
				//Goes to Column name
				cur = cur->next;
				do
				{
					if ((cur->tok_class != keyword) &&
							(cur->tok_class != identifier) &&
							(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_COLUMN_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						int i;
						//Add 1 to record length for storing length of column
						record_size++;
						for(i = 0; i < cur_id; i++)
						{
              /* make column name case sensitive */
							if (strcmp(col_entry[i].col_name, cur->tok_string)==0)
							{
								rc = DUPLICATE_COLUMN_NAME;
								cur->tok_value = INVALID;
								break;
							}
						}

						if (!rc)
						{
							strcpy(col_entry[cur_id].col_name, cur->tok_string);
							col_entry[cur_id].col_id = cur_id;
							col_entry[cur_id].not_null = false;    /* set default */
							//Goes to column type
							cur = cur->next;
							if (cur->tok_class != type_name)
							{
								// Error
								rc = INVALID_TYPE_NAME;
								cur->tok_value = INVALID;
							}
							else
							{
                /* Set the column type here, int or char */
								col_entry[cur_id].col_type = cur->tok_value;
								cur = cur->next;
		
								if (col_entry[cur_id].col_type == T_INT)
								{
									if ((cur->tok_value != S_COMMA) &&
										  (cur->tok_value != K_NOT) &&
										  (cur->tok_value != S_RIGHT_PAREN))
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
								  else
									{
										record_size += sizeof(int);
										col_entry[cur_id].col_len = sizeof(int);
										
										if ((cur->tok_value == K_NOT) &&
											  (cur->next->tok_value != K_NULL))
										{
											rc = INVALID_COLUMN_DEFINITION;
											cur->tok_value = INVALID;
										}	
										else if ((cur->tok_value == K_NOT) &&
											    (cur->next->tok_value == K_NULL))
										{					
											col_entry[cur_id].not_null = true;
											cur = cur->next->next;
										}
	
										if (!rc)
										{
											/* I must have either a comma or right paren */
											if ((cur->tok_value != S_RIGHT_PAREN) &&
												  (cur->tok_value != S_COMMA))
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
		                  {
												if (cur->tok_value == S_RIGHT_PAREN)
												{
 													column_done = true;
												}
												cur = cur->next;
											}
										}
									}
								}   // end of S_INT processing
								else
								{
									// It must be char()
									if (cur->tok_value != S_LEFT_PAREN)
									{
										rc = INVALID_COLUMN_DEFINITION;
										cur->tok_value = INVALID;
									}
									else
									{
										/* Enter char(n) processing */
										cur = cur->next;
		
										if (cur->tok_value != INT_LITERAL)
										{
											rc = INVALID_COLUMN_LENGTH;
											cur->tok_value = INVALID;
										}
										else
										{
											/* Got a valid integer - convert */
											col_entry[cur_id].col_len = atoi(cur->tok_string);
											record_size += col_entry[cur_id].col_len;
											cur = cur->next;
											
											if (cur->tok_value != S_RIGHT_PAREN)
											{
												rc = INVALID_COLUMN_DEFINITION;
												cur->tok_value = INVALID;
											}
											else
											{
												cur = cur->next;
						
												if ((cur->tok_value != S_COMMA) &&
														(cur->tok_value != K_NOT) &&
														(cur->tok_value != S_RIGHT_PAREN))
												{
													rc = INVALID_COLUMN_DEFINITION;
													cur->tok_value = INVALID;
												}
												else
												{
													if ((cur->tok_value == K_NOT) &&
														  (cur->next->tok_value != K_NULL))
													{
														rc = INVALID_COLUMN_DEFINITION;
														cur->tok_value = INVALID;
													}
													else if ((cur->tok_value == K_NOT) &&
																	 (cur->next->tok_value == K_NULL))
													{					
														col_entry[cur_id].not_null = true;
														cur = cur->next->next;
													}
		
													if (!rc)
													{
														/* I must have either a comma or right paren */
														if ((cur->tok_value != S_RIGHT_PAREN) &&															  (cur->tok_value != S_COMMA))
														{
															rc = INVALID_COLUMN_DEFINITION;
															cur->tok_value = INVALID;
														}
														else
													  {
															if (cur->tok_value == S_RIGHT_PAREN)
															{
																column_done = true;
															}
															cur = cur->next;
														}
													}
												}
											}
										}	/* end char(n) processing */
									}
								} /* end char processing */
							}
						}  // duplicate column name
					} // invalid column name

					/* If rc=0, then get ready for the next column */
					if (!rc)
					{
						cur_id++;
					}

				} while ((rc == 0) && (!column_done));
	
				if ((column_done) && (cur->tok_value != EOC))
				{
					rc = INVALID_TABLE_DEFINITION;
					cur->tok_value = INVALID;
				}
				//Finish adding up size of .tab file
				int table_size = sizeof(table_file_header) + record_size;
				if (table_size % 4 != 0)
				{
					table_size = (4- (table_size % 4)) + table_size;
				}
				if (!rc)
				{
					/* Now finished building tpd and add it to the tpd list */
					tab_entry.num_columns = cur_id;
					tab_entry.tpd_size = sizeof(tpd_entry) + 
															 sizeof(cd_entry) *	tab_entry.num_columns;
					if (record_size % 4 != 0)
					{
						record_size = (4- (record_size % 4)) + record_size;
					}
					int table_size = sizeof(table_file_header) + record_size;
				  	tab_entry.cd_offset = sizeof(tpd_entry);
					new_entry = (tpd_entry*)calloc(1, tab_entry.tpd_size);

					if (new_entry == NULL)
					{
						rc = MEMORY_ERROR;
					}
					else
					{
						//Copy Pointer of tab entry to address of new Entry
						memcpy((void*)new_entry,
							     (void*)&tab_entry,
									 sizeof(tpd_entry));
		
						memcpy((void*)((char*)new_entry + sizeof(tpd_entry)),
									 (void*)col_entry,
									 sizeof(cd_entry) * tab_entry.num_columns);
	
						rc = add_tpd_to_list(new_entry);

						free(new_entry);
					}
				}
				//Added Code to create .tab file
				if (!rc)
				{
					FILE *fhandle = NULL;
					char *table_file = strcat(tab_entry.table_name,".tab");
					table_file_header *file_header = NULL;
					file_header = (table_file_header*)calloc(1, sizeof(table_file_header));
					if (!file_header)
					{
						rc = MEMORY_ERROR;
					}
					else
					{
						//By default only the header
						file_header->file_size = sizeof(table_file_header);
						file_header->record_size = record_size;
						file_header->num_records = 0;
						file_header->record_offset = sizeof(table_file_header);\
						file_header->file_header_flag = NULL;
						file_header->tpd_ptr = NULL;
					}
					create_file(table_size, table_file, file_header);
				}
			}
		}
	}
  return rc;
}

int sem_drop_table(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;

	cur = t_list;
	if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		if (cur->next->tok_value != EOC)
		{
			rc = INVALID_STATEMENT;
			cur->next->tok_value = INVALID;
		}
		else
		{
			if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
			{
				rc = TABLE_NOT_EXIST;
				cur->tok_value = INVALID;
			}
			else
			{
				/* Found a valid tpd, drop it from tpd list */
				rc = drop_tpd_from_list(cur->tok_string);
				//Delete the .tab file
				if (!rc)
				{
					remove(strcat(cur->tok_string,".tab"));
				}
				
			}
		}
	}

  return rc;
}

int sem_list_tables()
{
	int rc = 0;
	int num_tables = g_tpd_list->num_tables;
	tpd_entry *cur = &(g_tpd_list->tpd_start);

	if (num_tables == 0)
	{
		printf("\nThere are currently no tables defined\n");
	}
	else
	{
		printf("\nTable List\n");
		printf("*****************\n");
		while (num_tables-- > 0)
		{
			printf("%s\n", cur->table_name);
			if (num_tables > 0)
			{
				cur = (tpd_entry*)((char*)cur + cur->tpd_size);
			}
		}
		printf("****** End ******\n");
	}

  return rc;
}

table_file_header* getDataTable(char *file_name)
{

	// printf("%16s \t%d \t %d\n",tmp, t_class, t_value);
	table_file_header *tfh;
	("s%s", file_name);
	//---
	char *file_ptr;
	bool insert_done;
	int rc = 0;
	//Copy All of file to a pointer: http://stackoverflow.com/questions/29915545/reading-from-text-file-to-char-array
		std::ifstream fin(file_name);
		  // get pointer to associated buffer object
		std::filebuf* pbuf = fin.rdbuf();
		  // get file size using buffer's members
		std::size_t size = pbuf->pubseekoff (0,fin.end,fin.in);
		pbuf->pubseekpos (0,fin.in);
		  // allocate memory to contain file data
		char* buffer= (char*) calloc(1, size);
		  // get file data
		pbuf->sgetn (buffer,size);
		fin.close();
	("%s\n", buffer);

	tfh = (table_file_header*) ((char*)buffer);
 
	return tfh;
}


int insertColumns(token_list *cur, char *tabname, char* file_name)
{
	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	int column_inserted = 0;
	bool found = false;
	table_file_header *tbh;
	FILE *fhandle = NULL;
	row_string *string_row_to_add ;
	row_int *int_row_to_add;
	int bytes_added = 0;
	int i, row_length, leftover, tbh_size, temp, temp3;
	bool insert_done = false;
	bool is_int, is_null;
	row_int *int_entry;
	int *temp2;


	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, tabname) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}
	cd_entry *column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
	do
	{
		is_null = false;
		cur = cur->next;
		if ((cur->tok_value != INT_LITERAL) &&
								(cur->tok_value != STRING_LITERAL) &&
								(cur->tok_value != K_NULL))
		{
			// Error
			cur->tok_value = INVALID;
			printf("\nError in the string: %s\n",cur->tok_string);
			return INVALID_DATA;
			
		}
		//Check for conflicts
		if (column_entry->col_id >= tab_entry->num_columns)
		{		cur->tok_value = INVALID;
				printf("Column Count doesn't match schema\n");
				return COLUMN_COUNT_MISMATCH;
		}
		if (cur->tok_value != K_NULL && column_entry->col_type == T_INT && cur->tok_value != INT_LITERAL)
		{
			cur->tok_value = INVALID;
			printf("Column should be an integer\n");
			return CONFLICTING_DATA_TYPE;
		}
		if (cur->tok_value != K_NULL && column_entry->col_type == T_CHAR && cur->tok_value != STRING_LITERAL)
		{
			cur->tok_value = INVALID;
			printf("Column should be a string\n");
			return CONFLICTING_DATA_TYPE;
		}
		else 
		{
			if (cur->tok_value != INT_LITERAL && strlen(cur->tok_string) > column_entry->col_len)
			{
				cur->tok_value = INVALID;
				 printf("Parameter is too large\n");
				return TOO_LARGE_PARAMETER;
			}
			else 
			{
				if (cur->tok_value == K_NULL && column_entry->not_null == 1)
				{
					cur->tok_value = INVALID;
					printf("Cannot insert NULL\n");
					return CANNOT_INSERT_NULL;
				}
			}
			
		}
		//Create data to be added
		row_length = 0;
		if (cur->tok_value == STRING_LITERAL || cur->tok_value == K_NULL)
	    {
	    	
	    	string_row_to_add = (row_string*)calloc(1, sizeof(row_string));
	    	
	    	is_int = false;
	    	if ( cur->tok_value == K_NULL)
	    	{
	    		string_row_to_add->length = 0;
	    		for (i = 0; i < column_entry->col_len; i++)
	    		{
	    			string_row_to_add->value[i] = '\0';
	    		}
	    	}
	    	else
	    	{
	    		string_row_to_add->length = strlen(cur->tok_string);
	    		strcpy(string_row_to_add->value, cur->tok_string);
	    	}
			
			
			bytes_added++;
			bytes_added += column_entry->col_len;
			row_length += column_entry->col_len;
	    }
	    else if (cur->tok_value == INT_LITERAL)
	    {
	    	
	    	int_row_to_add = (row_int*)calloc(1, sizeof(row_int));
	    	
	    	is_int = true;
	    	int_row_to_add->length = sizeof(int);
	    	temp3 = atoi(cur->tok_string);
	    	char temp_num[4];
	    	memcpy(temp_num, &temp3, sizeof(int));
	    	for (i = 0; i < 4; i++)
	    	{
	    		int_row_to_add->value[i] = temp_num[i];
	    	}
	    	// int_row_to_add->value = temp_num;
	    	bytes_added++;
			bytes_added += column_entry->col_len;
			row_length += column_entry->col_len;
		      
	    }
	 
	    //Write to File
	    tbh = getDataTable(file_name);
		tbh_size = tbh->file_size;
		tbh->file_size += row_length +1;

	 	if((fhandle = fopen(file_name, "wbc")) == NULL)
		{
			return FILE_OPEN_ERROR;
		}
		else
		{

			//rewrite original file
			fwrite(tbh, tbh_size, 1, fhandle);

			if (is_int)
			{
				
				fwrite(int_row_to_add, sizeof(row_int), 1, fhandle);
				;
				free(int_row_to_add);
				// printf("Length of length: %d\nLength of Value: %d\n", sizeof(int_row_to_add->length), sizeof(int_row_to_add->value));
				// exit(0);
				// printf("length: %d\nvalue: %d\n", int_row_to_add->length, int_row_to_add->value);
			}
			// else if (is_null)
			// {
			// 	printf("test\n");
			// 	fwrite(tbh, tbh->file_size, 1, fhandle);
			// 	printf("test\n");
			// }
			else
			{
				fwrite(string_row_to_add, row_length+1, 1, fhandle);
				free(string_row_to_add);
			}
			// fwrite('\0', 1, column_entry->col_len - row_length, fhandle);	
			fflush(fhandle);
			fclose(fhandle);
			
		}

		cur = cur->next;
		if ((cur->tok_value != S_RIGHT_PAREN) &&
						  (cur->tok_value != S_COMMA))
		{
			cur->tok_value = INVALID;
			printf("Missing comma or Right Parenthesis\n");
			return MISSING_PUNC;
		}
		else 
		{
			if ((cur->tok_value == S_RIGHT_PAREN))
			{
				insert_done = true;
			}
		}

		if (!insert_done)
		{

			column_entry = (cd_entry*) ((char*)column_entry + sizeof(cd_entry));

		}
		else
		{
			//Checks if not enough columns
			if (column_entry->col_id != tab_entry->num_columns-1)
			{		
					cur->tok_value = INVALID;
					printf("Column Count doesn't match schema\n");
					return COLUMN_COUNT_MISMATCH;
			}
		}
	}
	while (!insert_done);


	if (bytes_added != tbh->record_size)
	{
		leftover = tbh->record_size - bytes_added;
		tbh = getDataTable(file_name);
		tbh_size = tbh->file_size;
		tbh->file_size += leftover;
		 tbh->num_records++;
		if((fhandle = fopen(file_name, "wbc")) == NULL)
		{
			cur->tok_value = INVALID;
			printf("Cannot open File\nError in the string: %s\n",cur->tok_string);
			return FILE_OPEN_ERROR;
		}
		else
		{
			//rewrite original file
			fwrite(tbh, tbh->file_size, 1, fhandle);
			printf("%s size: %d. Number of Records: %d\n", file_name, tbh->file_size, tbh->num_records);
			// fwrite('\0', 1,leftover ,fhandle);		
			fflush(fhandle);
			fclose(fhandle);
			
		}
		
	}

	
	
	return 0;
}

int sem_insert_data(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;
	bool insert_done = false;
	int column_id = -1;
	char *table_name;
	char *file_name;
	char extension[5] = ".tab";
	FILE *fhandle = NULL;
	table_file_header *tbh;
	table_file_header *original_tbh ;


	cur = t_list;
	if ((cur->tok_class != keyword) &&
		  (cur->tok_class != identifier) &&
			(cur->tok_class != type_name))
	{
		// Error./t
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else
	{
		table_name = cur->tok_string;
		file_name = (char*)calloc(1, sizeof(table_name) + sizeof(extension));
		strcpy(file_name,table_name);
		strcat(file_name,extension);
		if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
		{
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
		}
		else
		{
			cur = cur->next;
			if (cur->tok_value != K_VALUES)
			{
				//Error
				rc = MISSING_VALUES;
				printf("values keyword missing\n",cur->tok_string);
				cur->tok_value = INVALID;
			}
			else
			{
				cur = cur->next;
				if (cur->tok_value != S_LEFT_PAREN)
				{
					//Error
					rc = MISSING_LEFT_PAREN;
					cur->tok_value = INVALID;
					printf("Left Parenthesis missing\n",cur->tok_string);
				}
				else
				{
							
						tbh = getDataTable(file_name);
						original_tbh  = (table_file_header*)calloc(1, tbh->file_size);;
						memcpy((void*)original_tbh,(void*)tbh, tbh->file_size);
						rc = insertColumns(cur, table_name, file_name);
						if (!rc)
						{
							cur->tok_value = INVALID;
							
						}
						else
						{
							if((fhandle = fopen(file_name, "wbc")) == NULL)
							{
								printf("Cannot open File\nError in the string: %s\n",cur->tok_string);
								return FILE_OPEN_ERROR;
							}
							else
							{
								//rewrite original file
								fwrite(original_tbh, original_tbh->file_size, 1, fhandle);	
								fflush(fhandle);
								fclose(fhandle);
								
							}
						}
					free(file_name);
					free(original_tbh);
				}
			}
		}
	}
	return rc;
}

row_loc_type* getRecordOffSet(tpd_entry* tabent, char* column_name, table_file_header *tablebh)
{
	int i;
	table_file_header *tbh = tablebh;
	tpd_entry* tab_entry = tabent;
	//Initialize offset with tableheader offset
	int current_offset = 0;
	row_loc_type *row_location_type = (row_loc_type*)calloc(1, sizeof(row_loc_type));

	//Initializes column entry
	cd_entry *column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
	for (i = 0; i < tab_entry->num_columns; i++)
	{
		if (strcmp(column_entry->col_name, column_name) == 0)
		{
			row_location_type->offset = current_offset;
			row_location_type->type = column_entry->col_type;
			row_location_type->size = column_entry->col_len;
			row_location_type->not_null = column_entry->not_null;
			row_location_type->col_id = column_entry->col_id;
			return row_location_type;
		} 
		else 
		{
			//Add DataSize
			current_offset += column_entry->col_len;
			//Add Byte for length
			current_offset++;
		}
		//Go to next column entry
		column_entry = (cd_entry*) ((char*)column_entry + sizeof(cd_entry));
	}
	return NULL;
}

int is_affected_by_where(token_list *t_list, char *current_position, row_loc_type *row_location_type)
{
	
	token_list *cur = t_list;
	int *int_write;
	char *char_write;
	char len;
	char *start_of_row = current_position;
	int tok_count = 0;

	// printf("----start: %d\n", start_of_row[0] );
	// printf("Token String: %s\n", t_list->tok_string);
	if (row_location_type->type == T_INT)
	{
		len = start_of_row[row_location_type->offset];
		if (len > 0 )
		{
			// printf("len: %d\n", len);
			int_write = (int*)calloc(1, sizeof(int));
			memcpy((void*)int_write,
						     (void*)(start_of_row + row_location_type->offset + 1),
								sizeof(int));
		}
	}
	else
	{
		len = start_of_row[row_location_type->offset];
		if (len > 0 )
		{
		
		char_write = (char*)calloc(1, MAX_IDENT_LEN);
		memcpy((void*)char_write,
					     (void*)(start_of_row + row_location_type->offset + 1),
							(int)len);
		}
	}
	switch (cur->tok_value) 
			{
				case S_EQUAL:
					cur = cur->next;
					tok_count++;
					if (cur->tok_value == INT_LITERAL)
					{
						if (row_location_type->type == T_INT)
						{
							// printf("Token Value: %d\n",  *int_write);
							if (*int_write == atoi(cur->tok_string))
							{
								// printf("InFile: %d\nRequested%d\n", *int_write, atoi(cur->tok_string));
								return 1;
							}
							else return 0;
						}
						else 
						{
							cur->tok_value = INVALID;
							return CANNOT_COMPARE_INT_TO_STRING;
						}
						
					}
					else if (cur->tok_value == STRING_LITERAL)
					{
						if (row_location_type->type == T_CHAR)
						{

							if (strcmp(cur->tok_string, char_write) == 0)
							{
								// printf("test");
								return 1;
							}
							else return 0;
						}
						else 
						{
							cur->tok_value = INVALID;
							return CANNOT_COMPARE_STRING_TO_INT;
						}
					}
					else 
					{
						cur->tok_value = INVALID;
						return MUST_COMPARE_WITH_INT_OR_STRING_OR_NULL;
					}
					break;

				case S_LESS:
					cur = cur->next;
					tok_count++;
					if (cur->tok_value == INT_LITERAL)
					{
						if (*int_write < atoi(cur->tok_string))
						{
							return 1;
						}
						else return 0;
					}
					else 
					{
						cur->tok_value = INVALID;
						return LESS_NOT_FOLLOWED_BY_INT;
					}
					break;
					
				case S_GREATER:

					cur = cur->next;
					tok_count++;
					// printf("Greater: %d\nActual: %d\n", atoi(cur->tok_string), *int_write);
					if (cur->tok_value == INT_LITERAL)
					{
						if (*int_write > atoi(cur->tok_string))
						{
							// printf("Yes");
							return 1;
						}
						else return 0;
					}
					else 
					{
						cur->tok_value = INVALID;
						return GREATER_NOT_FOLLOWED_BY_INT;
					}
					break;

				case K_IS:
					cur = cur->next;
					tok_count++;
					if (cur->tok_value == K_NOT && cur->next != NULL)
					{
						cur=cur->next;
						tok_count++;
						if (cur->tok_value == K_NULL)
						{
							if (current_position[row_location_type->offset] != 0)
							{
								return 1;
							}
							else return 0;
						}
						else 
						{
							cur->tok_value = INVALID;
							return IS_NOT_FOLLOWED_BY_NOT_OR_NULL;
						}
					}
					else if (cur->tok_value == K_NULL)
					{
						if (current_position[row_location_type->offset] == 0)
						{
							return 1;
						}
						else return 0;
					}
					else 
						{
							cur->tok_value = INVALID;
							return IS_NOT_FOLLOWED_BY_NOT_OR_NULL;
						}
					break;

				default:
					cur->tok_value = INVALID;
					return MISSING_CONDITION_OR_OPERATOR;
					break;
			}
}


int select_aggregate(table_file_header *tbh, char *tabname, token_list *cur, int columns_2_print[], int size_array, int agg_type) {

	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int i,j, length, count = 0, k, offset;
	char *current_position;
	int *int_write;
	char *char_write;
	char len;
	char *col_name;
	row_loc_type *row_location_type;
	int within_where_clause, sum = 0;
	token_list *cur_orig = cur;
	char *agg_string;

	//Searches for the corrrect table entry
	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, tabname) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}

	//Initializes column entry
	cd_entry *column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
	//Prints Table top

	printf("+----------------+\n");
	if (agg_type == F_SUM)
	{
		agg_string = "Sum";
		printf("|%-16s|\n", agg_string);
	}
	else if (agg_type == F_AVG)
	{
		agg_string = "Average";
		printf("|%-16s|\n", agg_string);
	}
	else if (agg_type == F_COUNT)
	{
		agg_string = "Count";
		printf("|%-16s|\n", agg_string);
	}
	printf("+----------------+\n");
	//For each record
	for (i = 0; i < tbh->num_records; i++)
	{
		//Reinitializes column entry to check column type
		column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
		current_position = (char*)tbh + (i * tbh->record_size + tbh->record_offset);
		//If has no where 
		cur = cur_orig;

		// printf("%d", cur->tok_value);
		if (cur == NULL || cur->tok_value == EOC)
		{
			within_where_clause = 1;
		}
		else within_where_clause = 0;

		// printf("-----------------");
		// printf("original addy: %d\n", cur_orig);
		// printf("new addy: %d\n", cur);
		
		// printf("current token: %d\n\n", cur->tok_value);
		while (cur != NULL && cur->tok_value != EOC && cur->next != NULL)
		{
			if ((cur->tok_value == K_WHERE || cur->tok_value == K_OR) && cur->next != NULL)
			{	
				cur = cur->next;
				if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
				{
					// Error
					cur->tok_value = INVALID;
					return INVALID_COLUMN_NAME;
				}
				row_location_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
				if (row_location_type == NULL)
				{
					cur->tok_value = INVALID;
					return COLUMN_NOT_EXIST;
				}
				cur = cur->next;
				within_where_clause = within_where_clause || 
					is_affected_by_where(cur, current_position , row_location_type);
								///if not a 0 or 1 than it errored
				if (within_where_clause != 0  && within_where_clause != 1)
				{
					cur->tok_value = INVALID;
					return within_where_clause;
				}
				while (cur->tok_value != K_OR && cur->tok_value != K_AND &&  cur->tok_value != EOC)
				{
					cur = cur->next;
				}

			}
			else if (cur->tok_value == K_AND && cur->next != NULL)
			{
				cur = cur->next;
				if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
				{
					// Error
					cur->tok_value = INVALID;
					return INVALID_COLUMN_NAME;
				}
				row_location_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
				if (row_location_type == NULL)
				{
					cur->tok_value = INVALID;
					return COLUMN_NOT_EXIST;
				}
				cur = cur->next;
				within_where_clause = within_where_clause && 
					is_affected_by_where(cur, current_position , row_location_type);
								///if not a 0 or 1 than it errored
				if (within_where_clause != 0  && within_where_clause != 1)
				{
					cur->tok_value = INVALID;
					return within_where_clause;
				}
				while (cur->tok_value != K_OR && cur->tok_value != K_AND &&  cur->tok_value != EOC)
				{
					cur = cur->next;
				}

			}
			else
			{
				cur->tok_value = INVALID;
				return MISSING_WHERE_OR_AND_OR_OR;
			}
		}
		// printf("boo: %d", within_where_clause);
		if (within_where_clause)
		{

			count++;
			
			//For each column 2 print
			for (j = 0; j < size_array; j++)
			{
				offset = 0;
				column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);

				current_position = (char*)tbh + (i * tbh->record_size + tbh->record_offset);
					//-1 means it was a *star
				if (columns_2_print[j] != -1)
				{
					for (k = 0; k < columns_2_print[j]; k++)
					{
						offset += (column_entry->col_len+1);
						column_entry = (cd_entry*) ((char*)column_entry  + sizeof(cd_entry));
					}
					// printf("Offset %d", offset);
					// column_entry = (cd_entry*) ((char*)column_entry  + (sizeof(cd_entry) * columns_2_print[j]));
					current_position += (offset);

					if (column_entry->col_type == T_INT)
					{
						len = current_position[0];
						//IF NULL SKIP
						if (len == 0)
						{
							// char* nulls = "NULL";
							// printf("|%-16s" , nulls);
						}
						else
						{

							int_write = (int*)calloc(1, sizeof(int));
							
							memcpy((void*)int_write,
										     (void*)(current_position+1),
												sizeof(int));
							
							// printf("|%16d" , *int_write);
							sum += *int_write;

							free(int_write);
						}

					}
					else if (column_entry->col_type == T_CHAR)
					{	
						cur_orig->tok_value = INVALID;
						return CANNOT_AGGREGATE_STRINGS;	
					}
				}
			}
		}
	}

	if (agg_type == F_SUM)
	{
		printf("|%16d|\n" , sum);
	}
	else if (agg_type == F_AVG)
	{
		printf("|%16d|\n" , sum/count);
	}
	else if (agg_type == F_COUNT)
	{
		printf("|%16d|\n" , count);
	}
	printf("+----------------+\n");
	printf("1 row selected\n");

	return 0;
}

int sem_select_aggregate(token_list *t_list)
{
	int rc = 0, count = 0, i, size = 0;
	token_list *cur = t_list;
	char *table_name;
	char *file_name;
	char extension[5] = ".tab";
	table_file_header *tbh;
	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int has_star = 0;
	row_loc_type *row_location_type;
	int *columns_2_print = (int *) calloc(1, MAX_NUM_COL);

	int agg_type = cur->tok_value;
		//get Table name
	while (t_list->tok_value != K_FROM)
	{
		t_list=t_list->next;
		if (t_list->tok_value == EOC)
		{
			t_list->tok_value = INVALID;
			return MISSING_KEYWORD_FROM;
		}
	}
	table_name = t_list->next->tok_string;
	// int columns_2_print[];
	// to left paren
	cur = cur->next;
	if (cur->tok_value != S_LEFT_PAREN || cur->next == NULL)
	{
		cur->tok_value = INVALID;
		return MISSING_LEFT_PAREN;
	}
	cur = cur->next;
	// to column
	if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name) && (cur->tok_value != S_STAR))
	{
		// Error
		cur->tok_value = INVALID;
		rc = INVALID_COLUMN_NAME;
		return rc;
	}
		//get tbd_entry
	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, table_name) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}
	if (cur->tok_value != S_STAR)
	{

		file_name = (char*)calloc(1, sizeof(table_name) + sizeof(extension));
		strcpy(file_name,table_name);
		strcat(file_name,extension);
		tbh = getDataTable(file_name);
		//check if column exists
		row_location_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
		if (row_location_type == NULL && cur->next != NULL)
		{
			cur->tok_value = INVALID;
			return COLUMN_NOT_EXIST;
		}
		//Aggreate only has 1 column
		size = 1;
		columns_2_print[0] = row_location_type->col_id;
		// printf("token: %d\n", cur->tok_value);
	}
	else
	{
		//Aggreate only has 1 column
		size = 1;
		columns_2_print[0] = -1;
	}
	
	
	//to right paren
	cur = cur->next;
	
	if (cur->tok_value != S_RIGHT_PAREN || cur->next == NULL)
	{
		cur->tok_value = INVALID;
		return MISSING_RIGHT_PAREN;
	}
	//to FROM
	cur = cur->next;
	//get Table name
	while (t_list->tok_value != K_FROM)
	{
		t_list=t_list->next;
		if (t_list->tok_value == EOC)
		{
			t_list->tok_value = INVALID;
			return MISSING_KEYWORD_FROM;
		}
	}
	//Sizes

	//to table name
	cur = cur->next;
	if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
	{
		// Error
		// printf("%s\n", cur->tok_string);
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
		return rc;
	}
	else
	{
		//TO EOF OR WHERE
		// cur = cur->next;
		rc = select_aggregate(tbh, table_name, cur->next, columns_2_print, size, agg_type);
		free(columns_2_print);
		cur->tok_value = INVALID;
		if (!rc)
		{
			//Need to check for EOF stuff later
		/*	if (cur == NULL || cur->tok_value != EOC)
			{	
				rc = INVALID_STATEMENT;
			}
			cur->tok_value = INVALID;*/
		}
	}
	return rc;
}

int select(table_file_header *tbh, char *tabname, token_list *cur, int columns_2_print[], int size_array) {

	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int i,j, length, count = 0, k, offset;
	char *current_position;
	int *int_write;
	char *char_write;
	char len;
	char *col_name;
	row_loc_type *row_location_type;
	int within_where_clause;
	token_list *cur_orig = cur;

	//Searches for the corrrect table entry
	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, tabname) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}

	//Initializes column entry
	cd_entry *column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
	//Prints Table top
	for (i = 0; i < size_array; i++)
	{
		printf("+----------------");

	}
	printf("+\n");
	//Prints column Names
	for (i = 0; i < size_array; i++)
	{
		column_entry = (cd_entry*) ((char*)column_entry  + (sizeof(cd_entry) * columns_2_print[i]));
		printf("|%-16s", column_entry->col_name);
		//Go to next column entry
		column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
	}
	printf("|\n");
	//Prints Header Bottom
	for (i = 0; i < size_array; i++)
	{
		printf("+----------------");
	}
	printf("+\n");

	//For each record
	for (i = 0; i < tbh->num_records; i++)
	{
		//Reinitializes column entry to check column type
		column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
		current_position = (char*)tbh + (i * tbh->record_size + tbh->record_offset);
		//If has no where 
		cur = cur_orig;
		if (cur->tok_value == EOC)
		{
			within_where_clause = 1;
		}
		else within_where_clause = 0;
		// printf("-----------------");
		// printf("original addy: %d\n", cur_orig);
		// printf("new addy: %d\n", cur);
		
		// printf("current token: %d\n\n", cur->tok_value);
		while (cur->tok_value != EOC && cur->next != NULL)
		{
			if ((cur->tok_value == K_WHERE || cur->tok_value == K_OR) && cur->next != NULL)
			{	
				cur = cur->next;
				if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
				{
					// Error
					cur->tok_value = INVALID;
					return INVALID_COLUMN_NAME;
				}
				row_location_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
				if (row_location_type == NULL)
				{
					cur->tok_value = INVALID;
					return COLUMN_NOT_EXIST;
				}
				cur = cur->next;
				within_where_clause = within_where_clause || 
					is_affected_by_where(cur, current_position , row_location_type);
								///if not a 0 or 1 than it errored
				if (within_where_clause != 0  && within_where_clause != 1)
				{
					cur->tok_value = INVALID;
					return within_where_clause;
				}
				while (cur->tok_value != K_OR && cur->tok_value != K_AND &&  cur->tok_value != EOC)
				{
					cur = cur->next;
				}

			}
			else if (cur->tok_value == K_AND && cur->next != NULL)
			{
				cur = cur->next;
				if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
				{
					// Error
					cur->tok_value = INVALID;
					return INVALID_COLUMN_NAME;
				}
				row_location_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
				if (row_location_type == NULL)
				{
					cur->tok_value = INVALID;
					return COLUMN_NOT_EXIST;
				}
				cur = cur->next;
				within_where_clause = within_where_clause && 
					is_affected_by_where(cur, current_position , row_location_type);
								///if not a 0 or 1 than it errored
				if (within_where_clause != 0  && within_where_clause != 1)
				{
					cur->tok_value = INVALID;
					return within_where_clause;
				}
				while (cur->tok_value != K_OR && cur->tok_value != K_AND &&  cur->tok_value != EOC)
				{
					cur = cur->next;
				}

			}
			else
			{
				cur->tok_value = INVALID;
				return MISSING_WHERE_OR_AND_OR_OR;
			}
		}
		// printf("boo: %d", within_where_clause);
		if (within_where_clause)
		{
			count++;
			
			//For each column 2 print
			for (j = 0; j < size_array; j++)
			{
				offset = 0;
				column_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);

				current_position = (char*)tbh + (i * tbh->record_size + tbh->record_offset);

				for (k = 0; k < columns_2_print[j]; k++)
				{
					offset += (column_entry->col_len+1);
					column_entry = (cd_entry*) ((char*)column_entry  + sizeof(cd_entry));
				}

				// printf("Offset %d", offset);
				// column_entry = (cd_entry*) ((char*)column_entry  + (sizeof(cd_entry) * columns_2_print[j]));
				current_position += (offset);
				if (column_entry->col_type == T_INT)
				{
					len = current_position[0];
					if (len == 0)
					{
						char* nulls = "NULL";
						printf("|%-16s" , nulls);
					}
					else
					{

						int_write = (int*)calloc(1, sizeof(int));
						
						memcpy((void*)int_write,
									     (void*)(current_position+1),
											sizeof(int));
						
						printf("|%16d" , *int_write);
						free(int_write);
					}

				}
				else if (column_entry->col_type == T_CHAR)
				{
					len = current_position[0];
					if (len == 0)
					{
						char* nulls = "NULL";
						printf("|%-16s" , nulls);
					}
					else
					{
						char_write = (char*)calloc(1, column_entry->col_len);

						memcpy((void*)char_write,
									     (void*)(current_position+1),
											(int)(len));
						
						printf("|%-16s", char_write);
					}
					
				}
				
					
				
			
			}
			printf("|\n");
		}
	}

	for (i = 0; i < size_array; i++)
	{
		printf("+----------------");
	}
	printf("+\n");
	printf("%d rows selected\n", count);


	return 0;
}

int sem_select(token_list *t_list)
{
	int rc = 0, count = 0, i, size = 0;
	token_list *cur = t_list;
	char *table_name;
	char *file_name;
	char extension[5] = ".tab";
	table_file_header *tbh;
	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int has_star = 0;
	row_loc_type *row_location_type;
	int *columns_2_print = (int *) calloc(1, MAX_NUM_COL);
	// int columns_2_print[];

	//get Table name
	while (t_list->tok_value != K_FROM)
	{
		t_list=t_list->next;
		if (t_list->tok_value == EOC)
		{
			t_list->tok_value = INVALID;
			return MISSING_KEYWORD_FROM;
		}
	}
	table_name = t_list->next->tok_string;
	file_name = (char*)calloc(1, sizeof(table_name) + sizeof(extension));
	strcpy(file_name,table_name);
	strcat(file_name,extension);
	tbh = getDataTable(file_name);
	

	//get tbd_entry
	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, table_name) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}

	if (cur->tok_value == S_STAR)
	{
		if (cur->next->tok_value == K_FROM)
		{
			size = tab_entry->num_columns;
			for (i = 0; i < size ; i++)
			{
				columns_2_print[i] = i;
			}
			//to column name
			cur = cur->next->next;
		}

		else 
		{
			cur->tok_value = INVALID;
			return MISSING_FROM_AFTER_STAR;
		}
		
	}
	//should be columns
	else
	{
		// get columns
		while (cur->tok_value != K_FROM)
		{
			if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
			{
				// Error
				cur->tok_value = INVALID;
				rc = INVALID_COLUMN_NAME;
				return rc;
				
			}
			row_location_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
			if (row_location_type == NULL)
			{
				cur->tok_value = INVALID;
				return COLUMN_NOT_EXIST;
			}
			columns_2_print[size] = row_location_type->col_id;
			size++;
			if (cur->next->tok_value == S_COMMA || cur->next->tok_value == K_FROM)
			{
				//if comma go to next column
				if (cur->next->tok_value == S_COMMA)
				{
					cur=cur->next;
				}
				//if from just stay to exit
				cur=cur->next;
			}
			else
			{
				cur->tok_value = INVALID;
				return INVALID_SELECT_STATEMENT;
			}
			
		}
		cur=cur->next;
		table_name = t_list->next->tok_string; 
	}
	//column
	if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
	{
		// Error
		// printf("%s\n", cur->tok_string);
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
		return rc;
	}
	else
	{
		
		file_name = (char*)calloc(1, sizeof(table_name) + sizeof(extension));
		strcpy(file_name,table_name);
		strcat(file_name,extension);
		if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
		{
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
		}
		else
		{		
			tbh = getDataTable(file_name);
			// printf("%d", cur->next->tok_value);
			rc = select(tbh, table_name, cur->next, columns_2_print, size);
			free(columns_2_print);
			if (rc)
			{
				cur = cur->next;
				if (cur->tok_value != EOC)
				{
					rc = INVALID_STATEMENT;
				}
				cur->tok_value = INVALID;
			}
		}
	}
	return rc;
}

int sem_select_data(token_list *t_list)
{
	int rc;
	token_list *cur;
	cur = t_list;
	if (cur->tok_value == F_SUM || cur->tok_value == F_AVG || cur->tok_value == F_COUNT)
	{
		if (cur->next != NULL && cur->next->tok_value == S_LEFT_PAREN)
		{
			//go to aggregate stuffs
			return sem_select_aggregate(cur);
		}
		else 
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
			return rc;
		}
	}
	else if (cur->tok_value == S_STAR || cur->tok_class == keyword || (cur->tok_class == identifier) || (cur->tok_class == type_name))
	{
		if ((cur->next != NULL) && (cur->next->next != NULL))
		{
			//TODO insert a where
			return sem_select(cur);
		}
		else 
		{
			rc = INVALID_STATEMENT;
			cur->tok_value = INVALID;
			return rc;
		}
		
	}
	else 
	{
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
		return rc;
	}
}

int update_where(table_file_header *table_fh, char *file_name, char *tabname, char* col_name, token_list *t_list)
{
	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	table_file_header *tbh = table_fh;
	int num_tables = g_tpd_list->num_tables;
	FILE *fhandle = NULL;

	int i,j, length, count, temp3;
	char *current_position;
	int *int_write;
	char *char_write;
	char len;
	int row_offset;
	row_loc_type *row_location_type;
	row_loc_type *where_loc_type;
	int within_where_clause;
	bool found = false;
	char temp_num[4];
	token_list *cur = t_list->next->next;

	
	char *new_file = (char *) calloc(1, table_fh->file_size);

	//Searches for the corrrect table entry
	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, tabname) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}
	//check set column first
	row_location_type = getRecordOffSet(tab_entry, col_name, tbh);
	if (row_location_type == NULL)
	{
		t_list->tok_value = INVALID;
		return COLUMN_NOT_EXIST;
	}
	//check where clause column
	if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
	{
		// Error
		cur->tok_value = INVALID;
		return INVALID_COLUMN_NAME;
	}
	where_loc_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
	if (where_loc_type == NULL)
	{
		cur->tok_value = INVALID;
		return COLUMN_NOT_EXIST;
	}

	//Reinitializes column entry to check column type
	
	//Delete file
	remove(file_name);
	if((fhandle = fopen(file_name, "wbc")) == NULL)
	{
		return FILE_OPEN_ERROR;
	}
	else
	{
		cur = cur->next;
		memcpy((void*)new_file,(void*)tbh, tbh->record_offset);
		//for each record
		count = 0;
		for (i = 0; i < tbh->num_records; i++)
		{
			current_position = (char*)tbh + (i * tbh->record_size + tbh->record_offset );
			within_where_clause = is_affected_by_where(cur, current_position , where_loc_type);
			///if not a 0 or 1 than it errored
			if (within_where_clause != 0  && within_where_clause != 1)
			{
				t_list->tok_value = INVALID;
				return within_where_clause;
			}
			else if (within_where_clause)
			{
				count++;
				if (row_location_type->type == T_INT)
				{
					if (t_list->tok_value == K_NULL)
					{
						if (row_location_type->not_null)
						{
							t_list->tok_value = INVALID;
							return CANNOT_INSERT_NULL;
						}
						else
						{
							int_write = (int*)calloc(1, sizeof(int));
							current_position[row_location_type->offset] = '\0';
							memcpy(current_position + 1 + row_location_type->offset,
							    			 (void*)(int_write),
													sizeof(int));
						}
					}
					else
					{
						temp3 = atoi(t_list->tok_string);
	    				memcpy(temp_num, &temp3, sizeof(int));
				    	for (j = 0; j < 4; j++)
				    	{
				    		current_position[(1 + row_location_type->offset + j)] = temp_num[j];
				    	}
				    	current_position[row_location_type->offset] = sizeof(int);
					}
				}
				else
				{
					if (t_list->tok_value == K_NULL)
					{
						if (row_location_type->not_null)
						{
							t_list->tok_value = INVALID;
							return CANNOT_INSERT_NULL;
						}
						else
						{
							char_write = (char*)calloc(1, row_location_type->size);
							current_position[row_location_type->offset] = '\0';
							memcpy(current_position + row_location_type->offset + 1,
							    			 (void*)(char_write),
													row_location_type->size);
						}
					}
					else
					{

						if (strlen(t_list->tok_string) > row_location_type->size)
						{
							t_list->tok_value = INVALID;
							return TOO_LARGE_PARAMETER;
						}
						else
						{
								
							current_position[row_location_type->offset] = strlen(t_list->tok_string);
							memcpy(current_position + 1 + row_location_type->offset,
									     (void*)(t_list->tok_string),
											row_location_type->size);
						}
						
					}
					
				}
			}
		}
		((table_file_header*) new_file)->file_size = tbh->record_offset + (count * tbh->record_size);
		((table_file_header*) new_file)->num_records = count;
		printf("%d rows updated\n", count);
		fwrite(tbh, tbh->file_size, 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
		return 0;
	}
}

int update_all(table_file_header *table_fh, char *file_name, char *tabname, char* col_name, token_list *t_list) {

	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	table_file_header *tbh = table_fh;
	int num_tables = g_tpd_list->num_tables;
	FILE *fhandle = NULL;

	int i,j, length, count;
	char *current_position;
	int *int_write;
	char *char_write;
	char len;
	int row_offset, temp3;
	row_loc_type *row_location_type;
	int within_where_clause;
	bool found = false;
	row_int *int_row_to_add;	
	char temp_num[4];
	
	char *new_file = (char *) calloc(1, table_fh->file_size);

	//Searches for the corrrect table entry
	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, tabname) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}

	row_location_type = getRecordOffSet(tab_entry, col_name, tbh);
	if (row_location_type == NULL)
	{
		t_list->tok_value = INVALID;
		return COLUMN_NOT_EXIST;
	}		
	if (row_location_type->type == T_INT)
	{
	
	}
	//Reinitializes column entry to check column type
	
	//Delete file
	remove(file_name);
	if((fhandle = fopen(file_name, "wbc")) == NULL)
	{
		t_list->tok_value = INVALID;
		return FILE_OPEN_ERROR;
	}
	else
	{
		memcpy((void*)new_file,(void*)tbh, tbh->record_offset);
		//for each record
		count = 0;
		for (i = 0; i < tbh->num_records; i++)
		{
			current_position = (char*)tbh + (i * tbh->record_size + tbh->record_offset );
			if (row_location_type->type == T_INT)
			{

				if (t_list->tok_value == K_NULL)
				{
					if (row_location_type->not_null)
					{
						t_list->tok_value = INVALID;
						return CANNOT_INSERT_NULL;
					}
					else
					{
						int_write = (int*)calloc(1, sizeof(int));
						current_position[row_location_type->offset] = '\0';
						memcpy(current_position + 1 + row_location_type->offset,
						    			 (void*)(int_write),
												sizeof(int));
					}
				}
				else
				{
		    		temp3 = atoi(t_list->tok_string);
    				memcpy(temp_num, &temp3, sizeof(int));
			    	for (j = 0; j < 4; j++)
			    	{
			    		current_position[(1 + row_location_type->offset + j)] = temp_num[j];
			    	}
			    	current_position[row_location_type->offset] = sizeof(int);
				}
			}
			else
			{
				if (t_list->tok_value == K_NULL)
				{
					if (row_location_type->not_null)
					{
						t_list->tok_value = INVALID;
						return CANNOT_INSERT_NULL;
					}
					else
					{
						char_write = (char*)calloc(1, row_location_type->size);
						current_position[row_location_type->offset] = '\0';
						memcpy(current_position + row_location_type->offset + 1,
						    			 (void*)(char_write),
												row_location_type->size);
					}
				}
				else
				{
					if (strlen(t_list->tok_string) > row_location_type->size)
					{
						t_list->tok_value = INVALID;
						return TOO_LARGE_PARAMETER;
					}
					else
					{
							current_position[row_location_type->offset] = strlen(t_list->tok_string);
							memcpy(current_position + 1 + row_location_type->offset,
							     (void*)(t_list->tok_string),
									row_location_type->size);
					}
				
				}
				
			}
		}
		((table_file_header*) new_file)->file_size = tbh->record_offset + (count * tbh->record_size);
		((table_file_header*) new_file)->num_records = count;
		printf("%d rows updated\n", tbh->num_records);
		fwrite(tbh, tbh->file_size, 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
		return 0;
	}
}


int sem_update_data(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	cur = t_list;
	tpd_entry *tab_entry = NULL;
	char *table_name;
	char *file_name;
	char extension[5] = ".tab";
	table_file_header *tbh;
	table_file_header *original_tbh;
	FILE *fhandle = NULL;
	char *column_name;
	char *value;
	//Currently at table name
	if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
	{
		// Error
		cur->tok_value = INVALID;
		return  INVALID_TABLE_NAME;
	
	}
	else 
	{
		//check if table exists
		if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
		{
			cur->tok_value = INVALID;
			return  TABLE_NOT_EXIST;
		}
		else 
		{
			//Initiates filename, and backups table
			table_name = cur->tok_string;
			file_name = (char*)calloc(1, sizeof(table_name) + sizeof(extension));
			strcpy(file_name,table_name);
			strcat(file_name,extension);
			tbh = getDataTable(file_name);
			original_tbh  = (table_file_header*)calloc(1, tbh->file_size);;
			memcpy((void*)original_tbh,(void*)tbh, tbh->file_size);
			cur = cur->next;
			if (cur->tok_value != K_SET || (cur->next == NULL))
			{
				cur->tok_value = INVALID;
				return MISSING_KEYWORD_SET;
			}
			else
			{
				cur = cur->next;
				//To column
				if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
				{
					// Error
					cur->tok_value = INVALID;
					return INVALID_COLUMN_NAME;
				}
				else
				{
					column_name = cur->tok_string;
					cur = cur->next;
					if (cur->tok_value != S_EQUAL || (cur->next == NULL))
					{
						// Error
						cur->tok_value = INVALID;
						return MISSING_EQUALS_AFTER_SET;
					}
					else
					{
						//at variable
						cur = cur->next;
						if (cur->tok_value != INT_LITERAL && cur->tok_value != STRING_LITERAL && cur->tok_value != K_NULL)
						{
							cur->tok_value = INVALID;
							return MUST_COMPARE_WITH_INT_OR_STRING_OR_NULL;
						}
						else
						{
							if (cur->next->tok_value == K_WHERE &&  (cur->next->next != NULL))
							{
								rc = update_where(tbh, file_name,table_name, column_name, cur);
							} 
							//check for END OF LINE
							else if (cur->next->tok_value == EOC)
							{
								//update all from table but leave header
								rc = update_all(tbh, file_name,table_name, column_name, cur);
							}
						}
					}
				}
				
			}
		}
	}
			
	
	if (!rc)
	{
		cur->tok_value = INVALID;
	}
	//Restores original TBH before changes
	else
	{
		if((fhandle = fopen(file_name, "wbc")) == NULL)
		{
			printf("Cannot open File\nError in the string: %s\n",cur->tok_string);
			return FILE_OPEN_ERROR;
		}
		else
		{
			//rewrite original file
			fwrite(original_tbh, original_tbh->file_size, 1, fhandle);	
			fflush(fhandle);
			fclose(fhandle);
			
		}
	}
	free(file_name);
	free(original_tbh);
	return rc;
}



int delete_where(token_list *cur, table_file_header *table_fh, char *file_name, char *tabname)
{
	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int i,j, length, count;
	char *current_position;
	int *int_write;
	char *char_write;
	char len;
	int row_offset;
	row_loc_type *row_location_type;
	int within_where_clause;
	table_file_header *tbh = table_fh;
	FILE *fhandle = NULL;
	char *new_file = (char *) calloc(1, table_fh->file_size);


	//Searches for the corrrect table entry
	if (num_tables > 0)
	{
		{
			while ((!found) && (num_tables-- > 0))
			{
				if (stricmp(cur_tpd->table_name, tabname) == 0)
				{
					/* found it */
					found = true;
					tab_entry = cur_tpd;
				}
				else
				{
					if (num_tables > 0)
					{
						cur_tpd = (tpd_entry*)((char*)cur_tpd + cur_tpd->tpd_size);
					}

				}
			}
		}
	}
	//Checks if valid column name
	if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
	{
		// Error
		cur->tok_value = INVALID;
		return INVALID_COLUMN_NAME;
	}
	//check if column exists and the offset/size in that row
	else
	{

		row_location_type = getRecordOffSet(tab_entry, cur->tok_string, tbh);
		if (row_location_type == NULL)
		{
			cur->tok_value = INVALID;
			return COLUMN_NOT_EXIST;
		}		
		//Found the offset of memory to read for each record.
		else
		{
			//go to relational operator
			cur = cur->next;
			//Delete file
			remove(file_name);
			if((fhandle = fopen(file_name, "wbc")) == NULL)
			{
				return FILE_OPEN_ERROR;
			}
			else
			{
				memcpy((void*)new_file,(void*)tbh, tbh->record_offset);
				//for each record
				count = 0;
				for (i = 0; i < tbh->num_records; i++)
				{
						//Reinitializes column entry to check column type
						current_position = (char*)tbh + (i * tbh->record_size + tbh->record_offset );
						within_where_clause = is_affected_by_where(cur, current_position , row_location_type);
						///if not a 0 or 1 than it errored
						if (within_where_clause != 0  && within_where_clause != 1)
						{
							return within_where_clause;
						}
						//If not affected by where
						else if (!within_where_clause)
						{	
							memcpy((void *) (new_file + (count * tbh->record_size + tbh->record_offset)),
									(void *)current_position, 
										tbh->record_size);
							count++;
						}

				}
				((table_file_header*) new_file)->file_size = tbh->record_offset + (count * tbh->record_size);
				((table_file_header*) new_file)->num_records = count;
				printf("%d rows deleted\n", tbh->num_records - count);
				fwrite(new_file, ((table_file_header*) new_file)->file_size, 1, fhandle);
				fflush(fhandle);
				fclose(fhandle);
				return 0;
			}
		}
	}
}

int delete_all(table_file_header *tbh, char *file_name) {
	tpd_entry *tab_entry = NULL;
	tpd_entry *cur_tpd = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int i,j, length;
	char *current_position;
	int *int_write;
	char *char_write;
	char len;

	tbh->file_size = tbh->record_offset;
	tbh->num_records = 0;

	remove(file_name);
	create_file(tbh->file_size, file_name, tbh);
	return 0;
}


int sem_delete_data(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	cur = t_list;
	tpd_entry *tab_entry = NULL;
	char *table_name;
	char *file_name;
	char extension[5] = ".tab";
	table_file_header *tbh;
	table_file_header *original_tbh;
	FILE *fhandle = NULL;

	//GO TO table name
	cur = cur->next;
	if ((cur->tok_class != keyword) && (cur->tok_class != identifier) && (cur->tok_class != type_name))
	{
		// Error
		rc = INVALID_TABLE_NAME;
		cur->tok_value = INVALID;
	}
	else 
	{
		//check if table exists
		if ((tab_entry = get_tpd_from_list(cur->tok_string)) == NULL)
		{
			rc = TABLE_NOT_EXIST;
			cur->tok_value = INVALID;
		}
		else 
		{
			table_name = cur->tok_string;
			file_name = (char*)calloc(1, sizeof(table_name) + sizeof(extension));
			strcpy(file_name,table_name);
			strcat(file_name,extension);
			tbh = getDataTable(file_name);
			original_tbh  = (table_file_header*)calloc(1, tbh->file_size);;
			memcpy((void*)original_tbh,(void*)tbh, tbh->file_size);
			cur = cur->next;
			if (cur->tok_value == K_WHERE && (cur->next != NULL))
			{
				rc = delete_where(cur->next, tbh, file_name,table_name);
			}
			//check for END OF LINE
			else if (cur->tok_value == EOC)
			{
				//delete all from table but leave header
				rc = delete_all(tbh, file_name);
			}
			else
			{
				cur->tok_value = INVALID;
				//anything just invalidate everything
				rc = INVALID_STATEMENT;
			}
		}
		if (!rc)
		{
			cur->tok_value = INVALID;
		}
		//Restores original TBH before changes
		else
		{
			if((fhandle = fopen(file_name, "wbc")) == NULL)
			{
				printf("Cannot open File\nError in the string: %s\n",cur->tok_string);
				return FILE_OPEN_ERROR;
			}
			else
			{
				//rewrite original file
				fwrite(original_tbh, original_tbh->file_size, 1, fhandle);	
				fflush(fhandle);
				fclose(fhandle);
				
			}
		}
		free(file_name);
		free(original_tbh);
	}
	return rc;
}

int sem_list_schema(token_list *t_list)
{
	int rc = 0;
	token_list *cur;
	tpd_entry *tab_entry = NULL;
	cd_entry  *col_entry = NULL;
	char tab_name[MAX_IDENT_LEN+1];
	char filename[MAX_IDENT_LEN+1];
	bool report = false;
	FILE *fhandle = NULL;
	int i = 0;

	cur = t_list;

	if (cur->tok_value != K_FOR)
  {
		rc = INVALID_STATEMENT;
		cur->tok_value = INVALID;
	}
	else
	{
		cur = cur->next;

		if ((cur->tok_class != keyword) &&
			  (cur->tok_class != identifier) &&
				(cur->tok_class != type_name))
		{
			// Error
			rc = INVALID_TABLE_NAME;
			cur->tok_value = INVALID;
		}
		else
		{
			memset(filename, '\0', MAX_IDENT_LEN+1);
			strcpy(tab_name, cur->tok_string);
			cur = cur->next;

			if (cur->tok_value != EOC)
			{
				if (cur->tok_value == K_TO)
				{
					cur = cur->next;
					
					if ((cur->tok_class != keyword) &&
						  (cur->tok_class != identifier) &&
							(cur->tok_class != type_name))
					{
						// Error
						rc = INVALID_REPORT_FILE_NAME;
						cur->tok_value = INVALID;
					}
					else
					{
						if (cur->next->tok_value != EOC)
						{
							rc = INVALID_STATEMENT;
							cur->next->tok_value = INVALID;
						}
						else
						{
							/* We have a valid file name */
							strcpy(filename, cur->tok_string);
							report = true;
						}
					}
				}
				else
				{ 
					/* Missing the TO keyword */
					rc = INVALID_STATEMENT;
					cur->tok_value = INVALID;
				}
			}

			if (!rc)
			{
				if ((tab_entry = get_tpd_from_list(tab_name)) == NULL)
				{
					rc = TABLE_NOT_EXIST;
					cur->tok_value = INVALID;
				}
				else
				{
					if (report)
					{
						if((fhandle = fopen(filename, "a+tc")) == NULL)
						{
							rc = FILE_OPEN_ERROR;
						}
					}

					if (!rc)
					{
						/* Find correct tpd, need to parse column and index information */

						/* First, write the tpd_entry information */
						printf("Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
						printf("Table Name               (table_name)  = %s\n", tab_entry->table_name);
						printf("Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
						printf("Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
            printf("Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 

						if (report)
						{
							fprintf(fhandle, "Table PD size            (tpd_size)    = %d\n", tab_entry->tpd_size);
							fprintf(fhandle, "Table Name               (table_name)  = %s\n", tab_entry->table_name);
							fprintf(fhandle, "Number of Columns        (num_columns) = %d\n", tab_entry->num_columns);
							fprintf(fhandle, "Column Descriptor Offset (cd_offset)   = %d\n", tab_entry->cd_offset);
              fprintf(fhandle, "Table PD Flags           (tpd_flags)   = %d\n\n", tab_entry->tpd_flags); 
						}

						/* Next, write the cd_entry information */
						for(i = 0, col_entry = (cd_entry*)((char*)tab_entry + tab_entry->cd_offset);
								i < tab_entry->num_columns; i++, col_entry++)
						{
							printf("Column Name   (col_name) = %s\n", col_entry->col_name);
							printf("Column Id     (col_id)   = %d\n", col_entry->col_id);
							printf("Column Type   (col_type) = %d\n", col_entry->col_type);
							printf("Column Length (col_len)  = %d\n", col_entry->col_len);
							printf("Not Null flag (not_null) = %d\n\n", col_entry->not_null);

							if (report)
							{
								fprintf(fhandle, "Column Name   (col_name) = %s\n", col_entry->col_name);
								fprintf(fhandle, "Column Id     (col_id)   = %d\n", col_entry->col_id);
								fprintf(fhandle, "Column Type   (col_type) = %d\n", col_entry->col_type);
								fprintf(fhandle, "Column Length (col_len)  = %d\n", col_entry->col_len);
								fprintf(fhandle, "Not Null Flag (not_null) = %d\n\n", col_entry->not_null);
							}
						}
	
						if (report)
						{
							fflush(fhandle);
							fclose(fhandle);
						}
					} // File open error							
				} // Table not exist
			} // no semantic errors
		} // Invalid table name
	} // Invalid statement

  return rc;
}

int initialize_tpd_list()
{
	int rc = 0;
	FILE *fhandle = NULL;
	struct _stat file_stat;

  /* Open for read */
  if((fhandle = fopen("dbfile.bin", "rbc")) == NULL)
	{
		if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
		{
			rc = FILE_OPEN_ERROR;
		}
    else
		{
			g_tpd_list = NULL;
			g_tpd_list = (tpd_list*)calloc(1, sizeof(tpd_list));
			
			if (!g_tpd_list)
			{
				rc = MEMORY_ERROR;
			}
			else
			{
				g_tpd_list->list_size = sizeof(tpd_list);
				fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
				fflush(fhandle);
				fclose(fhandle);
			}
		}
	}
	else
	{
		/* There is a valid dbfile.bin file - get file size */
		_fstat(_fileno(fhandle), &file_stat);
		printf("dbfile.bin size = %d\n", file_stat.st_size);

		g_tpd_list = (tpd_list*)calloc(1, file_stat.st_size);

		if (!g_tpd_list)
		{
			rc = MEMORY_ERROR;
		}
		else
		{
			fread(g_tpd_list, file_stat.st_size, 1, fhandle);
			fflush(fhandle);
			fclose(fhandle);

			if (g_tpd_list->list_size != file_stat.st_size)
			{
				rc = DBFILE_CORRUPTION;
			}

		}
	}
    
	return rc;
}
	
int add_tpd_to_list(tpd_entry *tpd)
{
	int rc = 0;
	int old_size = 0;
	FILE *fhandle = NULL;

	if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
	{
		rc = FILE_OPEN_ERROR;
	}
  else
	{
		old_size = g_tpd_list->list_size;

		if (g_tpd_list->num_tables == 0)
		{
			/* If this is an empty list, overlap the dummy header */
			g_tpd_list->num_tables++;
		 	g_tpd_list->list_size += (tpd->tpd_size - sizeof(tpd_entry));
			fwrite(g_tpd_list, old_size - sizeof(tpd_entry), 1, fhandle);
		}
		else
		{
			/* There is at least 1, just append at the end */
			g_tpd_list->num_tables++;
		 	g_tpd_list->list_size += tpd->tpd_size;
			fwrite(g_tpd_list, old_size, 1, fhandle);
		}

		fwrite(tpd, tpd->tpd_size, 1, fhandle);
		fflush(fhandle);
		fclose(fhandle);
	}

	return rc;
}

int drop_tpd_from_list(char *tabname)
{
	int rc = 0;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;
	int count = 0;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (stricmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				int old_size = 0;
				FILE *fhandle = NULL;

				if((fhandle = fopen("dbfile.bin", "wbc")) == NULL)
				{
					rc = FILE_OPEN_ERROR;
				}
			  else
				{
					old_size = g_tpd_list->list_size;

					if (count == 0)
					{
						/* If this is the first entry */
						g_tpd_list->num_tables--;

						if (g_tpd_list->num_tables == 0)
						{
							/* This is the last table, null out dummy header */
							memset((void*)g_tpd_list, '\0', sizeof(tpd_list));
							g_tpd_list->list_size = sizeof(tpd_list);
							fwrite(g_tpd_list, sizeof(tpd_list), 1, fhandle);
						}
						else
						{
							/* First in list, but not the last one */
							g_tpd_list->list_size -= cur->tpd_size;

							/* First, write the 8 byte header */
							fwrite(g_tpd_list, sizeof(tpd_list) - sizeof(tpd_entry),
								     1, fhandle);

							/* Now write everything starting after the cur entry */
							fwrite((char*)cur + cur->tpd_size,
								     old_size - cur->tpd_size -
										 (sizeof(tpd_list) - sizeof(tpd_entry)),
								     1, fhandle);
						}
					}
					else
					{
						/* This is NOT the first entry - count > 0 */
						g_tpd_list->num_tables--;
					 	g_tpd_list->list_size -= cur->tpd_size;

						/* First, write everything from beginning to cur */
						fwrite(g_tpd_list, ((char*)cur - (char*)g_tpd_list),
									 1, fhandle);

						/* Check if cur is the last entry. Note that g_tdp_list->list_size
						   has already subtracted the cur->tpd_size, therefore it will
						   point to the start of cur if cur was the last entry */
						if ((char*)g_tpd_list + g_tpd_list->list_size == (char*)cur)
						{
							/* If true, nothing else to write */
						}
						else
						{
							/* NOT the last entry, copy everything from the beginning of the
							   next entry which is (cur + cur->tpd_size) and the remaining size */
							fwrite((char*)cur + cur->tpd_size,
										 old_size - cur->tpd_size -
										 ((char*)cur - (char*)g_tpd_list),							     
								     1, fhandle);
						}
					}

					fflush(fhandle);
					fclose(fhandle);
				}

				
			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
					count++;
				}
			}
		}
	}
	
	if (!found)
	{
		rc = INVALID_TABLE_NAME;
	}

	return rc;
}

tpd_entry* get_tpd_from_list(char *tabname)
{
	tpd_entry *tpd = NULL;
	tpd_entry *cur = &(g_tpd_list->tpd_start);
	int num_tables = g_tpd_list->num_tables;
	bool found = false;

	if (num_tables > 0)
	{
		while ((!found) && (num_tables-- > 0))
		{
			if (stricmp(cur->table_name, tabname) == 0)
			{
				/* found it */
				found = true;
				tpd = cur;
			}
			else
			{
				if (num_tables > 0)
				{
					cur = (tpd_entry*)((char*)cur + cur->tpd_size);
				}
			}
		}
	}

	return tpd;
}


