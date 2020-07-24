#include "MyFrameCL.hpp"

#include <cstdlib>
#include <ctime>
#include <chrono>

enum{VEC_SIZE = 1024};

int main(){


	myfcl::Context context;
	
	std::vector<int> vec1;
	std::vector<int> vec2;
	std::vector<int> vec3;
	std::vector<int> vec4;

	for(int i = 0; i < VEC_SIZE; i++){
		vec1.emplace_back(rand()%100);
		vec2.emplace_back(rand()%100);
	}
	myfcl::Buffer& buf1 = context.createBuffer(VEC_SIZE  * sizeof(int), CL_MEM_READ_ONLY);
	myfcl::Buffer& buf2 = context.createBuffer(VEC_SIZE  * sizeof(int), CL_MEM_READ_ONLY);
	myfcl::Buffer& buf3 = context.createBuffer(VEC_SIZE  * sizeof(int), CL_MEM_WRITE_ONLY);
	myfcl::Buffer& buf4 = context.createBuffer(VEC_SIZE  * sizeof(int), CL_MEM_WRITE_ONLY);

	myfcl::Queue const& queue = context.createQueue();


	context.copy(vec1, buf1, queue);


	context.copy(vec2, buf2, queue);

	myfcl::Program const& prog_vec = context.createProgram("vector_add_kernel.cl");
	myfcl::Kernel const& vec_add = context.createKernel(prog_vec, "vector_add");
	myfcl::Kernel const& vec_diff = context.createKernel(prog_vec, "vector_diff");



	vec_add.addArgument(0, &buf1.buffer());
	vec_add.addArgument(1, &buf2.buffer());
	vec_add.addArgument(2, &buf3.buffer());

	vec_diff.addArgument(0, &buf1.buffer());
	vec_diff.addArgument(1, &buf2.buffer());
	vec_diff.addArgument(2, &buf4.buffer());


	context.execute(vec_add,{64}, {VEC_SIZE}, queue);

	context.copy(buf3, vec3, queue);
	context.copy(vec3, buf1, queue);
	
	context.execute(vec_diff,{64}, {VEC_SIZE}, queue);


	context.copy(buf4, vec4, queue);

/*	for(int i = 0; i < VEC_SIZE; i++){
		std::cout << vec1[i] << " + " << vec2[i] << " = " << vec3[i] << std::endl;
	}

	for(int i = 0; i < VEC_SIZE; i++){
		std::cout << vec3[i] << " - " << vec2[i] << " = " << vec4[i] << " = " << vec1[i] << std::endl;
	}

*/
	//Matrix multiplication

	
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
/*
	
	CPU multiplying test (~22 seconds)

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
*/

	std::cout << "Done!" << std::endl;
	std::cout << std::endl;
}