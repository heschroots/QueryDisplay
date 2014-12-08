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

typedef enum{
	BINARY_SEARCH,
	SIMPLE_UPDOWN_STAIRCASE
} SamplingProcedureType;

typedef enum{
	QS_RPS, //Rock Paper Scissors
	QS_FINGER_COUNT //FingerCounting
}QuerySetType;

class QuerySet
{
   public:
	  std::string base_configuration;
	  std::string change_dimension;
	  std::string protoImageName;
	  SamplingProcedureType samplingProcedure;

	  //members used to display next query and judge user's answer
	  int nextGuess;   //
	  QuerySetType queryType;
	  ProtoImageType protoImage;
	  ImageLocationType protoImageLocation;
	  char correctIDResponse;

	  //members used when by the samplingProcedure = BINARY_SEARCH 
      int lowerBound;  // 
      int upperBound;  //

	  //members used when by the samplingProcedure = SIMPLE_UPDOWN_STAIRCASE 	  
	  int stepSizes [3]; //array to keep track of the size of the steps taken when SIMPLE_UPDOWN_STAIRCASE is used. Will be [15,5,2]
	  int currentStepSize; //int to keep track of which step size we are currently taking
	  int pivotsPerStepSize[3]; //array to keep track of how many pivot points should be collected using each step size. Initially, will be [1, 2, 2] (though first will be thrown out)
	  int pivotValues[5]; //array to keep track of the values of the pivot points. Will be used to calculate the threshold
	  int totalPivotsIdentified; //the total number of pivot points that have already been found
	  int totalPivotPointsNeeded; //the total number of pivot points we will need. Should be summation of values in pivotsPerStepSize.
	  int currentStepPivotsFound; //counter to keep track of how many pivots have been found at the current step size. Used to determine when to move to the next step size / end the queries
	  bool lastPointRecognized; //boolean value keeping track of whether the last point was corrected IDed or not. Used to determine which direction to step in
	  bool isFinished; // used to determine if we've finished with this query set



	  QuerySet(std::string base_configuration, std::string change_dimension, QuerySetType type, CsvWriter* writer, SamplingProcedureType samplingProcedure);
	  std::string getImageName();
	  std::string getProtoImageName();
	  void getImageFileNames(std::string &leftImage, std::string &rightImage);
	  void processAnswer(char answer);
	  void generateCorrectIDResponse();

private:
	void generateNewProtoImage();
	void generateProtoImageLocation();
	void write(char asnwer, string answerString, string baseConfiguration, int amountOfChange, string changeDimension, bool isPivotPoint);

	CsvWriter* myWriter;
};