#include "MyFrameCL.hpp"
#include <cstdlib>
#include <ctime>

/* 
	matrices.cpp 
	
	Runs tests of matrices.cl kernels
	
	

*/



template<typename T>
class Matrix{

	// 2D matrix container adapter

	using container = std::vector<T>;
	
	container data_;
	
	size_t x_, y_; // x  -horisonal size; y - vertical size

	class MatrixRowConst{
		container::const_iterator row;
		size_t x_;
	public:
		MatrixRowConst(container::const_iterator it, size_t x): row(it), x_(x){};
		T const& operator[](size_t x_index) {
			if(x_index > x_){
				std::stringstream ss;
				ss << "Column index " << x_index << " is out of matrix column range(" << x_ << ")";
				throw(std::out_of_range{ss.str()});
			}
			return row[x_index];
		}
	};

	class MatrixRow{
		container::iterator row;
		size_t x_;
	public:
		MatrixRow(container::iterator it, size_t x): row(it), x_(x){};
		T& operator[](size_t x_index) {
			if(x_index > x_){
				std::stringstream ss;
				ss << "Column index " << x_index << " is out of matrix column range(" << x_ << ")";
				throw(std::out_of_range{ss.str()});
			}
			return row[x_index];
		}
	};

public:

	Matrix(size_t x): x_(x), y_(x), data_(x * x){
	}

	Matrix(size_t x, size_t y): x_(x), y_(y), data_(x * y){
	}

	size_t x() const{
		return x_;
	}

	size_t y() const{
		return y_;
	}

	MatrixRowConst operator[](size_t y_index) const{
		if(y_index > y_){
			std::stringstream ss;
			ss << "Row index " << y_index << " is out of matrix row range(" << y_ << ")";
			throw(std::out_of_range{ss.str()});
		}
		
		return MatrixRowConst{data_.begin() + y_index * x_, x_}; 
	}

	MatrixRow operator[](size_t y_index) {
		if(y_index > y_){
			std::stringstream ss;
			ss << "Row index " << y_index << " is out of matrix row range(" << y_ << ")";
			throw(std::out_of_range{ss.str()});
		}
		
		return MatrixRow{data_.begin() + y_index * x_, x_}; 
	}

	void randomize(size_t range = 100){
		int rint = static_cast<int>(range);
		for(auto&& i: data_)
			i = static_cast<T>(rand() % rint - rint / 2);
	}

	container const& data() const{
		return data_;
	}

	container& data() {
		return data_;
	}

	void setNull(){
		for(auto&& i: data_)
			i = static_cast<T>(0);
	}


	void swapRows(size_t row1, size_t row2){
		if(row1 > y_ || row2 > y_)
			throw(std::out_of_range("Matrix index out of range"));

		if(row1 == row2)
			return;

		std::swap_ranges(data_.begin() + row1 * x_, data_.begin() + (row1 + 1) * x_, data_.begin() + row2 * x_);
	}


	void print(){
		for(int j = 0; j < y_; j++){
			for(int i = 0; i < x_; i++)
				std::cout << data_[j * x_ + i] << " ";
			std::cout << std::endl;
		}
	}
};

template<typename T>
Matrix<T> getEMatrix(size_t size){

	//Creates diag(1,1...1) matrix of given size

	Matrix<T> ret{size, size};
	
	ret.setNull();

	for(int i = 0; i < size; i++)
		ret[i][i] = static_cast<T>(1);

	return ret;
}

template<typename T>
void require_squared(Matrix<T> const& mat){

	// Ensures mat to have equal dimensions

	if(mat.x() != mat.y())
		throw(std::logic_error("Square matrix required"));
}

template<typename T>
void require_E(Matrix<T> const& mat){

	// Ensures mat to have diag(1,1...1) form

	try{
		require_squared(mat);
	}
	catch(std::logic_error e){
		throw(std::logic_error{"Matrix required to be E, but is not squared"});
	}

	for(int i = 0; i < mat.x(); i++)
		for(int j = 0; j < mat.y(); j++){
			if((i == j && mat[j][i] != static_cast<T>(1)) || (i != j && mat[j][i] != static_cast<T>(0)))
				throw(std::logic_error("Matrix required to be E"));
		}
}

bool flt_eps_equal(float f1, float f2){
	return abs(f1 - f2) < 0.01f;
}

template<>
void require_E<float>(Matrix<float> const& mat){
	try{
		require_squared(mat);
	}
	catch(std::logic_error e){
		throw(std::logic_error{"Matrix required to be E, but is not squared"});
	}
	for(int i = 0; i < mat.x(); i++)
		for(int j = 0; j < mat.y(); j++){
			if((i == j && !flt_eps_equal(mat[j][i],1.0f)) || (i != j && !flt_eps_equal(mat[j][i],0.0f)))
				throw(std::logic_error("Matrix required to be E"));
		}
}


Matrix<float> mat_reverse(Matrix<float> const& mat, myfcl::Context context){ 

	//performs matrix reverse by gaussian method using OCL context

	require_squared(mat);

	Matrix<float> temp = mat;

	Matrix<float> ret = getEMatrix<float>(mat.x());

	myfcl::Buffer<float> buf1{context, &temp.data()};
	myfcl::Buffer<float> buf2{context, &ret.data()};
	myfcl::Buffer<int> errBuf{context, 1};

	errBuf.begin()[0] = -2;

	std::copy(mat.data().begin(), mat.data().end(), buf1.begin());

	myfcl::Program prog{context, "matrices.cl"};
	myfcl::Kernel simpl{prog, "matrix_simplify_column"};

	myfcl::Queue queue{context};

	int size = mat.x();

	simpl.addArgument(0, &buf1.buffer());
	simpl.addArgument(1, &buf2.buffer());
	simpl.addArgument(2, &size);
	simpl.addArgument(4, &errBuf.buffer());

	queue.addTask(new myfcl::Write{buf1});
	queue.addTask(new myfcl::Write{buf2});
	queue.addTask(new myfcl::Write{errBuf});

	int work_group_size = mat.x() < 8 ? mat.x(): 8;

	for(int i = 0; i < mat.x(); i++){
		simpl.addArgument(3, &i);
		queue.addTask(new myfcl::Execute{simpl, {work_group_size}, {mat.x()}});
		queue.addTask(new myfcl::Read{errBuf});
		queue.execute();

		if(errBuf.begin()[0] == -1)  

			// Matrix is discovered to have null determinant
			
			throw(std::logic_error{"Matrix can't be reversed(det == 0)"});
	}

	queue.addTask(new myfcl::Read{buf2});

	queue.execute();
	return ret;
}

