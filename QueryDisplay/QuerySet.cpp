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
QuerySet::QuerySet(string base_configuration, string change_dimension, CsvWriter* writer){
			this->base_configuration = base_configuration;
			this->change_dimension = change_dimension;
			this->lowerBound = 0;
			this->upperBound = 100;
			this->nextGuess = 50;

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

	bool gestureIdenifiedCorrectly = false;
	string answerString= "Incorrect_ID";

		//if(answer != RIGHT_IMAGE_WON && answer != LEFT_IMAGE_WON && answer != BOTH_IMAGES_TIE && answer != NOT_SURE){}; // user hit a bad key

	if (answer == correctIDResponse){
		gestureIdenifiedCorrectly = true;
		answerString = "Correct_ID";
	}

	write(answer, answerString);


	

	if(gestureIdenifiedCorrectly){
		lowerBound = nextGuess;
		std::cout<<"correct"<<endl;
	}else{
		upperBound = nextGuess;
	}
	nextGuess = (int)floor((lowerBound + upperBound )/ 2.0);

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

void QuerySet::write(char answer, string answerString)
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

	os << getImageName()
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
	   << '\n'
	   << '\0';

	string output;
	os >> output;

	myWriter->write(output);
}