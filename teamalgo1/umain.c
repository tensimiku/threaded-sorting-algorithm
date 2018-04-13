#include<stdio.h>
#include<string.h>
#include <time.h>
#include <windows.h> //I've ommited this line.

#define MALLOCSIZE 1024
#define NTHREADS 16

int *orig, *temparr;

void insertion(int low, int high) {
	int tmp;
	int i, j;
	for (i = low + 1; i <= high; i++) {
		tmp = orig[i];
		for (j = i - 1; j >= low && orig[j] > tmp; j--) {
			orig[j + 1] = orig[j];
		}
		orig[j + 1] = tmp;
	}
}

void l_merge(int low, int mid, int high) {
	int i, j, k;
	i = k = low; j = mid + 1;
	while (k <= mid && j <= high) {
		if (orig[i] < orig[j]) {
			temparr[k] = orig[i];
			i++;
		}
		else {
			temparr[k] = orig[j];
			j++;
		}
		k++;
	}
	while (k <= mid) {
		temparr[k] = orig[i];
		k++; i++;
	}
}
void r_merge(int low, int mid, int high) {
	int i, j, k;
	i = mid;
	j = k = high;
	while (i >= low && k > mid) {
		if (orig[i] > orig[j]) {
			temparr[k] = orig[i];
			i--;
		}
		else {
			temparr[k] = orig[j];
			j--;
		}
		k--;
	}
	while (k > mid) {
		temparr[k] = orig[j];
		k--; j--;
	}
}

void merge(int low, int mid, int high) {
	int i, j, k;
	i = k = low; j = mid + 1;
	while (i <= mid && j <= high) {
		if (orig[i] < orig[j]) {
			temparr[k] = orig[i];
			i++;
		}
		else {
			temparr[k] = orig[j];
			j++;
		}
		k++;
	}
	if (i > mid) {
		//memcpy(temparr+k, orig+j, sizeof(int)*(high-j+1));
		for (i = low; i<j; i++) {
			orig[i] = temparr[i];
		}
		return;
	}
	else {
		//memcpy(temparr+k, orig+i, sizeof(int)*(mid-i+1));
		while (i <= mid) {
			temparr[k] = orig[i];
			i++; k++;
		}
		memcpy(orig + low, temparr + low, sizeof(int)*(high - low + 1));
	}
	/**
	for (i = low; i <= high ; i++) {
	orig[i] = temparr[i];
	}**/
}

void partition(int low, int high, int* pivot) {
	int i, j = low;
	int pitem = orig[low];
	int temp;
	for (i = low + 1; i <= high; i++) {
		if (orig[i] < pitem) {
			j++;
			temp = orig[i];
			orig[i] = orig[j];
			orig[j] = temp;
		}
	}
	*pivot = j;
	temp = orig[low];
	orig[low] = orig[j];
	orig[j] = temp;
}

void quicksort(int low, int high) {
	int pivot;
	if (low < high) {
		if ((high - low) < 32) {
			insertion(low, high);
			return;
		}
		partition(low, high, &pivot);
		quicksort(low, pivot - 1);
		quicksort(pivot + 1, high);
	}
}

void mergesort(int low, int high) {
	int mid;
	if (low < high) {
		if ((high - low) < 32) {
			insertion(low, high);
			return;
		}
		mid = (low + high) / 2;
		mergesort(low, mid);
		mergesort(mid + 1, high);
		merge(low, mid, high);
	}
}
struct margs {
	int low;
	int mid;
	int high;
};


DWORD WINAPI r_merge_wrapper(LPVOID *args) {
	struct margs *temp = (struct margs*)args;
	r_merge(temp->low, temp->mid, temp->high);
	return 0;
}

DWORD WINAPI memcpy_wrapper(LPVOID *args) {
	struct margs *temp = (struct margs*)args;
	memcpy(orig + temp->low, temparr + temp->low, sizeof(int)*(temp->mid - temp->low + 1));
	return 0;
}

