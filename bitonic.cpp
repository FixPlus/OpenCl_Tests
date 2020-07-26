

#include "MyFrameCL.hpp"
#include <cstdlib>
#include <chrono>

constexpr size_t VEC_SIZE = 1u << 20;

enum SortDir{SD_UP, SD_DOWN};


void requireSorted(std::vector<int> arr, SortDir sortDir){
	for(auto it = arr.begin(); it + 1 < arr.end(); it++)
		if((*it > *(it + 1) && sortDir == SD_UP) || (*it < *(it + 1) && sortDir == SD_DOWN))
			throw(std::logic_error{"Array is not sorted properly"});
}

void ref_kernel(std::vector<int>& arr, SortDir sortDir, int i, int j, int range){

	for(int id = 0; id < range; id++){
		unsigned int id1, id2;

		unsigned int dif = (unsigned int)(i - j);
		
		unsigned int group = id >> dif;
		unsigned int in_group = id & ~(group << dif);
		
		id1 = group * (2u << dif) + in_group;
		
		if(j == 0) 
			id2 = (group + 1u) * (2u << dif) - in_group - 1;
		else
			id2 = id1 + (1u << dif); 

		bool cmp = arr[id1] < arr[id2];
		if(sortDir == SD_UP) cmp = !cmp;

		if(cmp){
			int temp = arr[id1];
			arr[id1] = arr[id2];
			arr[id2] = temp;

		}
	}
}

enum ExecPlatform{EP_HOST, EP_OCL};

void bitonic_sort(myfcl::Context const& context, std::vector<int>& array, SortDir sortDir = SD_UP, ExecPlatform platform = EP_OCL) {
	unsigned int N = array.size();

	if(N <= 1)
		return;

	unsigned int logN = 0;
	while(N != 1) N = N >> 1u, logN++;

	if(1u << logN != array.size())
		throw(std::logic_error("Array size must be presisely 2^N"));

	N = array.size();

	if(platform == EP_OCL){
		myfcl::Buffer<int> buf{context, &array};


		myfcl::Program prog{context, "bitonic_sort.cl"};
		
		std::string kerName = sortDir == SD_UP ? "sortUp" : "sortDown";
		
		myfcl::Kernel sort{prog, kerName.c_str()};

		myfcl::Queue queue{context};
		
		cl_int i = 0,j = 0;

		sort.addArgument(0, &buf.buffer());
		sort.addArgument(1, &i);
		sort.addArgument(2, &j);

		queue.addTask(new myfcl::Write{buf});

		for(i = 0; i < logN; i++)
			for(j = 0; j <= i; j++){
				sort.addArgument(1, &i);
				sort.addArgument(2, &j);
				queue.addTask(new myfcl::Execute{sort,{8}, {N / 2}});
				queue.execute();
			}
		
		queue.addTask(new myfcl::Read{buf});
		queue.execute();

	}
	else{
		for(int i = 0; i < logN; i++)
			for(int j = 0; j <= i; j++){
				ref_kernel(array, sortDir, i, j, N / 2);
			}

	}
}


int main(){

	try{
		myfcl::Context context;
		
		std::vector<int> arr(VEC_SIZE);
		std::vector<int> arr2(VEC_SIZE);

		for(int i = 0; i < VEC_SIZE; i++){
			arr[i] = rand() % 10000;
		}

		std::copy(arr.begin(), arr.end(), arr2.begin());
		



		std::cout << "Sorting array of " << arr.size() << " elements using GPU..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();

		bitonic_sort(context, arr, SD_UP, EP_OCL);
		
		auto finish = std::chrono::high_resolution_clock::now();
		
		std::chrono::duration<double> fs = finish - start;

		std::cout << "Done!" << std::endl << fs.count() << " seconds elapsed" << std::endl;

		std::cout << "Sorting array of " << arr.size() << " elements using CPU..." << std::endl;

		start = std::chrono::high_resolution_clock::now();

		bitonic_sort(context, arr2, SD_UP, EP_HOST);
		
		finish = std::chrono::high_resolution_clock::now();
		
		fs = finish - start;

		std::cout << "Done!" << std::endl << fs.count() << " seconds elapsed" << std::endl;

		requireSorted(arr, SD_UP);
		requireSorted(arr2, SD_UP);



	#ifdef PRINT_SORTED
		for(int i = 0; i < VEC_SIZE; i++){
			std::cout << "arr[" << i << "] = " << arr[i] << std::endl;
		}
	#endif

	}
	catch(myfcl::Exception e){
		std::cerr << "ERROR: " << e.what() << " (myfcl::Exception)" << std::endl;
		return -1;
	}
	catch(std::logic_error e){
		std::cerr << "ERROR: " << e.what() << " (std::logic_error)" << std::endl;
		return -1;
	}


}