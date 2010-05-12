//----------------------------------------------------------------------------------//
// OgreRecast Demo - A demonstration of integrating Recast Navigation Meshes		//
//					 with the Ogre3D rendering engine.								//
//																					//
//	This file was either Created by or Modified by :								//
//													Paul A Wilson 					//
//			All contents are Copyright (C) 2010 Paul A Wilson						//
//			Except where otherwise mentioned or where previous						//
//			copyright exists. In the case of pre-existing copyrights				//
//			all rights remain with the original Authors, and this is				//
//			to be considered a derivative work.										//
//																					//
//	Contact Email	:	paulwilson77@dodo.com.au									//
//																					//
// This 'SOFTWARE' is provided 'AS-IS', without any express or implied				//
// warranty.  In no event will the authors be held liable for any damages			//
// arising from the use of this software.											//
// Permission is granted to anyone to use this software for any purpose,			//
// including commercial applications, and to alter it and redistribute it			//
// freely, subject to the following restrictions:									//
// 1. The origin of this software must not be misrepresented; you must not			//
//    claim that you wrote the original software. If you use this software			//
//    in a product, an acknowledgment in the product documentation would be			//
//    appreciated but is not required.												//
// 2. Altered source versions must be plainly marked as such, and must not be		//
//    misrepresented as being the original software.								//
// 3. This notice may not be removed or altered from any source distribution.		//
//																					//
//----------------------------------------------------------------------------------//

//------------------------------------------------------------------------
//
//  Name: utils.h
//
//  Desc: misc utility functions and constants
//
//  Author: Original : Mat Buckland (fup@ai-junkie.com) 
//  Modifications	 : Paul A Wilson (paulwilson77@dodo.com.au)
//
//------------------------------------------------------------------------


#ifndef __H_MISCUTILS_H_
#define __H_MISCUTILS_H_


#include <math.h>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <cassert>
#include <iomanip>



//a few useful constants
const int     MaxInt    = (std::numeric_limits<int>::max)();
const double  MaxDouble = (std::numeric_limits<double>::max)();
const double  MinDouble = (std::numeric_limits<double>::min)();
const float   MaxFloat  = (std::numeric_limits<float>::max)();
const float   MinFloat  = (std::numeric_limits<float>::min)();

const double   Pi        = 3.14159;
const double   TwoPi     = Pi * 2;
const double   HalfPi    = Pi / 2;
const double   QuarterPi = Pi / 4;

//returns true if the value is a NaN
template <typename T>
inline bool isNaN(T val)
{
	return val != val;
}

inline double DegsToRads(double degs)
{
	return TwoPi * (degs/360.0);
}



//returns true if the parameter is equal to zero
inline bool IsZero(double val)
{
	return ( (-MinDouble < val) && (val < MinDouble) );
}

//returns true is the third parameter is in the range described by the
//first two
inline bool InRange(double start, double end, double val)
{
	if (start < end)
	{
		if ( (val > start) && (val < end) ) return true;
		else return false;
	}

	else
	{
		if ( (val < start) && (val > end) ) return true;
		else return false;
	}
}

template <class T>
T Maximum(const T& v1, const T& v2)
{
	return v1 > v2 ? v1 : v2;
}



//----------------------------------------------------------------------------
//  some random number functions.
//----------------------------------------------------------------------------

//returns a random integer between x and y
inline int   RandInt(int x,int y)
{
	assert(y>=x && "<RandInt>: y is less than x");
	return rand()%(y-x+1)+x;
}

//returns a random double between zero and 1
inline double RandFloat()      {return ((rand())/(RAND_MAX+1.0));}

inline double RandInRange(double x, double y)
{
	return x + RandFloat()*(y-x);
}

//returns a random bool
inline bool   RandBool()
{
	if (RandFloat() > 0.5) return true;

	else return false;
}

//returns a random double in the range -1 < n < 1
inline double RandomClamped()    {return RandFloat() - RandFloat();}


//returns a random number with a normal distribution. See method at
//http://www.taygeta.com/random/gaussian.html
inline double RandGaussian(double mean = 0.0, double standard_deviation = 1.0)
{				        
	double x1, x2, w, y1;
	static double y2;
	static int use_last = 0;

	if (use_last)		        /* use value from previous call */
	{
		y1 = y2;
		use_last = 0;
	}
	else
	{
		do 
		{
			x1 = 2.0 * RandFloat() - 1.0;
			x2 = 2.0 * RandFloat() - 1.0;
			w = x1 * x1 + x2 * x2;
		}
		while ( w >= 1.0 );

		w = sqrt( (-2.0 * log( w ) ) / w );
		y1 = x1 * w;
		y2 = x2 * w;
		use_last = 1;
	}

	return( mean + y1 * standard_deviation );
}



//-----------------------------------------------------------------------
//  
//  some handy little functions
//-----------------------------------------------------------------------


inline double Sigmoid(double input, double response = 1.0)
{
	return ( 1.0 / ( 1.0 + exp(-input / response)));
}


//returns the maximum of two values
template <class T>
inline T MaxOf(const T& a, const T& b)
{
	if (a>b) return a; return b;
}

