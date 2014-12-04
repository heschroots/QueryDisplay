#include <string>

class QuerySet
{
   public:
	  std::string base_configuration;
	  std::string change_dimension;
      int lowerBound;  // 
      int upperBound;  //
	  int nextGuess;   //
	  QuerySet(std::string base_configuration, std::string change_dimension);

	  std::string getImageName();
	  void processAnswer(bool answer);
};