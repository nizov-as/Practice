#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

typedef struct _List 
{
    char *word;             // указатель на слово в звене
    struct _List *next;     // указатель на следующий элемент звена
} List;

// добавление слова в список
List *addWordInList(List *node, char *current_word);

// печати:
void printList(List *node);                       // печать списка
void printArr(char** arr);                        // печать массива
void printDoubleArr(char ***arr, int *length);    // печать двумерного массива

// очистка памяти: удаление списка
void deleteList(List *node);

// очистка памяти: удаление массива
void deleteArray(char **arr);                        // удаление одномерного массива
void deleteDoubleArray(char ***arr, int *length);    // удаление двумерного массива

// чтение одной строки и формирование списка
List *readLine(int *not_only_enter);

// проверки на конкретные команды:
int is_cd(char **arr);                                            // на команду cd
int is_exit(char **arr);                                          // на команду exit
int is_redirection(char **arr, int *first_redir_symbol_place);    // на команду перенаправления ввода/вывода
int is_background(char **arr, int *background_symb_place);        // на команду запуска в фоновом режиме
int is_pipes_amps_seq(char **arr);                                // на команды ||, && или ;

// функции для создания двумерного массива из списка
int howMuchElements(List *node);                          // подсчёт числа элементов списка (для корректного выделения памяти)
char **makeArrayForOneProc(List **node);                  // создание одномерного массива
char ***makeArrayForConveyer(List *node, int *length);    // создание массива из одномерных массивов (то есть двумерного массива)

// обработка текущей команды
void cmdProcessing(char ***arr, int *length);

// обработки команд:
char **redirectionProcessing(char **arr, int first_redir_symbol_place);    // перенаправление ввода-вывода
char **backgroundProcessing(char **arr, int background_symb_place);        // фоновый режим

// функция-обработчик сигнала SIGCHLD
void zombieRemove(int s);

// функции для создания двумерного массива из массива (для работы с командами || && ;)
int howMuchElements2(char **arr);
char **makeArrayForCmd(char **arr, int *place);
char ***makeArrayForAllCmd(char **arr, int *length);

//========================================================
//========================================================

int main (int argc, char *argv[])
{   
    int not_only_enter = 0;
    int arr_length;    // количество команд в конвейере
    List *tmp_list = NULL;      
    printf("alexandernizov$ ");

    tmp_list = readLine(&not_only_enter); 
    //signal (SIGCHLD, zombieRemove);
    while (tmp_list != NULL)
    {
        char ***array = NULL;
        array = makeArrayForConveyer(tmp_list, &arr_length);
        
        if(not_only_enter)
        {
            cmdProcessing(array, &arr_length);
        }
        
        deleteList(tmp_list);
        deleteDoubleArray(array, &arr_length);
        printf("alexandernizov$ ");
        tmp_list = readLine(&not_only_enter);
    }
    
    deleteList(tmp_list);
    free(tmp_list);
    
    return 0;
}

//========================================================
//========================================================

List *addWordInList (List *node, char *current_word)
{
    if (node == NULL)
    {
        node = (List*)malloc(sizeof(List));
        node->word = (char*)malloc((strlen(current_word) + 1) * sizeof(char));
        strcpy(node->word, current_word);
        node->next = NULL;
    }
    else node->next = addWordInList (node->next, current_word);
    return node;
}

//========================================================

void printList(List *node)
{
    if (node)
    {
        printf("%s\n", node->word);
        printList(node->next);
    }
}

//========================================================

void printArr(char** arr)
{    
    int j = 0;
    while(arr[j] != NULL)
	{
		printf("%s, ", arr[j]);
        j++;
	}
    printf("%s\n", arr[j]);
}

void printDoubleArr(char ***arr, int *length)
{   
    int j;
	for (j = 0; j < *length; j++)
    {
        printArr(arr[j]);
    }
    printf("\n");
}

//========================================================

void deleteList (List *node)
{
    if (node != NULL)
    {
        if (node->next != NULL)
            deleteList (node->next);
        if (node->word != NULL)
        {
            free(node->word);
            node->word = NULL;
        }
        free(node);
        node = NULL;
    }
}

