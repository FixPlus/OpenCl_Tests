#include <iostream>
#include <sstream>
#include <vector>
#include <fstream>
#include <list>
#include <CL/cl.h>

#define CHECK_ERR(RET, N) if(RET != CL_SUCCESS) throw(Exception(#N, RET, __LINE__, __FILE__));
namespace myfcl{

enum { STRING_BUFSIZE = 1024 };

struct Exception: public std::runtime_error{
	std::string what_str;
	Exception(const char* op_name,cl_int err_code, int line, const char* file = __FILE__): std::runtime_error(""){
		std::stringstream ss;
		ss << "Error: " << "In file " << file << " on line " << line << ":" << std::endl <<"\t"<< op_name << " returned with " << err_code;
		what_str = ss.str();
	};

	Exception(const char* what): std::runtime_error(""){
		what_str = what;
	}
	const char* what() const noexcept override  {
		return what_str.c_str();
	};
};

class Platform{
protected:
	cl_platform_id pid;
public:
	static void printInfo(cl_platform_id pid){
		cl_int ret;
		char buf[STRING_BUFSIZE];

		ret = clGetPlatformInfo(pid, CL_PLATFORM_NAME, sizeof(buf), buf, NULL);
		CHECK_ERR(ret, clGetPlatformInfo);
		printf("Platform: %s\n", buf);
		ret = clGetPlatformInfo(pid, CL_PLATFORM_VERSION, sizeof(buf), buf, NULL);
		CHECK_ERR(ret, clGetPlatformInfo);
		printf("Version: %s\n", buf);
		ret = clGetPlatformInfo(pid, CL_PLATFORM_PROFILE, sizeof(buf), buf, NULL);
		CHECK_ERR(ret, clGetPlatformInfo);
		printf("Profile: %s\n", buf);
		ret = clGetPlatformInfo(pid, CL_PLATFORM_VENDOR, sizeof(buf), buf, NULL);
		CHECK_ERR(ret, clGetPlatformInfo);
		printf("Vendor: %s\n", buf);
		ret = clGetPlatformInfo(pid, CL_PLATFORM_EXTENSIONS, sizeof(buf), buf, NULL);
		CHECK_ERR(ret, clGetPlatformInfo);
		printf("Extensions: %s\n", buf);
		printf("\n");

	}

	Platform(){
		//Getting platforms info
		cl_uint n_platforms;
   		
   		cl_int ret = clGetPlatformIDs(0, NULL, &n_platforms);
   		
   		CHECK_ERR(ret, clGetPlatformIDs);

    	std::vector<cl_platform_id> platforms;
    	
    	platforms.reserve(10);

    	ret = clGetPlatformIDs(n_platforms, platforms.data(), NULL);
  		
   		CHECK_ERR(ret, clGetPlatformIDs);

   		std::cout << "Avaible platforms info:" << std::endl << std::endl;

   		for(int i = 0; i < n_platforms; i++){
   			std::cout << "Plat id " << platforms[i] << ":" << std::endl;
   			printInfo(platforms[i]);
   		}

   		std::cout << "Choosen platform id: " << platforms[1] << std::endl;  //currently choosing only Intel platform
   		pid = platforms[1];
  		

	};

	virtual ~Platform(){};
};


class Context: public Platform{

	cl_context ct;
	std::vector<cl_device_id> devices;

public:

