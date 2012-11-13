#include <boost/thread/thread.hpp>
#include <iostream>
//using namespace std;


void thread_function()
{
std::cout << "hey" << std::endl;
}
int main (int argc, char *argv[])
{
	std::cout << "Hello world! V2" << std::endl;
	boost::thread threadone(&thread_function);
	threadone.join();
	return 0;
}