//========================================================

void deleteArray(char **arr)
{
    int i = 0;
    if (arr != NULL) 
    {
        while (arr[i] != NULL)
        {
            free(arr[i]);
            arr[i] = NULL;
            i++;
        }
        free(arr[i]);
        arr[i] = NULL;
        free(arr);
        arr = NULL;
    }
}

void deleteDoubleArray(char ***arr, int *length)
{
    int j;
    if (arr != NULL)
    {
        for (j = 0; j < *length; j++)
            deleteArray(arr[j]);
        free(arr);
        arr = NULL;
    }
}

//========================================================

List *readLine(int *not_only_enter)
{
    int c;                                                   // рассматриваемый символ  
    char control_symbols[] = "<;()&|>";                      // строка из всех управляющих символов
    char unclear_control_symbols[] = "&|>";                  // строка из управляющих символов, которые могут быть двойными
    char *control, *unclear_control;
    
    List *tmp_list = NULL;                                   // формирующийся список из слов
    int tmp_size = 0;                                        // размер добавляемого слова
    int tmp_reserve = 3;                                     // зарезервированный объем памяти под массив tmp (именно 3 для обработки управляющих слов типа &&)
    char *tmp = (char*)malloc(tmp_reserve * sizeof(char));   // добавляемое слово
    int quotes_are_open = 0;                                 // флаг: открыты ли кавычки

    while ((c = getchar()) != '\n')
    {
        *not_only_enter = 1;
        if (c == EOF) 
        {
            free(tmp);
            return NULL;
        }
        control = strchr(control_symbols, c);
        unclear_control = strchr(unclear_control_symbols, c);

        // если кавычки закрыты и считанный символ — это пробел или управляющий символ
        if (!quotes_are_open && (isspace(c) || (control != NULL))) 
        {
            if (tmp_size > 0) // добавляем слово в список только если в нём есть хотя бы один символ (исключаем добавление мусора)
            {
                tmp[tmp_size] = '\0';
                tmp_list = addWordInList(tmp_list, tmp);
                tmp_size = 0;
                free(tmp);
                tmp_reserve = 3; 
                tmp = (char*)malloc(tmp_reserve * sizeof(char));
            }
            if (control) // если мы считали не пробел, а управляющий символ, заносим его в tmp
            {
                char b;
                tmp[tmp_size] = c;
                tmp_size ++;
                if (unclear_control != NULL) // если управляющий символ может быть двойным (то есть &&, || или >>), делаем проверку, считав следующий
                {
                    if (((b = getchar()) != EOF) && (b == c))
                    {
                        tmp[tmp_size] = b;
                        tmp_size ++;                    
                    }
                    else ungetc (b, stdin); // если повторения управляющего символа нет, возвращаем символ b в поток ввода, чтобы обработать на следующей итерации
                }
                tmp[tmp_size] = '\0';
                tmp_list = addWordInList(tmp_list, tmp);
                tmp_size = 0;
                free(tmp);
                tmp_reserve = 3; 
                tmp = (char*)malloc(tmp_reserve * sizeof(char)); 
            }
        }
        else if (c == '"') // встретив кавычку, меняем её статус с 0 на 1 или наоборот
        {
            quotes_are_open = (quotes_are_open + 1) % 2;
        }
        else if (!isspace(c) || (isspace(c) && (quotes_are_open))) // если перед нами не пробел ИЛИ пробел при статусе открытых кавычек, то добавляем символ в список
        {
            tmp[tmp_size] = c;
            tmp_size ++;

            if (tmp_size == tmp_reserve) // если размер tmp достиг зарезервированной памяти, увеличиваем резерв и обновляем размер tmp
            {
                tmp_reserve *= 2;
                tmp = (char*)realloc(tmp, tmp_reserve * sizeof(char));
            }
        }
    }
    if (not_only_enter)
    {
        tmp[tmp_size] = '\0';
        tmp_list = addWordInList(tmp_list, tmp);
    }
    free(tmp);
    return tmp_list;
}

//========================================================

int is_cd(char **arr)
{
    if (!strcmp(arr[0], "cd"))
        return 1;
    else 
        return 0;
}

