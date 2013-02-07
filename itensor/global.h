//
// Distributed under the ITensor Library License, Version 1.0.
//    (See accompanying LICENSE file.)
//
#ifndef __ITENSOR_GLOBAL_H
#define __ITENSOR_GLOBAL_H
#include <cmath>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <fstream>
#include <error.h> //utilities
#include "option.h"
#include "assert.h"
#include "boost/array.hpp"
#include "boost/format.hpp"

#include "boost/foreach.hpp"
#define Foreach BOOST_FOREACH

static const int NMAX = 8;
static const Real MIN_CUT = 1E-20;
static const int MAX_M = 5000;

// The PAUSE macro is useful for debugging. 
// Prints the current line number and pauses
// execution until the enter key is pressed.
#define PAUSE { std::cout << "(Paused, Line " << __LINE__ << ")"; std::cin.get(); }


#ifndef DEBUG

#ifndef NDEBUG
#define NDEBUG //turn off asserts
#endif

#ifndef BOOST_DISABLE_ASSERTS
#define BOOST_DISABLE_ASSERTS
#endif

#endif


#ifdef DEBUG
#define DO_IF_DEBUG(X) X
#else
#define DO_IF_DEBUG(X)
#endif


#ifdef DEBUG
#define GET(container,j) (container.at(j))
#else
#define GET(container,j) (container[j])
#endif	

enum Printdat { ShowData, HideData };

#define PrintEither(X,Y) \
    {\
    bool savep = Global::printdat();\
    Global::printdat() = Y; \
    std::cout << "\n" << #X << " =\n" << X << std::endl; \
    Global::printdat() = savep;\
    }
#define Print(X)    PrintEither(X,false)
#define PrintDat(X) PrintEither(X,true)
#define PrintIndices(T) { T.printIndices(#T); }


bool inline
fileExists(const std::string& fname)
    {
    std::ifstream file(fname.c_str());
    return file.good();
    }
bool inline
fileExists(const boost::format& fname)
    {
    return fileExists(fname.str());
    }

void inline
writeVec(std::ostream& s, const Vector& V)
    {
    int m = V.Length();
    s.write((char*)&m,sizeof(m));
    Real val;
    for(int k = 1; k <= m; ++k)
        {
        val = V(k);
        s.write((char*)&val,sizeof(val));
        }
    }

void inline
readVec(std::istream& s, Vector& V)
    {
    int m = 1;
    s.read((char*)&m,sizeof(m));
    V.ReDimension(m);
    Real val;
    for(int k = 1; k <= m; ++k)
        {
        s.read((char*)&val,sizeof(val));
        V(k) = val;
        }
    }

template<class T> 
void inline
readFromFile(const std::string& fname, T& t) 
    { 
    std::ifstream s(fname.c_str()); 
    if(!s.good()) 
        Error("Couldn't open file \"" + fname + "\" for reading");
    t.read(s); 
    s.close(); 
    }

template<class T> 
void inline
readFromFile(const boost::format& fname, T& t) 
    { 
    readFromFile(fname.str(),t);
    }

template<>
void inline
readFromFile(const std::string& fname, Matrix& t) 
    { 
    std::ifstream s(fname.c_str()); 
    if(!s.good()) 
        Error("Couldn't open file \"" + fname + "\" for reading");
    int Nr = 1;
    s.read((char*)&Nr,sizeof(Nr));
    Vector V;
    readVec(s,V);
    int Nc = V.Length()/Nr;
    t = Matrix(Nr,Nc);
    for(int r = 1; r <= Nr; ++r)
    for(int c = 1; c <= Nc; ++c)
        t(r,c) = V(r+Nr*(c-1));
    s.close(); 
    }

template<class T> 
void inline
writeToFile(const std::string& fname, const T& t) 
    { 
    std::ofstream s(fname.c_str()); 
    if(!s.good()) 
        Error("Couldn't open file \"" + fname + "\" for writing");
    t.write(s); 
    s.close(); 
    }

template<class T> 
void inline
writeToFile(const boost::format& fname, const T& t) 
    { 
    writeToFile(fname.str(),t); 
    }

