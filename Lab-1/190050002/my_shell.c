#include  <stdio.h>
#include  <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

static int KILL_CHECK = 0;

int background_proc[MAX_NUM_TOKENS];
int bg_index=0;

pid_t fg_group;

/* Splits the string by space and returns the array of tokens
*
*/
char **tokenize(char *line)
{
  char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
  char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
  int i, tokenIndex = 0, tokenNo = 0;

  for(i =0; i < strlen(line); i++){

    char readChar = line[i];

    if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
      token[tokenIndex] = '\0';

      if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
      }

    }

    else {
      token[tokenIndex++] = readChar;
    }
    
  }
 
  free(token);
  tokens[tokenNo] = NULL ;
  return tokens;
}

void check_and(char **Tokens, int *bg, int *sr, int *pr) {
	int i = 0;
	while(Tokens[i] != NULL)
	{
		if (strcmp(Tokens[i], "&") == 0) *bg = 1;
		if (strcmp(Tokens[i], "&&") == 0) *sr = 1;
		if (strcmp(Tokens[i], "&&&") == 0) *pr = 1;
		i = i + 1;
	}
}

void sigint_handler(int num)
{
	KILL_CHECK = 1;
	kill(0, SIGTERM);
	printf("\n$");
}


int main(int argc, char* argv[]) {
	char  line[MAX_INPUT_SIZE];            
	char  **tokens;              
	int i;

	signal(SIGINT, sigint_handler);
	signal(SIGTERM, SIG_IGN);


	int j = 0, k = 0, a, status;
	int pp = 0;
	int bg=0, sr=0, pr=0;
	for(int j=0; j<64; j++) background_proc[j]=-1;


	while(1) {	

		a = waitpid(-1, NULL, WNOHANG);

    while (a>0) {
    	printf("%s\n","Shell: Background processes finished" );

			int change = 0;
			for (j=0; j<63; j++) {
				if (background_proc[j] == a) {
					change = 1;
				}
				if (change) background_proc[j] = background_proc[j+1];
			}
			bg_index--;
			background_proc[63] = -1;
			// printf("%d\n", change);

			a = waitpid(-1, NULL, WNOHANG);
    }

		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		printf("$ ");
		scanf("%[^\n]", line);
		getchar();

		// printf("Command entered: %s (remove this debug output later)\n", line);
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line);
   
    //do whatever you want with the commands, here we just print them

		// for(i=0;tokens[i]!=NULL;i++){
		// 	printf("found token %s (remove this debug output later)\n", tokens[i]);
		// }
		bg =0;
		pr = 0;
		sr = 0;
		check_and(tokens, &bg, &sr, &pr);
		KILL_CHECK = 0;

		int i = 0, counter = 0;
		int num_proc = 1;
		pp=0;
		while(tokens[i] != NULL) {
				if (strcmp(tokens[i], "&&&") == 0 || strcmp(tokens[i], "&&") == 0 || strcmp(tokens[i], "&") == 0) {
					tokens[i] = NULL;
					num_proc = num_proc +1;
				}
				i = i + 1;
		}

		if (bg + sr + pr > 1) {
			printf("%s\n","Shell: Incorrect command" );
		}

		else {

			i = 0;

			int parallel_proc[num_proc];
			for (int j=0; j<num_proc; j++) parallel_proc[j]=0;
			pp = 0;
			while(tokens[0] != NULL) {

				if (KILL_CHECK == 1) {

					while (tokens[0] != NULL) {
						int k = 0;
						while(tokens[k] != NULL) {
							free(tokens[k]);
							k = k + 1;
						}

						i = k + 1;
						tokens = &tokens[i];
						// printf("The direction of s is: %p\n (remove this debug output later)", (void *) tokens[0]);

					}

					break;
				}

				if (!strcmp(tokens[0],"exit")){
					for(i=0;i<64;i++){

						if(background_proc[i]>-1){
							kill(background_proc[i],SIGKILL); //Check the order of arguments
							background_proc[i] = -1;
							// printf("%s\n", "Shell: Background process killed (remove this debug output later)" ); //DEBUG
						}
					}
					// printf("The direction of s is: %p (remove this debug output later)\n", tokens[0]);


					for(i=0;tokens[i]!=NULL;i++){
					  free(tokens[i]);
					}
					tokens = &tokens[2];
					// printf("Exiting\n"); //DEBUG

					if (*tokens!=NULL) free(tokens);

					// printf("Exiting\n"); //DEBUG

					exit(0);

				}

				if (strcmp(tokens[0], "cd") == 0){
					if (tokens[1] == NULL){

						printf("%s\n","Shell: Incorrect command" );
						// break;

					}

					else{

						int check;
						check = chdir(tokens[1]);
						if (check == -1) {
							printf("%s\n", "Shell: Incorrect command" );
						}

					}

				}

				else {

					pid_t child_pid;
					child_pid = fork();

					if (child_pid == 0){
						int check;						
						fg_group = child_pid;
						check = execvp(tokens[0], tokens);
						if (check == -1) printf("%s\n", "Shell: Incorrect command" );
						exit(EXIT_FAILURE);
					}

					else if (child_pid < 0) {

						printf("%s\n", "Shell: Fork failed" );
						// exit(EXIT_FAILURE);

					}
					// else if (child_pid > 0) wait(NULL);
					else if (child_pid > 0) {
						parallel_proc[pp] = child_pid;
						pp = pp+1;

						if (bg == 1) {
							background_proc[bg_index] = child_pid;
							bg_index++;
							setpgid(child_pid, 0);
							// printf("%s\n", "I don't want to kill children (remove this debug output later)" );
						}

						else if (pr == 1) {
							counter = counter+1;
							// printf("%s\n", "No children were harmed (remove this debug output later)" );
						}

						else if (sr == 1) {
							waitpid(child_pid, NULL, 0);
							// printf("%s\n", "Killed a child! bwahahahaha (remove this debug output later)" );
						}

						else {
							int k = waitpid(child_pid, NULL, 0);
							// printf("%s\n", "Killed that child! yeahhhhhh (remove this debug output later)" );
						}

					}

				}

				if (pr || sr) {
					int k = 0;
					while(tokens[k] != NULL) {
						free(tokens[k]);
						k = k + 1;
					}

					i = k + 1;
					tokens = &tokens[i];
				}

				else {
					// tokens[0] = NULL;
					break;
				}

			}

			if (pr) for (int i = 0; i < counter; i++) {
				waitpid(parallel_proc[i], NULL, 0);
			}
		
		}

		// bg = 0;
		// pr = 0;
		// sr = 0;

		// Freeing the allocated memory	
		for(i=0;tokens[i]!=NULL;i++){
			free(tokens[i]);
		}
		
		if (*tokens!=NULL) {
			// printf("%s\n", "which freee (remove this debug output later)");
			// printf("The direction of s is: %p\n (remove this debug output later)", (void *) *tokens);
			free(tokens);
			// printf("%s\n", "which freee (remove this debug output later)");
		}

	}
	return 0;
}