//========================================================

int is_exit(char **arr)
{
    if (!strcmp(arr[0], "exit"))
        return 1;
    else 
        return 0;
}

//========================================================

int is_redirection(char **arr, int *first_redir_symbol_place)
{
    int i = 0;
    while(arr[i] != NULL)
    {
        if (!strcmp(arr[i], ">>") || !strcmp(arr[i], ">") || !strcmp(arr[i], "<"))
        {
            *first_redir_symbol_place = i;
            return 1;
        }
        i++;
    }
    return 0;
}

//========================================================

int is_background(char **arr, int *background_symb_place)
{
    int i = 0;
    while (arr[i] != NULL)
    {
        if (!strcmp(arr[i], "&"))
        {
            *background_symb_place = i;
            return 1;
        }
        i++;
    }
    return 0;
}

//========================================================

int is_pipes_amps_seq(char **arr)
{
    int i = 0;
    while (arr[i] != NULL)
    {
        if(!strcmp(arr[i], "||") || !strcmp(arr[i], "&&") || !strcmp(arr[i], ";"))
            return 1;
        i++;
    }
    return 0;
}

//========================================================

char **backgroundProcessing(char **arr, int background_symb_place)
{
    free(arr[background_symb_place]);
    arr[background_symb_place] = NULL;
    return arr;
}

//========================================================

