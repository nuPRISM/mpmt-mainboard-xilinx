
#include "TBaselineSing.hxx"

BSingleton* BSingleton::singleton_= nullptr;;

/**                                                                                                                                                                    
 * Static methods should be defined outside the class.                                                                                                                 
 */
BSingleton *BSingleton::GetInstance()
{

  if(singleton_==nullptr){
    singleton_ = new BSingleton();
  }
  return singleton_;
}

