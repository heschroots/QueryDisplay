#include "CsvWriter.h"

CsvWriter::CsvWriter(){}

CsvWriter::CsvWriter(string outFileName)
{
	//open filestream
	os.open(outFileName, ios::out);

	if(!os)
		std::cout << "Error opening ouput CSV file" << std::endl;

	fileName = outFileName;

	char delim = ',';
	//output header
	os << "Row_ID"
		<< delim
		<< "QueryFileName"
		<< delim
		<< "ProtoImageName"
		<< delim
		<< "ProtoImageLocation"
		<< delim
		<< "AnswerInput" 
		<< delim 
		<< "AnswerString"
		<< delim
		<< "LowerBound"
		<< delim
		<< "UpperBound"
		<< delim
		<< "BaseConfiguration"
		<< delim
		<< "AmountOfChange"
		<< delim
		<< "ChangeDimension"
		<< delim
		<< "IsPivotPoint"
		<< '\n';
}

int CsvWriter::write(const string outputString)
{
	os << outputString << '\n';
		return 0;
}

int CsvWriter::write(const string queriedFileName, const string protoImageFileName,
			const string protoImageLocation, const string answer, const int lowerBound, const int upperBound)
{
	return 0;
}

/*
tempalte<typename T>
int CsvWriter<T>::write(const T* qs, char answer)
{
	char delim = ',';
	os << qs->getImageName()
	   << delim
	   << qs->getProtoImageName()
	   << delim
	   << (qs->protoImageLocation) ? "Right" : "Left" 
	   << delim
	   << answer
	   << "\n";
		return 0;
}
*/

int CsvWriter::close()
{
	os.close();
	return 0;
}
