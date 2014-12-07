#include <string>
#include <time.h>
#include <sstream>
#include "QuerySet.h"

#define RIGHT_IMAGE_WON 'm'
#define LEFT_IMAGE_WON 'z'
#define BOTH_IMAGES_TIE 't'
#define NOT_SURE '\n'


using namespace std;

//QuerySet Constructor
QuerySet::QuerySet(string base_configuration, string change_dimension, CsvWriter* writer, SamplingProcedureType samplingProcedure){
			this->base_configuration = base_configuration;
			this->change_dimension = change_dimension;

			this->samplingProcedure = samplingProcedure; //BINARY_SEARCH or SIMPLE_UPDOWN_STAIRCASE;

			if (this->samplingProcedure == BINARY_SEARCH){
				this->lowerBound = 0;
				this->upperBound = 100;
				this->nextGuess = 50;
			}
		
			if (this->samplingProcedure == SIMPLE_UPDOWN_STAIRCASE){
				this->nextGuess = 0;
				this->lastPointRecognized = true;
				this->totalPivotsIdentified = 0;
				this->isFinished = false;
				this->currentStepSize = 0;
				this->currentStepPivotsFound = 0;

				this->stepSizes[0] = 15;
				this->stepSizes[1] = 5;
				this->stepSizes[2] = 2;

				this->pivotsPerStepSize[0] = 1;
				this->pivotsPerStepSize[1] = 2;
				this->pivotsPerStepSize[2] = 2;
				this->totalPivotPointsNeeded = 5; //needs to be the sum of all the pivotsPerStepSize values
			}


			//seed random number generator
			srand( time(NULL) );
			generateNewProtoImage();

			myWriter = writer;
}


//Method that sets correctIDResponse to a char, corresponding to the keyboard input indicating that symbol has been correctly interpreted
enum outcomes{WIN, LOSE, TIE};

void QuerySet::generateCorrectIDResponse(){
	
	outcomes outcome;
		 if((base_configuration == "paper") && (protoImage == PROTO_ROCK)){ outcome = WIN;}
	else if((base_configuration == "paper") && (protoImage == PROTO_PAPER)){ outcome = TIE;}
	else if((base_configuration == "paper") && (protoImage == PROTO_SCISSOR)){ outcome = LOSE;}

	else if((base_configuration == "scissor") && (protoImage == PROTO_ROCK)){ outcome = LOSE;}
	else if((base_configuration == "scissor") && (protoImage == PROTO_PAPER)){ outcome = WIN;}
	else if((base_configuration == "scissor") && (protoImage == PROTO_SCISSOR)){ outcome = TIE;}

	else if((base_configuration == "rock") && (protoImage == PROTO_ROCK)){ outcome = TIE;}
	else if((base_configuration == "rock") && (protoImage == PROTO_PAPER)){ outcome = LOSE;}
	else if((base_configuration == "rock") && (protoImage == PROTO_SCISSOR)){ outcome = WIN;}


	char theResponse = 'x';
		 if (outcome == WIN && protoImageLocation == PROTO_IMAGE_ON_LEFT){ theResponse = RIGHT_IMAGE_WON;}
	else if (outcome == WIN && protoImageLocation == PROTO_IMAGE_ON_RIGHT){ theResponse = LEFT_IMAGE_WON;}

	else if (outcome == LOSE && protoImageLocation == PROTO_IMAGE_ON_LEFT){ theResponse = LEFT_IMAGE_WON;}
	else if (outcome == LOSE && protoImageLocation == PROTO_IMAGE_ON_RIGHT){ theResponse = RIGHT_IMAGE_WON;}

	else if (outcome == TIE){ theResponse = BOTH_IMAGES_TIE;}

	this->correctIDResponse = theResponse;

}

