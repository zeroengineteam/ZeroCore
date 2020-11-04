#include "Precompiled.hpp"

namespace Zero
{

//-------------------------------------------------------------------DebugDrawStep
void DebugDrawStep::Draw()
{
#define ZeroDebugPrimitive(DebugType) \
  for(size_t i = 0; i < m##DebugType##List.Size(); ++i) \
    gDebugDraw->Add(m##DebugType##List[i]);
#include "DebugPrimitives.inl"
#undef ZeroDebugPrimitive
}

//-------------------------------------------------------------------DebugDrawStack
void DebugDrawStack::Add(const DebugDrawStep& step)
{
  mSteps.PushBack(step);
}

void DebugDrawStack::Clear()
{
  mSteps.Clear();
}

void DebugDrawStack::Draw(int index)
{
  // Guard against an empty list
  size_t size = mSteps.Size();
  if(size == 0)
    return;

  // Fix the index to always be within the range of
  // the number of steps, including negative numbers.
  size_t actualIndex = (index % size + size) % size;
  // Draw the given step
  mSteps[actualIndex].Draw();
}

}//namespace Zero