template<>
void inline
writeToFile(const std::string& fname, const Matrix& t) 
    { 
    std::ofstream s(fname.c_str()); 
    if(!s.good()) 
        Error("Couldn't open file \"" + fname + "\" for writing");
    int Nr = t.Nrows();
    int Nc = t.Ncols();
    s.write((char*)&Nr,sizeof(Nr));
    Vector V(Nr*Nc);
    for(int r = 1; r <= Nr; ++r)
    for(int c = 1; c <= Nc; ++c)
        V(r+Nr*(c-1)) = t(r,c);
    writeVec(s,V);
    s.close(); 
    }

//Given a prefix (e.g. pfix == "mydir")
//and an optional location (e.g. locn == "/var/tmp/")
//creates a temporary directory and returns its name
//without a trailing slash
//(e.g. /var/tmp/mydir_SfqPyR)
std::string inline
mkTempDir(const std::string& pfix,
          const std::string& locn = "./")
    {
    //Construct dirname
    std::string dirname = locn;
    if(dirname[dirname.length()-1] != '/')
        dirname += '/';
    //Add prefix and template string of X's for mkdtemp
    dirname += pfix + "_XXXXXX";

    //Create C string version of dirname
    char* cstr;
    cstr = new char[dirname.size()+1];
    strcpy(cstr,dirname.c_str());

    //Call mkdtemp
    char* retval = mkdtemp(cstr);
    //Check error condition
    if(retval == NULL)
        {
        delete[] cstr;
        throw ITError("mkTempDir failed");
        }

    //Prepare return value
    std::string final_dirname(retval);

    //Clean up
    delete[] cstr;

    return final_dirname;
    }


/*
*
* The Arrow enum is used to label how indices
* transform under a particular symmetry group. 
* Indices with an Out Arrow transform as vectors
* (kets) and with an In Arrow as dual vectors (bras).
*
* Conventions regarding arrows:
*
* * Arrows point In or Out, never right/left/up/down.
*
* * The Site indices of an MPS representing a ket point Out.
*
* * Conjugation switches arrow directions.
*
* * All arrows flow Out from the ortho center of an MPS 
*   (assuming it's a ket - In if it's a bra).
*
* * IQMPOs are created with the same arrow structure as if they are 
*   orthogonalized to site 1, but this is just a default since they 
*   aren't actually ortho. If position is called on an IQMPO it follows 
*   the same convention as for an MPS except Site indices point In and 
*   Site' indices point Out.
*
* * Local site operators have two IQIndices, one unprimed and pointing In, 
*   the other primed and pointing Out.
*
*/

enum Arrow { In = -1, Out = 1 };

Arrow inline
operator*(const Arrow& a, const Arrow& b)
    { 
    return (int(a)*int(b) == int(In)) ? In : Out; 
    }
const Arrow Switch = In*Out;

inline std::ostream& 
operator<<(std::ostream& s, Arrow D)
    { 
    s << (D == In ? "In" : "Out");
    return s; 
    }

////////
///////


class Global
    {
    public:

    static bool& 
    printdat()
        {
        static bool printdat_ = false;
        return printdat_;
        }
    static Real& 
    printScale()
        {
        static Real printScale_ = 1E-10;
        return printScale_;
        }
    static bool& 
    debug1()
        {
        static bool debug1_ = false;
        return debug1_;
        }
    static bool& 
    debug2()
        {
        static bool debug2_ = false;
        return debug2_;
        }
    static bool& 
    debug3()
        {
        static bool debug3_ = false;
        return debug3_;
        }
    static bool& 
    debug4()
        {
        static bool debug4_ = false;
        return debug4_;
        }
    static Vector& 
    lastd()
        {
        static Vector lastd_(1);
        return lastd_;
        }
    static bool& 
    checkArrows()
        {
        static bool checkArrows_ = true;
        return checkArrows_;
        }
    static OptSet&
    opts()
        {
        return OptSet::GlobalOpts();
        }
    };


class ResultIsZero : public ITError
    {
    public:

    typedef ITError
    Parent;

    ResultIsZero(const std::string& message) 
        : Parent(message)
        { }
    };

class ArrowError : public ITError
    {
    public:

    typedef ITError
    Parent;

    ArrowError(const std::string& message) 
        : Parent(message)
        { }
    };

Real ran1();

//void reportnew() { }

#endif