template<typename T>
struct mat_mult_kernel{

};

template<>
struct mat_mult_kernel<float>{
	static constexpr const char* name = "matrix_multiply_float";
};

template<>
struct mat_mult_kernel<int>{
	static constexpr const char* name = "matrix_multiply";
};


template<typename T>
Matrix<T> mat_mult(Matrix<T>& mat1, Matrix<T>& mat2, myfcl::Context context){ 

	//Perform multiplication of 2 matrices using OCL context


	srand(time(NULL));
	Matrix<T> ret{mat1.x(), mat2.y()};

	myfcl::Buffer<T> buf1{context, &mat1.data()};
	myfcl::Buffer<T> buf2{context, &mat2.data()};
	myfcl::Buffer<T> buf3{context, &ret.data()};
	
	myfcl::Program prog{context, "matrices.cl"};
	myfcl::Kernel mult{prog, mat_mult_kernel<T>::name};

	myfcl::Queue queue{context};

	int AX = mat1.x();
	int BX = mat2.x();

	mult.addArgument(0, &buf1.buffer());
	mult.addArgument(1, &buf2.buffer());
	mult.addArgument(2, &buf3.buffer());
	mult.addArgument(3, &AX);
	mult.addArgument(4, &BX);

	queue.addTask(new myfcl::Write{buf1});
	queue.addTask(new myfcl::Write{buf2});
	
	queue.addTask(new myfcl::Execute{mult, {{8}, {8}}, {{mat1.y()}, {mat1.x()}}});

	queue.addTask(new myfcl::Read{buf3});

	queue.execute();

	return ret;
}

Matrix<int> mat_transpose(Matrix<int>& mat, myfcl::Context context){ 
	

	//Perform matrix transpose using OCL context
	

	Matrix<int> ret{mat.y(), mat.x()};

	myfcl::Buffer<int> buf1{context, &mat.data()};
	myfcl::Buffer<int> buf2{context, &ret.data()};
	
	myfcl::Program prog{context, "matrices.cl"};
	myfcl::Kernel transpose{prog, "matrix_transpose"};

	myfcl::Queue queue{context};

	int X = mat.x();
	int Y = mat.y();

	transpose.addArgument(0, &buf1.buffer());
	transpose.addArgument(1, &buf2.buffer());
	transpose.addArgument(2, &X);
	transpose.addArgument(3, &Y);

	queue.addTask(new myfcl::Write{buf1});
	
	queue.addTask(new myfcl::Execute{transpose, {{8}, {8}}, {{mat.x()}, {mat.y()}}});

	queue.addTask(new myfcl::Read{buf2});

	queue.execute();

	return ret;
}

template<typename T>
void require_transposed(Matrix<T>& mat1, Matrix<T>& mat2){ 

	
	//Throws exception if mat1 and mat2 are not related as transposed form of each other
	

	if(mat1.x() != mat2.y() || mat1.y() != mat2.x())
		throw(std::logic_error{"ERROR: Matrices sizes are incompatible"});

	for(int i = 0; i < mat1.x(); i++)
		for(int j = 0; j < mat1.y(); j++)
			if(mat1[j][i] != mat2[i][j])
				throw(std::logic_error{"ERROR: Matrices are not transposed properly"});
}

const int TRANSPOSE_TEST_SIZE = 1024;
const int REVERSE_TEST_SIZE = 128; // max stable size is ~ 128 


int main(){
	
	bool err_catched = false;

	try{

	myfcl::Context context;


	std::cout << ">Checking matrix transpose" << std::endl;
	
	Matrix<int> mat{TRANSPOSE_TEST_SIZE};

	mat.randomize(10);

	Matrix<int> transpose = mat_transpose(mat, context);

	require_transposed(mat, transpose);

	std::cout << "Test completed successfully" << std::endl << std::endl;


	std::cout << ">Checking matrix reverse and multiplication" << std::endl;

	Matrix<float> matRef{REVERSE_TEST_SIZE};
	matRef.randomize(100);

	
	Matrix<float> matRev = mat_reverse(matRef, context);

	Matrix<float> probably_E = mat_mult(matRef, matRev, context);

	require_E<float>(probably_E);

	std::cout << "Test completed successfully" << std::endl << std::endl;
	}
	catch(myfcl::Exception e){
		std::cerr << "ERROR: " << e.what() << " (myfcl::Exception)" << std::endl;
		err_catched = true;  
	}
	catch(std::out_of_range e){
		std::cerr << "ERROR: " << e.what() << " (std::out_of_range)" << std::endl;
		err_catched = true;  
	}
	catch(std::logic_error e){
		std::cerr << "ERROR: " << e.what() << " (std::logic_error)" << std::endl;
		err_catched = true;  
	}

	if(err_catched){
		std::cout << "Tests finished with error" << std::endl;
		return -1;
	}
	else{
		std::cout << "All tests finished successfully" << std::endl;
		return 0;
	}
}