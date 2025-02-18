#ifdef __CLING__

#pragma link off all globals;
#pragma link off all classes;
#pragma link off all functions;

#pragma link C++ nestedclass;
#pragma link C++ nestedtypedef;

#pragma link C++ class ActPhysics::Particle;
#pragma link C++ class ActPhysics::Kinematics;
#pragma link C++ class ActPhysics::SRIM;

#pragma link C++ enum class ActPhysics::SilSide + ;
#pragma link C++ class ActPhysics::SilUnit;
#pragma link C++ class ActPhysics::SilLayer;
#pragma link C++ class ActPhysics::SilSpecs;

#pragma link C++ class ActPhysics::PIDCorrection + ;
#pragma link C++ class ActPhysics::PIDCorrector;

#pragma link C++ class ActPhysics::SilMatrix + ;

#pragma link C++ class ActPhysics::Gas;

#pragma link C++ class ActPhysics::GenCorrection + ;

#endif
