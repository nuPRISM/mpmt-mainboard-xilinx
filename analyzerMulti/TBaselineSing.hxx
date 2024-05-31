#ifndef TBaselineSing_h
#define TBaselineSing_h

#include <vector>
#include <array>
#include "odbxx.h"

// Singleton for saving baselines
class BSingleton
{

protected:
  BSingleton()
  {
    baselines = std::vector<double>(100,0);
  }

  static BSingleton* singleton_;

  std::vector<double> baselines;

  

public:

  BSingleton(BSingleton &other) = delete;

  void operator=(const BSingleton &) = delete;

  static BSingleton *GetInstance();

  double GetBaseline(int i){
    if(i < 0 || i >= baselines.size()) return 0;
    return baselines[i];
  }

  void UpdateBaselines(){

    midas::odb o2 = {
      {"Baseline2", std::array<double, 100>{} }
    };
    o2.connect("/Analyzer/Baselines2");
    for(int i = 0; i < 100; i++){
      baselines[i] = o2["Baseline2"][i];
    }
    std::cout << "Get baselines from ODB: First 4 baselines: " 
	      << baselines[0] << "  "
	      << baselines[1] << "  "
	      << baselines[2] << "  "
	      << baselines[3] << std::endl;
    printf("Finished update baselines \n");
  }

};




#endif