//returns the minimum of two values
template <class T>
inline T MinOf(const T& a, const T& b)
{
	if (a<b) return a; return b;
}


//clamps the first argument between the second two
template <class T, class U, class V>
inline void Clamp(T& arg, const U& minVal, const V& maxVal)
{
	assert ( ((double)minVal < (double)maxVal) && "<Clamp>MaxVal < MinVal!");

	if (arg < (T)minVal)
	{
		arg = (T)minVal;
	}

	if (arg > (T)maxVal)
	{
		arg = (T)maxVal;
	}
}


//rounds a double up or down depending on its value
inline int Rounded(double val)
{
	int    integral = (int)val;
	double mantissa = val - integral;

	if (mantissa < 0.5)
	{
		return integral;
	}

	else
	{
		return integral + 1;
	}
}

//rounds a double up or down depending on whether its 
//mantissa is higher or lower than offset
inline int RoundUnderOffset(double val, double offset)
{
	int    integral = (int)val;
	double mantissa = val - integral;

	if (mantissa < offset)
	{
		return integral;
	}

	else
	{
		return integral + 1;
	}
}

//compares two real numbers. Returns true if they are equal
inline bool isEqual(float a, float b)
{
	if (fabs(a-b) < 1E-12)
	{
		return true;
	}

	return false;
}

inline bool isEqual(double a, double b)
{
	if (fabs(a-b) < 1E-12)
	{
		return true;
	}

	return false;
}


template <class T>
inline double Average(const std::vector<T>& v)
{
	double average = 0.0;

	for (unsigned int i=0; i < v.size(); ++i)
	{    
		average += (double)v[i];
	}

	return average / (double)v.size();
}


inline double StandardDeviation(const std::vector<double>& v)
{
	double sd      = 0.0;
	double average = Average(v);

	for (unsigned int i=0; i<v.size(); ++i)
	{     
		sd += (v[i] - average) * (v[i] - average);
	}

	sd = sd / v.size();

	return sqrt(sd);
}


template <class container>
inline void DeleteSTLContainer(container& c)
{
	for (container::iterator it = c.begin(); it!=c.end(); ++it)
	{
		delete *it;
		*it = NULL;
	}
}

template <class map>
inline void DeleteSTLMap(map& m)
{
	for (map::iterator it = m.begin(); it!=m.end(); ++it)
	{
		delete it->second;
		it->second = NULL;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class Smoother
{
private:

	//this holds the history
	std::vector<T>  m_History;

	int           m_iNextUpdateSlot;

	//an example of the 'zero' value of the type to be smoothed. This
	//would be something like Vector2D(0,0)
	T             m_ZeroValue;

public:

	//to instantiate a Smoother pass it the number of samples you want
	//to use in the smoothing, and an exampe of a 'zero' type
	Smoother(int SampleSize, T ZeroValue):m_History(SampleSize, ZeroValue),
		m_ZeroValue(ZeroValue),
		m_iNextUpdateSlot(0)
	{}

	//each time you want to get a new average, feed it the most recent value
	//and this method will return an average over the last SampleSize updates
	T Update(const T& MostRecentValue)
	{  
		//overwrite the oldest value with the newest
		m_History[m_iNextUpdateSlot++] = MostRecentValue;

		//make sure m_iNextUpdateSlot wraps around. 
		if (m_iNextUpdateSlot == m_History.size()) m_iNextUpdateSlot = 0;

		//now to calculate the average of the history list
		T sum = m_ZeroValue;

		std::vector<T>::iterator it = m_History.begin();

		for (it; it != m_History.end(); ++it)
		{
			sum += *it;
		}

		return sum / (double)m_History.size();
	}
};



////////////////////////////////////////////////////////////////////////////////////
// STREAM UTILITY METHODS

//------------------------------ ttos -----------------------------------------
//
//  convert a type to a string
//-----------------------------------------------------------------------------
template <class T>
inline std::string ttos(const T& t, int precision = 2)
{
	std::ostringstream buffer;

	buffer << std::fixed << std::setprecision(precision) << t;

	return buffer.str();
}

//------------------------------ ttos -----------------------------------------
//
//  convert a bool to a string
//-----------------------------------------------------------------------------
inline std::string btos(bool b)
{
	if (b) return "true";
	return "false";
}

//--------------------------- GetValueFromStream ------------------------------
//
//  grabs a value of the specified type from an input stream
//-----------------------------------------------------------------------------
template <typename T>
inline T GetValueFromStream(std::ifstream& stream)
{
	T val;

	stream >> val;

	//make sure it was the correct type
	if (!stream)
	{
		throw std::runtime_error("Attempting to retrieve wrong type from stream");
	}

	return val;
}

//--------------------------- WriteBitsToStream ------------------------------------
//
// writes the value as a binary string of bits
//-----------------------------------------------------------------------------
template <typename T>
void WriteBitsToStream(std::ostream& stream, const T& val)
{
	int iNumBits = sizeof(T) * 8;

	while (--iNumBits >= 0)
	{
		if ((iNumBits+1) % 8 == 0) stream << " ";
		unsigned long mask = 1 << iNumBits;
		if (val & mask) stream << "1";
		else stream << "0";
	}
}






#endif // __H_MISCUTILS_H_