//Method to be called that will updated lowerBound, upperBound, and nextGuess when new information
//has been collected from the user. The information processed will correspond to the point specified
//by the prior value of nextGuess. If the method is passed True, the this point was correctly identified
//and the value of lowerBound is replaced by nextGuess's value. If the point was incorrectly identified,
//the value of upperBound is replaced by nextGuess's value. Finally, the new value of nextGuess must be computed
void QuerySet::processAnswer(char answer){
	int currentGuess = nextGuess;
	bool isPivotPoint = false;
	bool gestureIdenifiedCorrectly = false;

	string answerString= "Incorrect_ID";
	if (answer == correctIDResponse){
		gestureIdenifiedCorrectly = true;
		answerString = "Correct_ID";
	}




	
	if(this->samplingProcedure == BINARY_SEARCH){
		if(gestureIdenifiedCorrectly){
			lowerBound = nextGuess;
			std::cout<<"correct"<<endl;
		}else{
			upperBound = nextGuess;
		}
		nextGuess = (int)floor((lowerBound + upperBound )/ 2.0);
	}

	if(this->samplingProcedure == SIMPLE_UPDOWN_STAIRCASE){


		//first, check to see if this is a pivot point.
		if((gestureIdenifiedCorrectly && !lastPointRecognized) || (!gestureIdenifiedCorrectly && lastPointRecognized))
			isPivotPoint = true;

		std::cout<<int(gestureIdenifiedCorrectly)<<endl;
		
		if(isPivotPoint) //otherwise it is a pivot point
		{
			std::cout<<"PivotPoint"<<endl;
			//add it to our array of pivot points
			pivotValues[totalPivotsIdentified] = currentGuess;
			totalPivotsIdentified++;
			currentStepPivotsFound++;

			//if this was the last pivot point needed, then mark this query set as done
			if(currentStepPivotsFound == totalPivotPointsNeeded)
			{
				isFinished = true;
					write(answer, answerString, this->base_configuration, currentGuess, this->change_dimension, isPivotPoint);
				return ;
			}


			//otherwise, if this was the last pivot point needed for the step size, go to the next step size
			if(currentStepPivotsFound == pivotsPerStepSize[currentStepSize]){
				currentStepSize++;
			}
		}
			//Now that we are certain we will use the correct step size, increment or decrement the current guess to get the next one 

		//if pivot point and last point was correctly recognized, increase difficutly
		if(gestureIdenifiedCorrectly){
			nextGuess = currentGuess + stepSizes[currentStepSize];
			lastPointRecognized = true;
		}else{
			nextGuess = currentGuess - stepSizes[currentStepSize];
			lastPointRecognized = false;
		}

		//boundary conditions for nextGuess
		if(nextGuess > 100){
			write(answer, answerString, this->base_configuration, currentGuess, this->change_dimension, isPivotPoint);
			isFinished = true; //mark as finished, because apparently no change in this dimension matters whatsoever.
		}
		else if (nextGuess < 0)
			nextGuess = 0;


	
	}
	write(answer, answerString, this->base_configuration, currentGuess, this->change_dimension, isPivotPoint);

	//Identify new proto image
	generateNewProtoImage();

}

void QuerySet::getImageFileNames(string &leftImage, string &rightImage){

	if(protoImageLocation == PROTO_IMAGE_ON_LEFT)
	{
		leftImage = getProtoImageName();
		rightImage = getImageName();
	}
	else
	{
		leftImage = getImageName();
		rightImage = getProtoImageName();
	}

}
//returns the name of the next image viewed
string QuerySet::getImageName(void){
	return	base_configuration + "-" +
			change_dimension + "_" +
			to_string(100-nextGuess) + "-" +
			to_string(nextGuess) + ".tif";
}

string QuerySet::getProtoImageName(void){

	return protoImageName;
}

void QuerySet::generateNewProtoImage(){

	//generate a random number 0,1, or 2
	//This corresponds to rock, paper, scissor
	//map this random number to generate
	//the rock, paper, or scissors filename
	int randomNum = rand() % 3; //some int from 0 to 2
	switch(randomNum)
	{
	case 0:
		protoImage = PROTO_ROCK;
		protoImageName = "rock.tif";
		break;
	case 1:
		protoImage = PROTO_PAPER;
		protoImageName = "paper.tif";
		break;
	case 2:
		protoImage = PROTO_SCISSOR;
		protoImageName = "scissor.tif";
		break;
	}
	generateProtoImageLocation();
}

void QuerySet::generateProtoImageLocation(){

	//random coin toss (50/50) for if proto Image will
	//be on left or right
	int randomNum = rand() % 2; //either 0 or 1

	if(randomNum)
		protoImageLocation = PROTO_IMAGE_ON_LEFT;
	else
		protoImageLocation = PROTO_IMAGE_ON_RIGHT;

	generateCorrectIDResponse();

}

	static int row_id = 0;
void QuerySet::write(char answer, string answerString, string baseConfiguration, int amountOfChange, string changeDimension, bool isPivotPoint)
{
	stringstream os;
	string location;
	string answerChartoString;
	char delim = ',';

	if(protoImageLocation)
		location = "Right";
	else 
		location = string("Left");

	if(answer == 32)
		answerChartoString = "";
	else
		answerChartoString = string(&answer);

	os << row_id
	   << delim
		<< getImageName()
	   << delim
	   << getProtoImageName()
	   << delim
	   << location
	   << delim
	   << answerChartoString
	   << delim
	   << answerString
	   << delim
	   << lowerBound
	   << delim
	   << upperBound
	   << delim
	   << baseConfiguration
	   << delim
	   << amountOfChange
	   << delim
	   << changeDimension
	   << delim
	   << isPivotPoint
	   << '\n'
	   << '\0';

	string output;
	os >> output;

	myWriter->write(output);
	row_id++;
}