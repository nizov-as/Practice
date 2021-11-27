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
void deleteArray(char **arr);
void deleteDoubleArray(char ***arr, int *length);

// чтение одной строки и формирование списка
List *readLine(int *not_only_enter);

// формирование массива из списка
char **listToArray(List *node, int *length);

// проверки на конкретные команды:
int is_cd(char **arr);                                                // на команду cd
int is_exit(char **arr);                                              // на команду exit
int is_redirection(char **arr, int *file_name_index, int *length);    // на команду перенаправления ввода/вывода
int is_conveyer(char **arr, int *length);                             // на то, имеется ли в строке конвейерный символ

char **makeArrayForOneProc (List **node);
char ***makeArrayForConveyer (List **node, int *length);
int howMuchElements (List *node);

// обработка текущей команды
void cmdProcessing (char ***arr, int *length);

//========================================================
//========================================================

int main (int argc, char *argv[])
{   
    int not_only_enter = 0;
    int arr_length;
    List *tmp_list = NULL;      
    printf("alexandernizov$ ");

    tmp_list = readLine(&not_only_enter); 

    while (tmp_list != NULL)
    {
        char ***array = NULL;
        array = makeArrayForConveyer(&tmp_list, &arr_length);
        printf("arr length: %d\n", arr_length);

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
		printf("__%s__ ", arr[j]);
        j++;
	}
    printf("\n");
}

void printDoubleArr(char ***arr, int *length)
{   
    int j;
	for (j = 0; j < *length; j++)
        printArr(arr[j]);

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
	for (j = 0; j < *length; j++)
        deleteArray(arr[j]);
    free(arr);
    arr = NULL;
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

char **listToArray(List *node, int *length)
{
    int i = 0;
    char **arr = NULL;
    while (node)
    {
        arr = (char**)realloc(arr, (i + 2) * sizeof(char*));
        arr[i] = (char*)malloc((strlen(node->word) + 1) * sizeof(char));
        strcpy(arr[i], node->word);
        i++;
        node = node->next;
    }
    *length = i;
    arr[i] = NULL;
    return arr;
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

int is_redirection(char **arr, int *file_name_index, int *length)
{
    int i;
    for(i = 0; i < *length; i++)
    {
        if (!strcmp(arr[i], ">>"))
        {
            *file_name_index = i+1;
            return 1;
        }
        else if (!strcmp(arr[i], ">"))
        {
            *file_name_index = i+1;
            return 2;
        }
        else if (!strcmp(arr[i], "<"))
        {
            *file_name_index = i+1;
            return 3;
        }
    }
    return 0;
}

//========================================================

int is_conveyer(char **arr, int *length)
{
    int i;
    for (i = 0; i < *length; i++)
    {
        if (!strcmp(arr[i], "|"))
            return 1;
    }
    return 0;
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
    if(*node) (*node) = (*node)->next;
    new_arr[i] = NULL;
    return new_arr;
}

//========================================================

char ***makeArrayForConveyer(List **node, int *length)
{
    char ***conveyerArray = (char***)malloc(sizeof(char**) * (howMuchElements(*node)));
    int counter = 0;
    while (*node)
    {
        conveyerArray[counter] = makeArrayForOneProc(node);
        counter++;
    }
    *length = counter;
    return conveyerArray;
}

//========================================================

void cmdProcessing (char ***arr, int *length)
{
    // int file;
    // int file_name_index;
    // int is_redir = is_redirection(arr, &file_name_index, length);
    int save0 = dup(0);
    int save1 = dup(1);
    int pid;
    
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
            // printf("length val is: %d\n", *length);
            printDoubleArr(arr, length);
            int i = 0;
            int fd[2];
            while (i < *length)
            {
                pipe(fd);
                pid = fork();
                if(!pid)
                {
                    if (i+1 != *length)
                        dup2(fd[1], 1);
                    close(fd[0]);
                    close(fd[1]);
                    execvp(arr[i][0], arr[i]);
                    perror(arr[i][0]);
                    exit(1);
                }
                dup2(fd[0], 0);
                close(fd[0]);
                close(fd[1]);
                i++;
            }
            while (wait(NULL) != -1);
        }
        else
        {
            int status, wr;
            wr = wait(&status);
            if (wr == -1)
                printf ("There are no child processes");
        }
    }
    dup2(save0, 0);
    dup2(save1, 1);
    close(save0);
    close(save1);
}
//========================================================