	void printDevicesInfo() const{

		std::cout << "Avaible devices in the current context:" << std::endl;
		
		for(int j = 0; j < 1; j++){
			std::cout << std::endl;
	  		char buf[STRING_BUFSIZE];
			cl_int ret;
			cl_uint ubuf, i;
			size_t *psbuf, sbuf;
			cl_bool cavail, lavail;
			ret = clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(buf), buf, NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("Device: %s\n", buf);
			ret = clGetDeviceInfo(devices[j], CL_DEVICE_VERSION, sizeof(buf), buf, NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("OpenCL version: %s\n", buf);

			ret = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(ubuf), &ubuf,
			                        NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("Max units: %u\n", ubuf);
			ret = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(ubuf),
			                        &ubuf, NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("Max dimensions: %u\n", ubuf);

			psbuf = (size_t*)malloc(sizeof(size_t) * ubuf);
			ret = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_SIZES,
			                        sizeof(size_t) * ubuf, psbuf, NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("Max work item sizes: ");
			for (i = 0; i != ubuf; ++i)
				printf("%u ", (unsigned)psbuf[i]);
			printf("\n");
			free(psbuf);

			ret = clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(sbuf),
			                        &sbuf, NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("Max work group size: %u\n", (unsigned)sbuf);

			ret = clGetDeviceInfo(devices[j], CL_DEVICE_COMPILER_AVAILABLE, sizeof(cavail),
			                        &cavail, NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("Compiler %savailable\n", cavail ? "" : "not ");
			ret = clGetDeviceInfo(devices[j], CL_DEVICE_LINKER_AVAILABLE, sizeof(lavail),
			                        &lavail, NULL);
			CHECK_ERR(ret, clGetDeviceInfo);
			printf("Linker %savailable\n", lavail ? "" : "not ");
			printf("\n");

		}


	}
	Context(cl_device_type dtype = CL_DEVICE_TYPE_ALL, int dev_count = 1){
		std::cout << std::endl << "#Creating context..." << std::endl;
		std::cout << "Looking for avaible devices on choosen platform..." << std::endl;
		
		devices.resize(dev_count);
		
		cl_uint n_actual_devices;

		cl_int ret = clGetDeviceIDs( pid, dtype, dev_count, 
        devices.data(), &n_actual_devices);

        CHECK_ERR(ret, clGetDeviceIDs);

        std::cout << "There are " << n_actual_devices << " devices matching needed device type on this platform" << std::endl;

        if(dev_count > n_actual_devices) dev_count = n_actual_devices;

        devices.resize(dev_count);

        printDevicesInfo();
        cl_context_properties props[] = {CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(pid), 0};
   		ct = clCreateContext( props, dev_count, devices.data(), NULL, NULL, &ret);

        CHECK_ERR(ret, clCreateContext);

        std::cout << "Context created with " << devices.size() << " devices avaible"<< std::endl;
	};


	cl_context context() const{
		return ct;
	}

	cl_device_id getDevice() const{
		return devices[0];
	}

	cl_device_id const* getDevices() const{
		return devices.data();
	}

	cl_uint getNumOfDevices() const{
		return devices.size();
	}


	~Context(){

		clReleaseContext(ct);
	
	};
};


class Task {
public:


	Task(){};
	
	virtual void run(cl_command_queue queue) = 0;
	virtual ~Task(){};
};

class Queue {

	cl_command_queue queue_;
	std::list<Task*> tasks;

public:
	Queue(Context const& ct, cl_command_queue_properties properties = 0){
		cl_int ret;
		queue_ = clCreateCommandQueue(ct.context(), ct.getDevice(), properties, &ret);
		CHECK_ERR(ret, clCreateCommandQueue);
	}

	Queue(Queue const& another) = delete;

	Queue const& operator=(Queue const& another) = delete;

#if 0
	Queue(Queue&& another){
		queue_ = another.queue_;
		another.taken = true;
	};

	Queue const& operator=(Queue&& another){
		queue_ = another.queue_;
		another.taken = true;
		return *this;

	};

#endif

	void execute() {
		while(!tasks.empty()){
			Task* task = tasks.back();
			task->run(queue_);
			delete(task);
			tasks.pop_back();
		}
	}

	void addTask(Task* task){
		tasks.push_front(task);
	}

	cl_command_queue queue() const{
		return queue_;
	}
	virtual ~Queue(){
		while(!tasks.empty()){
			delete(tasks.back());
			tasks.pop_back();			
		}
		clFlush(queue_);
		clFinish(queue_);		
		
	}

};

template<typename T>
class Buffer {
	
	//ocl part

	cl_mem buffer_;
	cl_mem_flags flags_;

	//host part

	std::vector<T>* data_;
	bool is_extern_data = false;

public:

	Buffer(Context const& ct, size_t size, cl_mem_flags flags = CL_MEM_READ_WRITE): 
												flags_(flags), data_(new std::vector<T>(size)){
		
		cl_int ret;
		buffer_ = clCreateBuffer(ct.context(), flags, size * sizeof(T), NULL, &ret);
		CHECK_ERR(ret, clCreateBuffer);
	
	}

	Buffer(Context const& ct, std::vector<T>* extern_data, cl_mem_flags flags = CL_MEM_READ_WRITE): 
												flags_(flags), data_(extern_data), is_extern_data(true){
		
		cl_int ret;
		buffer_ = clCreateBuffer(ct.context(), flags, data_->size() * sizeof(T), NULL, &ret);
		CHECK_ERR(ret, clCreateBuffer);
	
	}

	Buffer(Buffer const& another) = delete;

	Buffer const& operator=(Buffer const& another) = delete;

#if 0
	Buffer(Buffer&& another){
		buffer_ = another.buffer_;
		size_ = another.size_;
		flags_ = another.flags_;
		another.taken = true;
	};

	Buffer const& operator=(Buffer&& another){
		buffer_ = another.buffer_;
		another.taken = true;
		return *this;

	};

#endif

	size_t size() const{
		return data_->size() * sizeof(T);
	}

	cl_mem& buffer() {
		return buffer_;
	};

	T* hostData() {
		return data_->data();
	}

	auto begin(){
		return data_->begin();
	}

	auto end(){
		return data_->end();
	}

	T& operator[](size_t index){
		if(index > data_->size()){
			std::stringstream ss;
			ss << "Index " << index << " is out of buffer range(" << data_->size() << ")"; 
			throw(std::out_of_range{ss.str()});
		}

		return (*data_)[index];
	}

	virtual ~Buffer(){
		clReleaseMemObject(buffer_);

		if(!is_extern_data)
			delete data_;
	};
};

enum { MAX_SOURCEFILE_LENGTH = 10000};

class Program{

	cl_program program_;

public:

	Program(Program const& another) = delete;

	Program const& operator=(Program const& another) = delete;

#if 0

	Program(Program&& another){
		program_ = another.program_;
		another.taken = true;
	};

	Program const& operator=(Program&& another){
		program_ = another.program_;
		another.taken = true;
		return *this;

	};

#endif

	Program(Context const& ct, const char* file_path){
		std::cout << "Building programm " << file_path << "..." << std::endl;
		std::fstream prog_file(file_path);
		if(!prog_file.good()){
			std::stringstream ss;
			ss << "Program file " << file_path << " cannot be opened"; 
			throw(Exception(ss.str().c_str()));
		}

		std::string prog_source_code;
		prog_source_code.resize(MAX_SOURCEFILE_LENGTH + 1);
		prog_file.read(prog_source_code.data(), MAX_SOURCEFILE_LENGTH);

		prog_source_code.data()[MAX_SOURCEFILE_LENGTH] = '\0';

		cl_int ret;
		
		const char* code = prog_source_code.data();
		
		program_ = clCreateProgramWithSource(ct.context(), 1, &code, NULL, &ret);
		CHECK_ERR(ret, clCreateProgramWithSource);

		ret = clBuildProgram(program_, ct.getNumOfDevices(), ct.getDevices(), NULL, NULL, NULL);

		if(ret != CL_SUCCESS){
			for(int i = 0; i < ct.getNumOfDevices(); i++){
				std::string log;
				log.resize(10000);

				clGetProgramBuildInfo(program_, ct.getDevices()[i], CL_PROGRAM_BUILD_LOG, 10000, log.data(), NULL);

				std::cout << "Programm build failed with following log:" << std::endl << log << std::endl;
			}
		}
		
		CHECK_ERR(ret, clBuildProgram);

		std::cout << "Programm has been built successfuly" << std::endl;
	}

	cl_program program() const{
		return program_;
	}

	virtual ~Program(){
		clReleaseProgram(program_);
	}
};


class NDRange // Copied from CL/cl2.hpp
{
private:
    size_t sizes_[3];
    cl_uint dimensions_;

public:
    //! \brief Default constructor - resulting range has zero dimensions.
    NDRange()
        : dimensions_(0)
    {
        sizes_[0] = 0;
        sizes_[1] = 0;
        sizes_[2] = 0;
    }

    //! \brief Constructs one-dimensional range.
    NDRange(size_t size0)
        : dimensions_(1)
    {
        sizes_[0] = size0;
        sizes_[1] = 1;
        sizes_[2] = 1;
    }

    //! \brief Constructs two-dimensional range.
    NDRange(size_t size0, size_t size1)
        : dimensions_(2)
    {
        sizes_[0] = size0;
        sizes_[1] = size1;
        sizes_[2] = 1;
    }

    //! \brief Constructs three-dimensional range.
    NDRange(size_t size0, size_t size1, size_t size2)
        : dimensions_(3)
    {
        sizes_[0] = size0;
        sizes_[1] = size1;
        sizes_[2] = size2;
    }

    /*! \brief Conversion operator to const size_type *.
     *  
     *  \returns a pointer to the size of the first dimension.
     */
    operator const size_t*() const { 
        return sizes_; 
    }

    //! \brief Queries the number of dimensions in the range.
    size_t dimensions() const 
    { 
        return dimensions_; 
    }

    //! \brief Returns the size of the object in bytes based on the
    // runtime number of dimensions
    size_t size() const
    {
        return dimensions_*sizeof(size_t);
    }

    size_t* get()
    {
        return sizes_;
    }
    
    const size_t* get() const
    {
        return sizes_;
    }
};



class Kernel{

	cl_kernel kernel_;

public:

	Kernel(Kernel const& another) = delete;

	Kernel const& operator=(Kernel const& another) = delete;

#if 0

	Kernel(Kernel&& another){
		kernel_ = another.kernel_;
		another.taken = true;
	};

	Kernel const& operator=(Kernel&& another){
		kernel_ = another.kernel_;
		another.taken = true;
		return *this;

	};

#endif

	template<typename T>
	void addArgument(cl_uint index, T* arg) const{
		cl_int ret = clSetKernelArg(kernel_, index, sizeof(T), reinterpret_cast<void*>(arg));
		CHECK_ERR(ret, clSetKernelArg);
	}

	Kernel(Program const& prog, const char* name){
		cl_int ret;
		kernel_ = clCreateKernel(prog.program(), name, &ret);
		CHECK_ERR(ret, clCreateKernel);

	}

	cl_kernel kernel() const{
		return kernel_;
	}
	~Kernel(){
		clReleaseKernel(kernel_);
	}
};

template<typename T>
class Read: public Task{
	Buffer<T>& buf_;
public:
	Read(Buffer<T>& buf): buf_(buf) {
	};

	void run(cl_command_queue queue) override{
		cl_int ret = clEnqueueReadBuffer(queue, buf_.buffer(), CL_TRUE, 0, buf_.size(), buf_.hostData(), 0, NULL, NULL);
		CHECK_ERR(ret, clEnqueueReadBuffer);
	}

	~Read(){};
};

template<typename T>
class Write: public Task{
	Buffer<T>& buf_;
public:
	Write(Buffer<T>& buf): buf_(buf) {
	};

	void run(cl_command_queue queue) override{
		cl_int ret = clEnqueueWriteBuffer(queue, buf_.buffer(), CL_TRUE, 0, buf_.size(), buf_.hostData(), 0, NULL, NULL);
		CHECK_ERR(ret, clEnqueueWriteBuffer);
	}
	~Write(){};
};


class Execute: public Task{
	
	Kernel& kernel_;
	NDRange local_, global_;

public:
	Execute(Kernel& kernel, NDRange local, NDRange global): kernel_(kernel), local_(local), global_(global) {
	};

	void run(cl_command_queue queue) override{
		cl_int ret = clEnqueueNDRangeKernel(queue, kernel_.kernel(), global_.dimensions(), NULL, global_.get(), local_.get(), 0, NULL, NULL);
		CHECK_ERR(ret, clEnqueueNDRangeKernel);
	}

	~Execute(){};

};



};