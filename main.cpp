#include "MyFrameCL.hpp"

#include <cstdlib>
#include <ctime>
#include <chrono>

enum{VEC_SIZE = 1024};



void bitonic_sort(myfcl::Context const& context, std::vector<int>& array) {
	unsigned int N = array.size();
	unsigned int logN = 0;
	while(N != 0 && logN < 10) N = N >> 1u, logN++; 
	if(1u << logN != array.size())
		throw(std::logic_error("Array size must be presisely 2^N"));

	N = array.size();

	myfcl::Buffer<int> buf{context, N};

	std::copy(array.begin(), array.end(), buf.begin());

	myfcl::Program prog{context, "bitonic_sort.cl"};
	myfcl::Kernel sort{prog, "sort"};

	myfcl::Queue queue{context};
	
	cl_uint i = 0,j = 0;

	sort.addArgument(0, &buf.buffer());
	sort.addArgument(1, &i);
	sort.addArgument(2, &j);

	queue.addTask(new myfcl::Write{buf});

//	for(i = 0; i < logN; i++)
//		for(j = 0; j <= i; j++){
			queue.addTask(new myfcl::Execute{sort,{64}, {N / 2}});
			queue.execute();
//		}
	
	queue.addTask(new myfcl::Read{buf});
	queue.execute();

	std::copy(buf.begin(), buf.end(), array.begin());

}

int main(){


	myfcl::Context context;
	std::vector<int> arr(1024);
	bitonic_sort(context, arr);
	std::cout << arr[0] << std::endl;
	myfcl::Buffer<int> buf1{context, VEC_SIZE};
	myfcl::Buffer<int> buf2{context, VEC_SIZE};
	myfcl::Buffer<int> buf3{context, VEC_SIZE};
	myfcl::Buffer<int> buf4{context, VEC_SIZE};

	for(int i = 0; i < VEC_SIZE; i++){
		buf1[i] = rand()%100;
		buf2[i] = rand()%100;
	}


	myfcl::Queue queue{context};


	queue.addTask(new myfcl::Write{buf1});
	queue.addTask(new myfcl::Write{buf2});

	myfcl::Program prog_vec{context, "vector_add_kernel.cl"};
	myfcl::Kernel vec_add{prog_vec, "vector_add"};
	myfcl::Kernel vec_diff{prog_vec, "vector_diff"};



	vec_add.addArgument(0, &buf1.buffer());
	vec_add.addArgument(1, &buf2.buffer());
	vec_add.addArgument(2, &buf3.buffer());

	vec_diff.addArgument(0, &buf1.buffer());
	vec_diff.addArgument(1, &buf2.buffer());
	vec_diff.addArgument(2, &buf4.buffer());


	queue.addTask(new myfcl::Execute{vec_add, {64}, {VEC_SIZE}});

	queue.addTask(new myfcl::Read{buf3});

	queue.execute();

//	for(int i = 0; i < VEC_SIZE; i++){
//		std::cout << buf1[i] << " + " << buf2[i] << " = " << buf3[i] << std::endl;
//	}

	//Matrix multiplication

#if 0	
	struct matrix_t{
		std::vector<int> mat;
		int x, y;
	};
	
	int AX = 1024;
	int AY = 1024;
	int BY = 1024;

	matrix_t mat1{{}, AX, AY}, mat2{{}, AY, BY}, mat3{{}, AX, BY};

	for(int i = 0; i < mat1.x * mat1.y; i++)
		mat1.mat.emplace_back(rand() % 10);

	for(int i = 0; i < mat2.x * mat2.y; i++)
		mat2.mat.emplace_back(rand() % 10);


	myfcl::Buffer& bufMat1 = context.createBuffer(mat1.mat.size()  * sizeof(int), CL_MEM_READ_ONLY);
	myfcl::Buffer& bufMat2 = context.createBuffer(mat2.mat.size()  * sizeof(int), CL_MEM_READ_ONLY);
	myfcl::Buffer& bufMat3 = context.createBuffer(mat1.mat.size()  * sizeof(int), CL_MEM_WRITE_ONLY);

	myfcl::Program const& prog_mat = context.createProgram("simple.cl");
	myfcl::Kernel const& mat_mult = context.createKernel(prog_mat, "matrix_multiply");


	mat_mult.addArgument(0, &bufMat1.buffer());
	mat_mult.addArgument(1, &bufMat2.buffer());
	mat_mult.addArgument(2, &bufMat3.buffer());
	mat_mult.addArgument(3, &AX);
	mat_mult.addArgument(4, &AY);
	mat_mult.addArgument(5, &BY);

	auto check1 = std::chrono::high_resolution_clock::now();

// GPU mult test (~0.3 sec)
	
	context.copy(mat1.mat, bufMat1, queue);
	context.copy(mat2.mat, bufMat2, queue);


	context.execute(mat_mult, {8, 8}, {1024, 1024}, queue);

	context.copy(bufMat3, mat3.mat, queue);
	auto check2 = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> fs = check2 - check1;
	
	std::cout << fs.count() << " seconds elapsed" << std::endl;

	
//	CPU multiplying test (~22 seconds)

	check1 = std::chrono::high_resolution_clock::now();
	for(int row = 0; row < 1024; row++)
		for(int col = 0; col < 1024; col++)
		{
			int sum = 0;
				for (int k = 0; k < AY; k++)
				sum += mat1.mat[row * AY + k] * mat2.mat[k * BY + col];

				mat3.mat[row * BY + col] = sum;
		}
	check2 = std::chrono::high_resolution_clock::now();
	fs = check2 - check1;

	std::cout << fs.count() << " seconds elapsed" << std::endl;

#endif
	std::cout << "Done!" << std::endl;
	std::cout << std::endl;
}