DWORD WINAPI lrmerge_wrapper(LPVOID *args) {
	struct margs *temp = (struct margs*)args;
	HANDLE pt;
	DWORD ptid;
	if ((pt = CreateThread(NULL, 0, r_merge_wrapper, args, 0, &ptid)) == NULL) {
		printf("not graceful spawning... exit program.. XqX\n");
		return 1;
	}
	l_merge(temp->low, temp->mid, temp->high);
	if (WaitForSingleObject(pt, INFINITE) != 0) {
		printf("not graceful joining threads... exit program.. XpX\n");
		return 1;
	}
	if ((pt = CreateThread(NULL, 0, memcpy_wrapper, args, 0, &ptid)) == NULL) {
		printf("not graceful spawning... exit program.. XqX\n");
		return 1;
	}
	memcpy(orig + temp->mid + 1, temparr + temp->mid + 1, sizeof(int)*(temp->high - temp->mid));
	if (WaitForSingleObject(pt, INFINITE) != 0) {
		printf("not graceful joining threads... exit program.. XpX\n");
		return 1;
	}
	return 0;
}

DWORD WINAPI quicksort_wrapper(LPVOID args) {
	struct margs *temp = (struct margs*)args;
	quicksort(temp->low, temp->high);
	return 0;
}

DWORD WINAPI mergesort_wrapper(LPVOID args) {
	struct margs *temp = (struct margs*)args;
	mergesort(temp->low, temp->high);
	return 0;
}
void* merge_wrapper(void *args) {
	struct margs *temp = args;
	merge(temp->low, temp->mid, temp->high);
}

int main(int argc, char* argv[]) {
	int i = 0;
	int x;
	int count = MALLOCSIZE;
	int readval;
	//v2 added code
	int nlist = NTHREADS;
	DWORD ptid[NTHREADS];
	HANDLE pt[NTHREADS];

	struct margs targs[NTHREADS];
	ULONGLONG sttime;
	ULONGLONG endtime;
	if (argc < 2) return 0;
	printf("opening file: %s\n", argv[1]);
	FILE* fp = fopen(argv[1], "r");
	FILE* outfp = fopen("sorted", "w+");
	if (fp == NULL || outfp == NULL) {
		printf("err on open file\n");
		return 0;
	}
	orig = (int*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(int)*count);
	printf("--------merge-sort-team-hw1-cs14375--------\n");
	while (fscanf(fp, "%d", &readval) != -1) {
		if (i == count) {
			count *= 2;
			orig = (int*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, orig, sizeof(int)*count);
		}
		orig[i] = readval;
		i++;
		if (i % 10000 == 0)printf("readed %d items..\n", i);
	}
	printf("file load complete. current count, size: %d, %d\n", count, i);
	temparr = (int*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(int)*count);

	//v2 added code
	targs[0].low = 0;
	targs[0].high = i / NTHREADS;
	for (x = 1; x<NTHREADS; x++) {
		targs[x].low = targs[x - 1].high + 1;
		targs[x].high = i * (x + 1) / NTHREADS - 1;
	}

	printf("----graceful-spawning-threads-for-merge----\n");
	sttime = GetTickCount64();

	for (x = 0; x<NTHREADS; x++) {
		if ((pt[x] = CreateThread(NULL, 0, mergesort_wrapper, &targs[x], 0, &ptid[x])) == NULL) {
			printf("not graceful spawning... exit program.. XqX\n");
			return -1;
		}
	}
	for (x = 0; x<NTHREADS; x++) {
		if (WaitForSingleObject(pt[x], INFINITE) != 0) {
			printf("not graceful joining threads... exit program.. XpX\n");
			return -1;
		}
	}

	//now merge it!
	while (nlist > 1) {
		nlist = nlist / 2;
		for (x = 0; x<nlist; x++) {
			targs[x].low = targs[x * 2].low;
			targs[x].mid = targs[x * 2].high;
			targs[x].high = targs[x * 2 + 1].high;
		}
		for (x = 0; x<nlist; x++) {
			if ((pt[x] = CreateThread(NULL, 0, lrmerge_wrapper, &targs[x], 0, &ptid[x])) == NULL) {
				printf("not graceful spawning... exit program.. XqX\n");
				return -1;
			}
		}
		for (x = 0; x<nlist; x++) {
			if (WaitForSingleObject(pt[x], INFINITE) != 0) {
				printf("not graceful joining threads... exit program.. XpX\n");
				return -1;
			}
		}
	}
	//quicksort(0, i-1);
	endtime = GetTickCount64();
	int delta = endtime - sttime;
	printf("sorting finished with %d.%03d second.\n", delta / 1000, delta % 1000);
	printf("writing to file....\n");
	for (x = 0; x<i; x++) {
		fprintf(outfp, "%10d ", orig[x]);
		if (x % 5 == 4) fprintf(outfp, "\n");
	}
	printf("finished!\n");

}
