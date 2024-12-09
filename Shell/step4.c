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
List *addWordInList (List *node, char *current_word);

// печати:
void printList(List *node);                       // печать списка
void printArr(char** arr);                        // печать массива
void printDoubleArr(char ***arr, int *length);    // печать двумерного массива

// очистка памяти: удаление списка
void deleteList (List *node);

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

//функции для создания двумерного массива:
int howMuchElements (List *node);                          // подсчёт числа элементов списка (для корректного выделения памяти)
char **makeArrayForOneProc (List **node);                  // создание одномерного массива
char ***makeArrayForConveyer (List *node, int *length);    // создание массива из одномерных массивов (то есть двумерного массива)

// обработка текущей команды
void cmdProcessing (char ***arr, int *length);

//обработки команд:
char **redirectionProcessing(char **arr, int first_redir_symbol_place);    // перенаправление ввода-вывода
char **backgroundProcessing(char **arr, int background_symb_place);        // фоновый режим

void zombieRemove();

//========================================================
//========================================================

int main (int argc, char *argv[])
{   
    int not_only_enter = 0;
    int arr_length;    // количество команд в конвейере
    List *tmp_list = NULL;      
    printf("alexandernizov$ ");

    tmp_list = readLine(&not_only_enter); 
    
    signal (SIGCHLD, zombieRemove);
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

//========================================================

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

//========================================================

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

void cmdProcessing (char ***arr, int *length)
{
    int save0 = dup(0);
    int save1 = dup(1);
    int pid;
    int first_redir_symbol_place;
    int background_symbol_place;
    int file;
    
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
        int i = 0;
        int fd[2];
        int background_cmd = is_background(arr[i], &background_symbol_place);
        while (i < *length)
        {   
            pipe(fd);
            pid = fork();
            //printf("BGD: %d\n", background_cmd);
            if(!pid)
            {
                if (is_redirection(arr[i], &first_redir_symbol_place))
                {
                    arr[i] = redirectionProcessing(arr[i], first_redir_symbol_place);
                }
                if (background_cmd)
                {
                    arr[i] = backgroundProcessing(arr[i], background_symbol_place);
                    file = open ("/dev/null", O_RDONLY);    
                    if (file == -1)
                        perror("file didn't open\n");
                    dup2(file, 0);    // перенаправляем стандартный ввод на файл "/dev/null", чтение из которого сразу дает EOF 
                    close(file);   
                    signal (SIGINT, SIG_IGN);    // устанавливаем игнорирование сигнала SIGINT (то есть Control+C)
                }

                if (i+1 != *length)
                    dup2(fd[1], 1);
                close(fd[0]);
                close(fd[1]);
                execvp(arr[i][0], arr[i]);
                perror(arr[i][0]);
                exit(1);
            }
            if (background_cmd)
                printf("background cmd child pid: [%d]\n", pid);
            dup2(fd[0], 0);
            close(fd[0]);
            close(fd[1]);
            i++;
        }
        if (!background_cmd)
            while (wait(NULL) != -1);
    }
    dup2(save0, 0);
    dup2(save1, 1);
    close(save0);
    close(save1);
}

//========================================================

void zombieRemove()
{
    int status;
	int pid;
    pid = waitpid(-1, &status, WNOHANG);   
    while (pid > 0)
    {
        if (WIFEXITED(status)) printf ("Command with pid [%d] is done, status: %d\n", pid, status);
        else printf ("Command with pid [%d] is done, status: error\n", pid);
        pid = waitpid(-1, &status, WNOHANG);    // WNOHANG: если ни одного процесса-зомби нет, вызов ничего не ждет и цикл прерывается (возвращает 0)
    }
}

//========================================================
