///////////////////////////////////////////////////////////////////////////////
///
/// \file ExtendableMath.hpp
/// 
/// Authors: Joshua Davis
/// Copyright 2014, DigiPen Institute of Technology
///
//////////////////////////////////////////////////////////////////////////////
#pragma once

namespace Math
{

//-------------------------------------------------------------------ExtendableVector
struct ZeroShared ExtendableVector
{
  void Resize(size_t size);

  real& operator[](size_t index);
  real operator[](size_t index) const;

  size_t GetSize() const;

  size_t mSize;
  Zero::Array<real> mData;
};

//-------------------------------------------------------------------ExtendableMatrix
struct ZeroShared ExtendableMatrix
{
  void Resize(size_t sizeX, size_t sizeY);

  real& operator()(size_t y, size_t x);
  real operator()(size_t y, size_t x) const;

  size_t mSizeX;
  size_t mSizeY;
  Zero::Array<real> mData;
};

//-------------------------------------------------------------------FixedVector
// A vector who's max size is compile-time but who's
// working size can be changed up to the fixed size.
// Currently used in position correction.
template <typename DataType, size_t FixedSize>
struct ZeroSharedTemplate FixedVector
{
  void Resize(size_t size)
  {
    if(size > FixedSize)
    {
      Error("Cannot set size greater than the fixed size.");
      size = FixedSize;
    }

    mSize = size;
  }

  DataType& operator[](size_t index)
  {
    ErrorIf(index >= mSize, "Access array out of bounds");
    return mData[index];
  }
  DataType operator[](size_t index) const
  {
    ErrorIf(index >= mSize, "Access array out of bounds");
    return mData[index];
  }

  size_t GetSize() const
  {
    return mSize;
  }

  size_t mSize;
  DataType mData[FixedSize];
};

//-------------------------------------------------------------------FixedMatrix
template <size_t SizeX, size_t SizeY>
struct ZeroSharedTemplate FixedMatrix
{
  real& operator()(size_t y, size_t x)
  {
    ErrorIf(y > SizeY || x > SizeX, "Access matrix out of bounds");
    return mData[x + SizeX * y];
  }
  real operator()(size_t y, size_t x) const
  {
    ErrorIf(y > SizeY || x > SizeX, "Access matrix out of bounds");
    return mData[x + SizeX * y];
  }

  real mData[SizeX * SizeY];
};

}//namespace Math
