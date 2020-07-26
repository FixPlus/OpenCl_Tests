

// I used alternative representation of algorithm given here https://en.wikipedia.org/wiki/Bitonic_sorter
 	

void sort(__global int* arr, int i, int j, bool up){
	unsigned int id = get_global_id(0);
	unsigned int id1, id2;

	unsigned int dif = (unsigned int)(i - j);
	
	unsigned int group = id >> dif;
	unsigned int in_group = id & ~(group << dif);
	
	id1 = group * (2u << dif) + in_group;
	
	if(j == 0) 
		id2 = (group + 1u) * (2u << dif) - in_group - 1;
	else
		id2 = id1 + (1u << dif); 

	
	bool cmp = arr[id1] > arr[id2];
	
	if(!up) cmp = !cmp;

	if(cmp){
		int temp = arr[id1];
		arr[id1] = arr[id2];
		arr[id2] = temp;

	}

}

__kernel void sortUp(__global int* arr, int i, int j){
	sort(arr, i, j, true);
}

__kernel void sortDown(__global int* arr, int i, int j){
	sort(arr, i, j, false);
}