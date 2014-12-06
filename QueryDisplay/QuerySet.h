#include <string>
#include "CsvWriter.h"

typedef enum{
	PROTO_ROCK,
	PROTO_PAPER,
	PROTO_SCISSOR
}ProtoImageType;

typedef enum{
	PROTO_IMAGE_ON_LEFT,
	PROTO_IMAGE_ON_RIGHT
}ImageLocationType;

class QuerySet
{
   public:
	  std::string base_configuration;
	  std::string change_dimension;
	  std::string protoImageName;
      int lowerBound;  // 
      int upperBound;  //
	  int nextGuess;   //
	  ProtoImageType protoImage;
	  ImageLocationType protoImageLocation;

	  QuerySet(std::string base_configuration, std::string change_dimension, CsvWriter* writer);
	  std::string getImageName();
	  std::string getProtoImageName();
	  void getImageFileNames(std::string &leftImage, std::string &rightImage);
	  void processAnswer(char answer);

private:
	void generateNewProtoImage();
	void generateProtoImageLocation();
	void write(char asnwer, string answerString);

	CsvWriter* myWriter;
};