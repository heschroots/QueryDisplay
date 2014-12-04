#include <string>
#include "QuerySet.h"
using namespace std;



//QuerySet Constructor
QuerySet::QuerySet(string base_configuration, string change_dimension){
			this->base_configuration = base_configuration;
			this->change_dimension = change_dimension;
			this->lowerBound = 0;
			this->upperBound = 100;
			this->nextGuess = 50;
}

//Method to be called that will updated lowerBound, upperBound, and nextGuess when new information
//has been collected from the user. The information processed will correspond to the point specified
//by the prior value of nextGuess. If the method is passed True, the this point was correctly identified
//and the value of lowerBound is replaced by nextGuess's value. If the point was incorrectly identified,
//the value of upperBound is replaced by nextGuess's value. Finally, the new value of nextGuess must be computed
void QuerySet::processAnswer(bool answer){
	if(answer){
		lowerBound = nextGuess;
	}else{
		upperBound = nextGuess;
	}
	nextGuess = (int)floor((lowerBound + upperBound )/ 2.0);

};

//returns the name of the next image viewed
string QuerySet::getImageName(void){
	return	base_configuration + "-" +
			change_dimension + "_" +
			to_string(100-nextGuess) + "-" +
			to_string(nextGuess) + ".tif";
}