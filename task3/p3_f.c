#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct _BinTree
{
	char *word;               // указатель на слово в бинарном дереве
	unsigned int entryCount;  // счетчик вхождений слова в тексте
	struct _BinTree *left;
	struct _BinTree *right;
} BinTree;

// добавление слова в бинарное дерево (или увеличение счетчика вхождений)
BinTree *addWordInTree(BinTree *nodePtr, char *word, int w_size, int *max_entry_num);

// процедура печати всего дерева
void printBinTree(BinTree *nodePtr, int i, double frequency, FILE *file_out);

// печать словаря
void printDict(BinTree *nodePtr, int *max_entry_num, int total_amount, FILE *file_out);

// очистка памяти (удаление всего дерева)
void deleteBinTree(BinTree *nodePtr);

int main (int argc, char *argv[])
{
    int flag_i = 0;     // флаг на наличие аргумента -i
    int flag_o = 0;     // флаг на наличие аргумента -o
    int out_place; 
    int in_place;         

    FILE *file_in;
    FILE *file_out;

    // проверка наличия аргументов -i и -o
    for (int i = 1; i < argc; i++)
	{
		if (!strcmp(argv[i], "-i")) 
        {
            flag_i = 1;
            in_place = i + 1;
        }
		if (!strcmp(argv[i], "-o")) 
		{
			flag_o = 1;
			out_place = i + 1; // в переменной сохраняем место с названием файла для записи
        }
    }

    // инициализация ввода
    if (flag_i) 
	{
		// открываем файл, указанный вторым аргументом, значение mode — файл для чтения
        if ((file_in = fopen(argv[in_place], "r")) == NULL) 
		{ 
            printf("File '%s' is not found\n", argv[in_place]);
            return 1;
        }
        printf("Input status: Input from file '%s':\n", argv[in_place]);
    }
	else 
	{
        file_in = stdin; // если опция -i не указана, то считываем со стандартного ввода
        printf("Input status: input from stdin\n");
    }

    // инициализация вывода
    if (flag_o)
	{
		// открываем файл, указанный после аргумента -o, значение mode — файл для записи
        if ((file_out = fopen(argv[out_place], "w")) == NULL) 
		{ 
            printf("File '%s' is not found\n", argv[out_place]);
            return 2;
        }
        printf("Output status: Output from file '%s':\n", argv[out_place]);
    }
	else
	{
        file_out = stdout; // если опция -o не указана, то выводим через стандартный вывод
        printf("Output status: output from stdout\n");
    }

    // реализация процесса добавления элементов в дерево
    BinTree *tree = NULL;
    int total_amount = 0;                           // общее количество слов в тексте
    int max_entry_num = 0;                          // максимальное количество вхождений слова в дереве
    char *tmp = (char*)malloc(2 * sizeof(char));    // добавляемое слово
    int tmp_numb = 0;                               // количество символов в массиве tmp
    char c;

    while ((c = getc(file_in)) != EOF)
    {
        while (isalpha(c)) // пока символ — буква любого регистра, добавляем ее в массив tmp
		{
            tmp[tmp_numb] = c;
            tmp_numb++;
            tmp = (char*)realloc(tmp, tmp_numb + 1);
            c = getc(file_in);
        }
		if (tmp_numb) // если мы, считав непустое слово, дошли до спец. символа
		{
            // то добавляем это слово в дерево
            tmp[tmp_numb] = 0; // нулевой бит - признак конца строки
            tree = addWordInTree(tree, tmp, tmp_numb, &max_entry_num);
            total_amount++;
            free(tmp);
            tmp_numb = 0;
            tmp = (char*)malloc(2 * sizeof(char));
        }
        // добавляем слова - знаки препинания
        if (!isalpha(c) && (c != '\n') && !isspace(c)) 
		{
            tmp[tmp_numb] = c;
            tmp_numb++;
            tmp[tmp_numb] = 0;
            tree = addWordInTree(tree, tmp, tmp_numb, &max_entry_num);
            total_amount++; 
            free(tmp);
            tmp_numb = 0; 
            tmp = (char*)malloc(2 * sizeof(char));  
        }
    }
    if (tmp != NULL) free(tmp);

    printDict(tree, &max_entry_num, total_amount, file_out);

    fclose(file_in);
    fclose(file_out);

    deleteBinTree(tree);
    return 0;
}

//==========================================

BinTree *addWordInTree(BinTree *nodePtr, char *word, int w_size, int *max_entry_num)
{
    int word_status;
	if (!nodePtr) //если перед нами пустое дерево, выделяем память и вставляем все данные
	{
		nodePtr = (BinTree*)malloc(sizeof(BinTree));
		nodePtr->word = (char*)malloc((w_size + 1) * sizeof(char));
		nodePtr->entryCount = 1;
		nodePtr->right = nodePtr->left = NULL;
        strcpy(nodePtr->word, word);
	}
    // если непустое и рассматриваемое слово совпадает со словом в дереве, увеличиваем счётчик и переопределяем максимальное число вхождений
	else if ((word_status = strcmp(nodePtr->word, word)) == 0) 
    {
        nodePtr->entryCount++;
        if ((nodePtr->entryCount) > *max_entry_num)
            *max_entry_num = (nodePtr->entryCount);
    }
    else if (word_status > 0) // если слово в узле больше добавляемого, ищем место добавляемому в левом поддереве
        nodePtr->left = addWordInTree(nodePtr->left, word, w_size, max_entry_num);
	else // обратная ситуация
        nodePtr->right = addWordInTree(nodePtr->right, word, w_size, max_entry_num);

    return nodePtr;
}

//==========================================

void printBinTree(BinTree *nodePtr, int i, double frequency, FILE *file_out)
{
    if (nodePtr)
    {
        printBinTree(nodePtr->left, i, frequency, file_out);
        if (i == nodePtr->entryCount)
            fprintf(file_out, "%s %d %f\n", nodePtr->word, i, frequency);
        printBinTree(nodePtr->right, i, frequency, file_out);
    }
}

//==========================================

void printDict(BinTree *nodePtr, int *max_entry_num, int total_amount, FILE *file_out)
{
    int i;
    for (i = (*max_entry_num); i > 0; i--) // идя от максимального вхождения, печатаем дерево, вычисляя частоту вхождения
        printBinTree(nodePtr, i, (double)i / total_amount, file_out);
}

//==========================================

void deleteBinTree(BinTree *nodePtr) 
{
	if (!nodePtr) return;

    deleteBinTree(nodePtr->left);
    deleteBinTree(nodePtr->right);
    free(nodePtr->word);
    free(nodePtr);
}

//==========================================