char **redirectionProcessing(char **arr, int first_redir_symbol_place)
{                                                                         
    int file;
    int i = 0;
    while (arr[i] != NULL)
    {
        if (!strcmp(arr[i], ">>"))
        {
            file = open(arr[i+1], O_CREAT | O_WRONLY | O_APPEND, 0666);
            if (file == -1)
                perror(arr[i+1]);
            dup2(file, 1);
            close(file);
        }
        else if (!strcmp(arr[i], ">"))
        {
            file = open(arr[i+1], O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (file == -1)
                perror(arr[i+1]);
            dup2(file, 1);
            close(file);        
        }
        else if (!strcmp(arr[i], "<"))
        {
            file = open(arr[i+1], O_RDONLY);
            if (file == -1)
                perror(arr[i+1]);
            dup2(file, 0);
            close(file);          
        }
        i++;            
    }

    i = first_redir_symbol_place;
    while (arr[i] != NULL)    // удаляем из массива всё, что идёт после команды, которую мы будем подавать фукнции execvp()
    {
        free(arr[i]);   
        arr[i] = NULL;
        i++;
    }
    
    return arr;
}

//========================================================

int howMuchElements(List *node)
{
	int counter = 0;
	while (node)
	{
		if (!strcmp(node->word, "|"))
		{
			counter++;
		}
		node = node->next;
	}	
	return counter + 1;
}

char **makeArrayForOneProc(List **node)
{
    int i = 0;
    char **new_arr = NULL;
    while ((*node) && strcmp((*node)->word, "|"))
    {
        new_arr = (char**)realloc(new_arr, (i + 2) * sizeof(char*));
        new_arr[i] = (char*)malloc((strlen((*node)->word) + 1) * sizeof(char));
        strcpy(new_arr[i], (*node)->word);
        i++;
        (*node) = (*node)->next;
    }
    if (*node)
        (*node) = (*node)->next;
    new_arr[i] = NULL;
    return new_arr;
}

char ***makeArrayForConveyer(List *node, int *length)
{
    char ***conveyerArray = (char***)malloc((howMuchElements(node)) * sizeof(char**));
    int counter = 0;
    while (node)
    {
        conveyerArray[counter] = makeArrayForOneProc(&node);
        counter++;
    }
    *length = counter; 
    return conveyerArray;
}

//========================================================

int howMuchElements2(char **arr)
{
	int count = 0;
    int j = 0;
	while (arr[j] != NULL)
	{
		if (!strcmp(arr[j], "||") || !strcmp(arr[j], "&&") || !strcmp(arr[j], ";"))
        {
            count++;
            j++;
        }
        else
            j++;
	}	
	return count + 1;
}

char **makeArrayForCmd(char **arr, int *place)
{
    int i = 0;
    char **new_arr = NULL;
    while ((arr[*place] != NULL) && strcmp(arr[*place], "|") && strcmp(arr[*place], "||") && strcmp(arr[*place], "&&") && strcmp(arr[*place], ";"))
    {
        new_arr = (char**)realloc(new_arr, (i + 2) * sizeof(char*));
        new_arr[i] = (char*)malloc((strlen(arr[*place]) + 1) * sizeof(char));
        strcpy(new_arr[i], arr[*place]);
        i++;
        (*place)++;
    }
    if (arr[*place] != NULL) (*place)++;

    new_arr[i] = NULL;
    return new_arr;
}

char ***makeArrayForAllCmd(char **arr, int *length)
{
    int place = 0;
    char ***AllCmdArray = (char***)malloc((howMuchElements2(arr)) * sizeof(char**));
    int counter = 0;
    while (arr[place] != NULL)
    {
        AllCmdArray[counter] = makeArrayForCmd(arr, &place);
        counter++;
    }
    *length = counter; 
    return AllCmdArray;
}

//========================================================

void cmdProcessing (char ***arr, int *length)
{
    int save0 = dup(0);
    int save1 = dup(1);
    pid_t pid;
    int first_redir_symbol_place;
    int background_symbol_place;
    int file;

    int cmd_count = 0;
    
    if (is_cd(arr[0]))
    {
        if (arr[0][1] == NULL)
            chdir(getenv("HOME"));
        else if (arr[0][2] != NULL)
            perror(arr[0][2]);
        else
            chdir(arr[0][1]);
    }
    else if (is_exit(arr[0]))
    {
        exit(0);
    }
    else
    {
        pid = fork();
        if (!pid)
        {
            int i = 0;
            int fd[2];
            int background_cmd = is_background(arr[i], &background_symbol_place);
            while (i < *length)
            {   
                pipe(fd);
                pid = fork();
                if(!pid)
                {
                    if (!is_pipes_amps_seq(arr[i]) && is_redirection(arr[i], &first_redir_symbol_place))
                    {
                        arr[i] = redirectionProcessing(arr[i], first_redir_symbol_place);
                    }

                    if (background_cmd)
                    {
                        arr[i] = backgroundProcessing(arr[i], background_symbol_place);
                        file = open("dev/null", O_RDONLY);
                        if (file == -1)
                            perror("file didn't open");
                        dup2(file, 0);
                        close(file);
                        signal(SIGINT, SIG_IGN);
                    }
                   
                    if (is_pipes_amps_seq(arr[i]))    // если это команда ||, && или ; то формируем новый двумерный массив с командами под текущий пайп
                    { 
                        int done = 0;
                        char ***cmd_arr = NULL;
                        cmd_arr = makeArrayForAllCmd(arr[i], &cmd_count);
                        int j = 0;
                        int cmd_num = 0;

                        if (is_redirection(cmd_arr[cmd_num], &first_redir_symbol_place))
                        {
                            cmd_arr[cmd_num] = redirectionProcessing(cmd_arr[cmd_num], first_redir_symbol_place);
                        }

                        pid_t pid1;
                        if ((pid1 = fork()) == -1)
                        {
                            perror("fork call");
                            exit(1);
                        }
                        if (pid1 == 0)
                        {
                            execvp(cmd_arr[cmd_num][0], cmd_arr[cmd_num]);
                            perror(cmd_arr[cmd_num][0]);
                            exit(2);
                        }
                        int status1;
                        wait(&status1);

                        while (arr[i][j] != NULL)
                        {
                            if (is_redirection(cmd_arr[cmd_num+1], &first_redir_symbol_place))
                            {
                                cmd_arr[cmd_num+1] = redirectionProcessing(cmd_arr[cmd_num+1], first_redir_symbol_place);
                            }
                            if (!strcmp(arr[i][j], "||"))
                            {
                                if(!WIFEXITED(status1) || WEXITSTATUS(status1))
                                {
                                    if ((pid1 = fork()) == -1)
                                    {
                                        perror("fork call");
                                        exit(3);
                                    }
                                    if (pid1 == 0)
                                    {
                                        if (done == (cmd_count-2))
                                        {
                                            if (i+1 != *length)
                                                dup2(fd[1], 1);
                                            close(fd[0]);
                                            close(fd[1]);                                
                                        }
                                        execvp(cmd_arr[cmd_num+1][0], cmd_arr[cmd_num+1]);
                                        perror(cmd_arr[cmd_num+1][0]);
                                        exit(4);                                    
                                    }
                                    wait(&status1);                               
                                }
                                done++;
                                cmd_num++;
                            }
                            else if (!strcmp(arr[i][j], "&&"))
                            {
                                if(WIFEXITED(status1) && !WEXITSTATUS(status1))
                                {
                                    if ((pid1 = fork()) == -1)
                                    {
                                        perror("fork call");
                                        exit(5);
                                    }
                                    if (pid1 == 0)
                                    {
                                        if (done == (cmd_count-2))
                                        {
                                            if (i+1 != *length)
                                                dup2(fd[1], 1);
                                            close(fd[0]);
                                            close(fd[1]);                                
                                        }
                                        execvp(cmd_arr[cmd_num+1][0], cmd_arr[cmd_num+1]);
                                        perror(cmd_arr[cmd_num+1][0]);
                                        exit(6);                                    
                                    }
                                    wait(&status1);                               
                                }
                                done++;
                                cmd_num++;
                            }
                            else if (!strcmp(arr[i][j], ";"))
                            {
                                if ((pid1 = fork()) == -1)
                                {
                                    perror("fork call");
                                    exit(7);
                                }
                                if (pid1 == 0)
                                {
                                    if (done == (cmd_count-2))
                                    {
                                        if (i+1 != *length)
                                            dup2(fd[1], 1);
                                        close(fd[0]);
                                        close(fd[1]);                                
                                    }
                                    execvp(cmd_arr[cmd_num+1][0], cmd_arr[cmd_num+1]);
                                    perror(cmd_arr[cmd_num+1][0]);
                                    exit(8);                                    
                                }
                                wait(&status1); 
                                cmd_num++;
                                done++;
                            }
                            j++;
                        }
                        deleteDoubleArray(cmd_arr, &cmd_count);
                    }
                    else    // иначе перед нами стандартная команда в пайпе (то есть без всяких &&, ||, ;)
                    {
                        if (i+1 != *length)
                            dup2(fd[1], 1);
                        close(fd[0]);
                        close(fd[1]);
                        execvp(arr[i][0], arr[i]);
                        perror(arr[i][0]);
                        exit(1);
                    }
                }
                if (background_cmd)
                    printf("background child pid: [%d]\n", pid);
                dup2(fd[0], 0);
                close(fd[0]);
                close(fd[1]);
                i++;
            }
            if (!background_cmd)
                while (wait(NULL) != -1);
        }
        else
        {
            int status;
            wait(&status);
        }
    }
    dup2(save0, 0);
    dup2(save1, 1);
    close(save0);
    close(save1);
}

//========================================================

void zombieRemove(int s)
{
    int status;
	int pid;
    pid = waitpid(-1, &status, WNOHANG);   
    while (pid > 0)
    {
        pid = waitpid(-1, &status, WNOHANG);

		if (WIFEXITED(status))    // WIFEXITED - дочерний процесс успешно завершился
		{
			printf("Process [%d] exited, status %d\n", pid, WEXITSTATUS(status));    // WEXITSTATUS возвращает восемь младших битов значения, которое вернул завершившийся дочерний процесс
		}
		else if (WIFSIGNALED(status))    // WIFSIGNALED - дочерний процесс завершился из-за необработанного сигнала
		{
			printf("Process [%d] killed by signal %d\n", pid, WTERMSIG(status));     // WTERMSIG возвращает номер сигнала, который привел к завершению дочернего процесса
		}
		else if (WIFSTOPPED(status))    // WIFSTOPPED - дочерний процесс, из-за которого функция вернула управление, в настоящий момент остановлен
		{
			printf("Process [%d] stopped by signal %d\n", pid, WSTOPSIG(status));    // WSTOPSIG возвращает номер сигнала, из-за которого дочерний процесс был остановлен
		} 
    } 
}

//========================================================