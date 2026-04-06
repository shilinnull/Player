#if 0
#include <iostream>
extern "C" 
{
#include <libavcodec/avcodec.h>
}
#pragma comment(lib,"avcodec.lib")
using namespace std;
int main(int argc, char *argv[])
{
	cout << "Test FFmpeg " << endl;
#ifdef WIN32
	cout << "32 位程序" << endl;
#else
	cout << "64 位程序" << endl;
#endif
	cout << avcodec_configuration() << endl;
	cout << avcodec_version() << endl;
	cout << avcodec_license() << endl;
	return 0;
}
#endif
