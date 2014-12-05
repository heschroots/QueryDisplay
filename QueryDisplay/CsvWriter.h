#include <iostream>
#include <fstream>
#include <string>

using namespace std;

static ofstream os;
class CsvWriter
{
public:
	CsvWriter();
	CsvWriter(string outFileName);

	int write(const string outputString);
	int write(const string queriedFileName, const string protoImageFileName,
				const string protoImageLocation, const string answer, const int lowerBound, const int upperBound);
	//int write(const T* qs, char answer);

	int close();

private:
	string fileName;
};