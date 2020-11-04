///////////////////////////////////////////////////////////////////////////////
///
/// \file ExtendableMath.cpp
/// 
/// Authors: Joshua Davis
/// Copyright 2014, DigiPen Institute of Technology
///
//////////////////////////////////////////////////////////////////////////////
#include "Precompiled.hpp"

namespace Math
{

//-------------------------------------------------------------------ExtendableVector
void ExtendableVector::Resize(size_t size)
{
  mSize = size;
  mData.Resize(mSize);
}

real& ExtendableVector::operator[](size_t index)
{
  return mData[index];
}

real ExtendableVector::operator[](size_t index) const
{
  return mData[index];
}

size_t ExtendableVector::GetSize() const
{
  return mSize;
}

//-------------------------------------------------------------------ExtendableMatrix
void ExtendableMatrix::Resize(size_t sizeX, size_t sizeY)
{
  mSizeX = sizeX;
  mSizeY = sizeY;
  mData.Resize(mSizeX * mSizeY);
}

real& ExtendableMatrix::operator()(size_t y, size_t x)
{
  return mData[x + mSizeX * y];
}

real ExtendableMatrix::operator()(size_t y, size_t x) const
{
  return mData[x + mSizeX * y];
}

}//namespace Math
