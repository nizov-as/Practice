#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct _List 
{
    char *word;             // указатель на слово в звене
    struct _List *next;     // указатель на следующий элемент звена
} List;

// добавление слова в список
List *addWordInList (List *node, char *current_word);

// печать списка
void printList(List *node);

// очистка памяти: удаление списка
void deleteList (List *node);

// очистка памяти: удаление массива
void deleteArray(char **arr);

// чтение одной строки и формирование списка
List *readLine(FILE *file_in, int *not_only_enter);

// формирование массива из списка
char **listToArray(List *node);

// проверка, является ли текущая команда командой cd
int is_cd(char **arr);

// обработка текущей команды
void cmd_processing (char **array);

//========================================================
//========================================================

int main (int argc, char *argv[])
{
    FILE *file_in;
    file_in = stdin;    

    int not_only_enter = 0;
    List *tmp_list = NULL;    
    printf("alexandernizov$ ");
    tmp_list = readLine(file_in, &not_only_enter); 

    while (tmp_list != NULL)
    {
        char **array = NULL;
        array = listToArray(tmp_list);
        if(not_only_enter)
        {
            int pid = fork();
            if(!pid)
                cmd_processing(array);
            else
            {
                int status, wr;
                wr = wait(&status);
                if (wr == -1)
                    printf ("There are no child processes");
            }
        }
        deleteList(tmp_list);
        deleteArray(array);
        printf("alexandernizov$ ");
        tmp_list = readLine(file_in, &not_only_enter);
    }
    
    deleteList(tmp_list);
    free(tmp_list);
    fclose(file_in);
    
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

//========================================================

List *readLine(FILE *file_in, int *not_only_enter)
{
    int c;                                                  // рассматриваемый символ  
    char control_symbols[] = "<;()&|>";                      // строка из всех управляющих символов
    char unclear_control_symbols[] = "&|>";                  // строка из управляющих символов, которые могут быть двойными
    char *control, *unclear_control;
    
    List *tmp_list = NULL;                                   // формирующийся список из слов
    int tmp_size = 0;                                        // размер добавляемого слова
    int tmp_reserve = 3;                                     // зарезервированный объем памяти под массив tmp (именно 3 для обработки управляющих слов типа &&)
    char *tmp = (char*)malloc(tmp_reserve * sizeof(char));   // добавляемое слово
    int quotes_are_open = 0;                                 // флаг: открыты ли кавычки

    while ((c = getc(file_in)) != '\n')
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
                    if (((b = getc(file_in)) != EOF) && (b == c))
                    {
                        tmp[tmp_size] = b;
                        tmp_size ++;                    
                    }
                    else ungetc (b, file_in); // если повторения управляющего символа нет, возвращаем символ b в поток ввода, чтобы обработать на следующей итерации
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

char **listToArray(List *node)
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

void cmd_processing (char **array)
{
    if (is_cd(array))
    {
        if (array[1] == NULL)
            chdir(getenv("HOME"));
        else if (array[2] != NULL)
            perror(array[2]);
        else
            chdir(array[1]);
    }
    else
    {
        execvp(array[0], array);
        perror(array[0]);
        exit(1);
    }
}