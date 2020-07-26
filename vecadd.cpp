#include "MyFrameCL.hpp"

#include <cstdlib>
#include <ctime>
#include <chrono>

enum{VEC_SIZE = 1024};

int main(){


	myfcl::Context context;

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
	std::cout << "Done!" << std::endl;
	std::cout << std::endl;
}