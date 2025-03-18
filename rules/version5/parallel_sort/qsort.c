#include <stdio.h>
#include <stdlib.h>

void
swap(char c, int *a, int *b)
{
	printf("%c %d\t%d\n", c, *a, *b);
	int t = *a;
	*a = *b;
	*b = t;
}

int
partition(int array[], int low, int high)
{
	int pivot = array[high];
	int i = (low - 1);

	for (int j = low; j < high; j++)
	{
		if (array[j] <= pivot)
		{	i++;
			swap('I', &array[i], &array[j]);
		}
	}
	swap('A', &array[i + 1], &array[high]);
  
	return (i + 1);
}

void
quickSort(int array[], int low, int high)
{
	if (low < high)
	{	int pi = partition(array, low, high);
		quickSort(array, low, pi - 1);
		quickSort(array, pi + 1, high);
	}
}

void
printArray(int array[], int size)
{
	for (int i = 0; i < size; ++i)
	{	printf("%d  ", array[i]);
	}
	printf("\n");
}

int
main(void)
{	int data[100];
	int i, n;

	for (i = 0; i < 100; i++)
	{	data[i] = 100-i;
	}
	n = sizeof(data) / sizeof(data[0]);
  
	quickSort(data, 0, n - 1);
  
	printf("Sorted array in ascending order: \n");
	printArray(data, n);
}
