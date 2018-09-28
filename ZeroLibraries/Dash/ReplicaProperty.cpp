///////////////////////////////////////////////////////////////////////////////
///
/// Authors: Andrew Colean
/// Copyright 2015, DigiPen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#include "Precompiled.hpp"

namespace Zero
{

//
// Helper Functions
//

/// Point Sort Policy
/// Used to sort points on the received value curve by their timestamp (x value)
template<typename ValueType>
struct PointSortPolicy : public SortPolicy<ValueType>
{
  typedef SortPolicy<ValueType> base_type;
  typedef typename base_type::value_type value_type;

  /// Compares the X values of each point
  template<typename CompareType>
  bool operator()(const value_type& lhs, const CompareType& rhs) const
  {
    return lhs.x < rhs;
  }
  bool operator()(const value_type& lhs, const value_type& rhs) const
  {
    return lhs.x < rhs.x;
  }

  template<typename CompareType>
  bool equal(const value_type& lhs, const CompareType& rhs) const
  {
    return lhs.x == rhs;
  }
  bool equal(const value_type& lhs, const value_type& rhs) const
  {
    return lhs.x == rhs.x;
  }
};
typedef ArraySet< Math::Vector3, PointSortPolicy<Math::Vector3> > PointSet;

/// NonZeroAbs: Floating-point specialization
/// Returns the absolute value, unless it's 0, in which case returns a small positive increment
template<typename T, TF_ENABLE_IF(is_floating_point<T>::value)>
inline T NonZeroAbs(T value)
{
  T result = Math::Abs(value);
  if(result <= T(0))
    return T(Math::Epsilon() * 10);
  else
    return result;
}
/// NonZeroAbs: Integral specialization
/// Returns the absolute value, unless it's 0, in which case returns a small positive increment
template<typename T, TF_ENABLE_IF(is_integral<T>::value)>
inline T NonZeroAbs(T value)
{
  T result = Math::Abs(value);
  if(result <= T(0))
    return T(1);
  else
    return result;
}

/// Converge: Floating-point specialization
/// Linearly interpolates currentValue toward targetValue, ensuring progress is made
template<typename T, TF_ENABLE_IF(is_floating_point<T>::value)>
inline T Converge(T currentValue, T targetValue, float targetWeight)
{
  // (Target weight must be clamped between 0 and 1)
  Assert(0 <= targetWeight && targetWeight <= 1);

  // Linearly interpolate the value
  T lerpedValue = Math::Lerp(currentValue, targetValue, targetWeight);
  return lerpedValue;
}
/// Converge: Integral specialization
/// Linearly interpolates currentValue toward targetValue, ensuring progress is made
template<typename T, TF_ENABLE_IF(is_integral<T>::value)>
inline T Converge(T currentValue, T targetValue, float targetWeight)
{
  // (Target weight must be clamped between 0 and 1)
  Assert(0 <= targetWeight && targetWeight <= 1);

  // Linearly interpolate the value as double and round the result back
  T lerpedValue = T(Round(Math::Lerp(double(currentValue), double(targetValue), targetWeight)));

  // Unable to make any progress?
  if(lerpedValue == currentValue)
  {
    // Progress by the smallest amount possible towards target value
    // (This solves the problem of integral values "never reaching" their target value)
    if(lerpedValue < targetValue)
      ++lerpedValue;
    else if(lerpedValue > targetValue)
      --lerpedValue;
  }

  return lerpedValue;
}

//---------------------------------------------------------------------------------//
//                              ReplicaProperty                                    //
//---------------------------------------------------------------------------------//

ReplicaProperty::ReplicaProperty(const String& name, ReplicaPropertyType* replicaPropertyType, const Variant& propertyData)
  : mName(name),
    mReplicaPropertyType(replicaPropertyType),
    mReplicaChannel(nullptr),
    mIndexListLink(),
    mIndexListSize(nullptr),
    mPropertyData(propertyData),
    mLastValue(),
    mLastChangeTimestamp(cInvalidMessageTimestamp),
    mLastReceivedChangeValue(),
    mLastReceivedChangeTimestamp(cInvalidMessageTimestamp),
    mLastReceivedChangeFrameId(0),
    mSplineCurve(),
    mBakedCurve(),
    mConvergenceState(ConvergenceState::None)
{
  // Configure spline curves
  for(size_t i = 0; i < 4; ++i)
  {
    mSplineCurve[i].SetCurveType(replicaPropertyType->GetInterpolationCurve());
    mSplineCurve[i].SetClosed(false);
  }
}

ReplicaProperty::~ReplicaProperty()
{
  // (Should have been unscheduled when the operating replica was made invalid,
  // else there is a dangling replica property held by the replica property type)
  Assert(!IsScheduled());
}

bool ReplicaProperty::operator ==(const ReplicaProperty& rhs) const
{
  return mName == rhs.mName;
}
bool ReplicaProperty::operator !=(const ReplicaProperty& rhs) const
{
  return mName != rhs.mName;
}
bool ReplicaProperty::operator  <(const ReplicaProperty& rhs) const
{
  return mName < rhs.mName;
}
bool ReplicaProperty::operator ==(const String& rhs) const
{
  return mName == rhs;
}
bool ReplicaProperty::operator !=(const String& rhs) const
{
  return mName != rhs;
}
bool ReplicaProperty::operator  <(const String& rhs) const
{
  return mName < rhs;
}

//
// Operations
//

const String& ReplicaProperty::GetName() const
{
  return mName;
}

ReplicaPropertyType* ReplicaProperty::GetReplicaPropertyType() const
{
  return mReplicaPropertyType;
}
NativeType* ReplicaProperty::GetNativeType() const
{
  return mReplicaPropertyType->GetNativeType();
}
NativeTypeId ReplicaProperty::GetNativeTypeId() const
{
  return mReplicaPropertyType->GetNativeTypeId();
}

bool ReplicaProperty::IsValid() const
{
  return (GetReplicator() != nullptr);
}

Replicator* ReplicaProperty::GetReplicator() const
{
  ReplicaChannel* replicaChannel = GetReplicaChannel();
  return replicaChannel ? replicaChannel->GetReplicator() : nullptr;
}

Replica* ReplicaProperty::GetReplica() const
{
  ReplicaChannel* replicaChannel = GetReplicaChannel();
  return replicaChannel ? replicaChannel->GetReplica() : nullptr;
}

void ReplicaProperty::SetReplicaChannel(ReplicaChannel* replicaChannel)
{
  mReplicaChannel = replicaChannel;
}
ReplicaChannel* ReplicaProperty::GetReplicaChannel() const
{
  return mReplicaChannel;
}

void ReplicaProperty::SetConvergenceState(ConvergenceState::Enum convergenceState)
{
  // Convergence state has changed?
  ConvergenceState::Enum lastConvergenceState = mConvergenceState;
  if(lastConvergenceState != convergenceState)
  {
    // Unschedule replica property for change convergence (if needed)
    GetReplicaPropertyType()->UnscheduleProperty(this);

    // Set convergence state
    mConvergenceState = convergenceState;

    // Using change convergence?
    if(mConvergenceState != ConvergenceState::None)
    {
      // Schedule replica property for change convergence
      GetReplicaPropertyType()->ScheduleProperty(this);
    }

    // TODO: Dispatch convergence state change events here
  }
}
ConvergenceState::Enum ReplicaProperty::GetConvergenceState() const
{
  return mConvergenceState;
}

bool ReplicaProperty::IsScheduled() const
{
  return mIndexListSize != nullptr;
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic HasChanged Function Type
typedef bool (*HasChangedFn)(const ReplicaProperty* replicaProperty);

/// HasChangedFn: Standard property type behavior
bool HasChangedStandard(const ReplicaProperty* replicaProperty)
{
  // Perform standard inequality comparison
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();
  return (currentValue != lastValue);
}

/// HasChangedFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
bool HasChangedCustom(const ReplicaProperty* replicaProperty)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get current and last property values for comparison
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();

  // Not enough property values to perform an arithmetic comparison?
  if(currentValue.IsEmpty()
  || lastValue.IsEmpty())
  {
    // Perform standard inequality comparison instead
    return (currentValue != lastValue);
  }

  // Use delta threshold comparison?
  if(replicaPropertyType->GetUseDeltaThreshold())
  {
    // Get delta threshold value for comparison
    const Variant& deltaThreshold = replicaPropertyType->GetDeltaThreshold();

    // (Delta threshold value should be non-empty)
    Assert(deltaThreshold.IsNotEmpty());

    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive members
      PrimitiveType&       currentValuePrimitiveMember   = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& lastValuePrimitiveMember      = lastValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& deltaThresholdPrimitiveMember = deltaThreshold.GetPrimitiveMemberOrError<PropertyType>(i);

      // Current value and last value primitive members differ by more than the delta threshold value primitive member?
      if(Math::Abs(currentValuePrimitiveMember - lastValuePrimitiveMember) > deltaThresholdPrimitiveMember)
      {
        // Has changed
        return true;
      }
    }
  }
  // Do not use delta threshold?
  else
  {
    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive members
      PrimitiveType&       currentValuePrimitiveMember = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& lastValuePrimitiveMember    = lastValue.GetPrimitiveMemberOrError<PropertyType>(i);

      // Current value and last value primitive members differ?
      if(currentValuePrimitiveMember != lastValuePrimitiveMember)
      {
        // Has changed
        return true;
      }
    }
  }

  // Has not changed
  return false;
}
// ------------------------------------------------------------------------------------------------------------------------------

bool ReplicaProperty::HasChanged() const
{
  // Determine the appropriate HasChanged function to use
  HasChangedFn hasChangedFn = nullptr;

  // Use the custom HasChanged function for our property type if one exists, else use the standard HasChanged function
  hasChangedFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                HasChangedCustom, HasChangedFn, HasChangedStandard);

  // Check if the property value has changed
  return hasChangedFn(this);
}

bool ReplicaProperty::HasChangedAtAll() const
{
  // Perform standard inequality comparison
  Variant        currentValue = GetValue();
  const Variant& lastValue    = GetLastValue();
  return (currentValue != lastValue);
}

void ReplicaProperty::SetValue(const Variant& value)
{
  Assert(value.IsNotEmpty());

  // Set current property value
  mReplicaPropertyType->GetSetValueFn()(value, mPropertyData);
}
Variant ReplicaProperty::GetValue() const
{
  // Get current property value
  Variant value = mReplicaPropertyType->GetGetValueFn()(mPropertyData);
  if(value.IsEmpty()) // Unable?
  {
    // Get last property value
    return GetLastValue();
  }

  // Return current property value
  return ZeroMove(value);
}

const Variant& ReplicaProperty::GetPropertyData() const
{
  return mPropertyData;
}

void ReplicaProperty::SetLastValue(MoveReference<Variant> value)
{
  mLastValue = ZeroMove(value);
}
const Variant& ReplicaProperty::GetLastValue() const
{
  return mLastValue;
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic UpdateLastValue Function Type
typedef void (*UpdateLastValueFn)(ReplicaProperty* replicaProperty);

/// UpdateLastValueFn: Standard property type behavior
void UpdateLastValueStandard(ReplicaProperty* replicaProperty)
{
  // Set last value verbatim
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();
  return replicaProperty->SetLastValue(ZeroMove(currentValue));
}

/// UpdateLastValueFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
void UpdateLastValueCustom(ReplicaProperty* replicaProperty)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get current and last property values for comparison
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();

  // Get serialization settings
  SerializationMode::Enum serializationMode = replicaPropertyType->GetSerializationMode();

  // (Current and last values should be non-empty)
  Assert(currentValue.IsNotEmpty());
  Assert(lastValue.IsNotEmpty());

  //    Serialize all primitive members?
  // OR Do not use delta threshold?
  if(serializationMode == SerializationMode::All
  || !replicaPropertyType->GetUseDeltaThreshold())
  {
    // Set last value verbatim instead
    return replicaProperty->SetLastValue(ZeroMove(currentValue));
  }
  // Serialize only the primitive members that have changed?
  else
  {
    Assert(serializationMode == SerializationMode::Changed);

    // Get delta threshold value for comparison
    const Variant& deltaThreshold = replicaPropertyType->GetDeltaThreshold();

    // (Delta threshold value should be non-empty)
    Assert(deltaThreshold.IsNotEmpty());

    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive members
      PrimitiveType&       currentValuePrimitiveMember   = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& lastValuePrimitiveMember      = lastValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& deltaThresholdPrimitiveMember = deltaThreshold.GetPrimitiveMemberOrError<PropertyType>(i);

      // Has this primitive member changed?
      // (Current value and last value primitive members differ by more than the delta threshold value primitive member?)
      bool hasChanged = (Math::Abs(currentValuePrimitiveMember - lastValuePrimitiveMember) > deltaThresholdPrimitiveMember);

      // Has not changed?
      if(!hasChanged)
      {
        // Reset current value's primitive member to our last value's primitive member
        // (Our intent is to only update the new last value's primitive members to our current value's primitive members
        // once the primitive member in question has exceeded it's delta threshold and is therefore considered changed)
        currentValuePrimitiveMember = lastValuePrimitiveMember;
      }
    }

    // Perform last value update with our modified current value
    return replicaProperty->SetLastValue(ZeroMove(currentValue));
  }
}
// ------------------------------------------------------------------------------------------------------------------------------

void ReplicaProperty::UpdateLastValue(bool forceAll)
{
  // Determine the appropriate UpdateLastValue function to use
  UpdateLastValueFn updateLastValueFn = nullptr;

  // Force update all primitive-components in the last property value?
  if(forceAll)
  {
    // Use the standard UpdateLastValue function (which just sets the value verbatim)
    updateLastValueFn = UpdateLastValueStandard;
  }
  else
  {
    // Use the custom UpdateLastValue function for our property type if one exists, else use the standard UpdateLastValue function
    updateLastValueFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                       UpdateLastValueCustom, UpdateLastValueFn, UpdateLastValueStandard);
  }

  // Update the property's last value
  return updateLastValueFn(this);
}

void ReplicaProperty::SetLastChangeTimestamp(TimeMs lastChangeTimestamp)
{
  mLastChangeTimestamp = lastChangeTimestamp;
}
TimeMs ReplicaProperty::GetLastChangeTimestamp() const
{
  return mLastChangeTimestamp;
}

void ReplicaProperty::SetLastReceivedChangeValue(const Variant& value)
{
  mLastReceivedChangeValue = value;
}
const Variant& ReplicaProperty::GetLastReceivedChangeValue() const
{
  return mLastReceivedChangeValue;
}

void ReplicaProperty::SetLastReceivedChangeTimestamp(TimeMs lastReceivedChangeTimestamp)
{
  mLastReceivedChangeTimestamp = lastReceivedChangeTimestamp;
}
TimeMs ReplicaProperty::GetLastReceivedChangeTimestamp() const
{
  return mLastReceivedChangeTimestamp;
}

void ReplicaProperty::SetLastReceivedChangeFrameId(uint64 lastReceivedChangeFrameId)
{
  mLastReceivedChangeFrameId = lastReceivedChangeFrameId;
}
uint64 ReplicaProperty::GetLastReceivedChangeFrameId() const
{
  return mLastReceivedChangeFrameId;
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic UpdateCurve Function Type
typedef void (*UpdateCurveFn)(ReplicaProperty* replicaProperty, TimeMs timestamp, const Variant& value);

/// UpdateCurveFn: Standard property type behavior
void UpdateCurveStandard(ReplicaProperty* replicaProperty, TimeMs timestamp, const Variant& value)
{
  UnusedParameter(replicaProperty);
  UnusedParameter(timestamp);
  UnusedParameter(value);

  Error("Unable to update property value curve - Unexpected property type");
}

/// UpdateCurveFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
void UpdateCurveCustom(ReplicaProperty* replicaProperty, TimeMs timestamp, const Variant& value)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get new point timestamp in seconds
  float newPointTimestamp = TimeMsToFloatSeconds(timestamp);

  // Get current time (in seconds)
  float now = TimeMsToFloatSeconds(replicaPropertyType->GetReplicator()->GetPeer()->GetLocalTime());

  // Determine min and max timestamps (in seconds)
  float minTimestamp = now - 1 /* second */;
  // float maxTimestamp = TimeMsToFloatSeconds(replicaProperty->GetLastReceivedChangeTimestamp() + replicaPropertyType->GetExtrapolationLimit());

  // New point timestamp is outside our current time range?
  if(newPointTimestamp < minTimestamp)
  {
    // Don't accept the point
    // (This can occur with high or spiked latency)
    return;
  }

  // For each primitive member
  for(size_t i = 0; i < PrimitiveCount; ++i)
  {
    // Get primitive member
    PrimitiveType& valuePrimitiveMember = value.GetPrimitiveMemberOrError<PropertyType>(i);

    // Get received change value curve points
    // (We treat this as a set to keep our points sorted by their timestamp (x value))
    PointSet& points = reinterpret_cast<PointSet&>(replicaProperty->mSplineCurve[i].ControlPoints);

    //
    // Remove Extraneous Points From Set
    //

    // Remove all points from the beginning forwards except for the last point less than the min timestamp
    while(points.Size() > 2)
    {
      // Get two points at a time
      PointSet::pointer p1 = (points.Data() + 0);
      PointSet::pointer p2 = (points.Data() + 1);

      // Point 1 (first) is less than the min timestamp?
      if(p1->x < minTimestamp)
      {
        // Point 2 (second) is also less than the min timestamp?
        if(p2->x < minTimestamp)
        {
          // Remove first point from set (it is no longer relevant)
          points.Erase(p1);
        }
        else
          break;
      }
      else
        break;
    }

    //
    // Add New Point To Set
    //

    // Create control point at specified timestamp
    Math::Vec3 point;
    point.x = newPointTimestamp;
    point.y = float(valuePrimitiveMember);
    point.z = 0;

    // Add control point to set
    points.Insert(point);

    //
    // Bake Curve
    //

    // Bake out spline curve to be sampled later
    replicaProperty->mBakedCurve[i].Bake(replicaProperty->mSplineCurve[i], 0.05);
  }
}
// ------------------------------------------------------------------------------------------------------------------------------

void ReplicaProperty::UpdateCurve(TimeMs timestamp, const Variant& value)
{
  // Determine the appropriate UpdateCurve function to use
  UpdateCurveFn updateCurveFn = nullptr;

  // Use the custom UpdateCurve function for our property type if one exists, else use the standard UpdateCurve function
  updateCurveFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                 UpdateCurveCustom, UpdateCurveFn, UpdateCurveStandard);

  // Update the property's value curve
  return updateCurveFn(this, timestamp, value);
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic SampleCurve Function Type
typedef Variant (*SampleCurveFn)(ReplicaProperty* replicaProperty, TimeMs timestamp);

/// SampleCurveFn: Standard property type behavior
Variant SampleCurveStandard(ReplicaProperty* replicaProperty, TimeMs timestamp)
{
  UnusedParameter(replicaProperty);
  UnusedParameter(timestamp);

  Error("Unable to sample property value curve - Unexpected property type");
  return Variant();
}

/// SampleCurveFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
Variant SampleCurveCustom(ReplicaProperty* replicaProperty, TimeMs timestamp)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get sample timestamp in seconds
  float sampleTimestamp = TimeMsToFloatSeconds(timestamp);

  // Empty received value curve?
  // (This can occur if we've never received a change value)
  if(replicaProperty->mBakedCurve[0].Size() == 0)
    return Variant();

  // Create sample result by sampling each primitive member's value curve
  Variant result;
  result.DefaultConstruct<PropertyType>();

  // For each primitive member
  for(size_t i = 0; i < PrimitiveCount; ++i)
  {
    // Get primitive member
    PrimitiveType& resultPrimitiveMember = result.GetPrimitiveMemberOrError<PropertyType>(i);

    // Sample baked out spline curve
    Math::Vec3 point = replicaProperty->mBakedCurve[i].SampleFunction(sampleTimestamp, false);
    resultPrimitiveMember = PrimitiveType(point.y);
  }

  // Success
  return result;
}
// ------------------------------------------------------------------------------------------------------------------------------

Variant ReplicaProperty::SampleCurve(TimeMs timestamp)
{
  // Determine the appropriate SampleCurve function to use
  SampleCurveFn sampleCurveFn = nullptr;

  // Use the custom SampleCurve function for our property type if one exists, else use the standard SampleCurve function
  sampleCurveFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                 SampleCurveCustom, SampleCurveFn, SampleCurveStandard);

  // Sample the property's value curve
  return sampleCurveFn(this, timestamp);
}

Variant ReplicaProperty::GetCurrentSampledValue()
{
  // Sample received value curve
  return SampleCurve(GetCurrentSampleTime());
}

TimeMs ReplicaProperty::GetCurrentSampleTime()
{
  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Get current time
  TimeMs now = replicaPropertyType->GetReplicator()->GetPeer()->GetLocalTime();

  // Determine max sample time
  TimeMs extrapolationLimit = replicaPropertyType->GetExtrapolationLimit();
  TimeMs maxSampleTime = GetLastReceivedChangeTimestamp() + extrapolationLimit;

  // Compute sample time
  TimeMs sampleTime = now + replicaPropertyType->GetSampleTimeOffset();
  sampleTime = Math::Min(sampleTime, maxSampleTime);
  return sampleTime;
}

bool ReplicaProperty::IsResting()
{
  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Get current time
  TimeMs now = replicaPropertyType->GetReplicator()->GetPeer()->GetLocalTime();

  // Determine max sample time
  TimeMs extrapolationLimit = replicaPropertyType->GetExtrapolationLimit();
  TimeMs maxSampleTime = GetLastReceivedChangeTimestamp() + extrapolationLimit;

  // Compare sample time
  TimeMs sampleTime = now + replicaPropertyType->GetSampleTimeOffset();
  return (sampleTime > maxSampleTime);
}

float ReplicaProperty::ComputeRestingInterpolant()
{
  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Get current time
  TimeMs now = replicaPropertyType->GetReplicator()->GetPeer()->GetLocalTime();

  // Determine max sample time
  TimeMs extrapolationLimit = replicaPropertyType->GetExtrapolationLimit();
  TimeMs maxSampleTime = GetLastReceivedChangeTimestamp() + extrapolationLimit;

  // Get sample time
  TimeMs sampleTime = now + replicaPropertyType->GetSampleTimeOffset();
  Assert(sampleTime > maxSampleTime);

  // Compute resting interpolant
  TimeMs restingTimeElapsed = (sampleTime - maxSampleTime);
  TimeMs restingConvergenceDuration = replicaPropertyType->GetRestingConvergenceDuration();
  float t = Math::InverseLerpClamped(TimeMsToFloatSeconds(restingTimeElapsed), float(0), TimeMsToFloatSeconds(restingConvergenceDuration));
  Assert(0 <= t <= 1);
  return t;
}

void ReplicaProperty::SnapNow()
{
  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Get target value
  Variant targetValue;

  // Use interpolation?
  if(replicaPropertyType->GetUseInterpolation())
  {
    // Set target value as the current sampled value from the received change value curve
    targetValue = GetCurrentSampledValue();
  }
  // Don't use interpolation?
  else
  {
    // Set target value as the last received change value
    targetValue = GetLastReceivedChangeValue();
  }

  // No target value?
  // (This can occur if we've never received a change value)
  if(targetValue.IsEmpty())
  {
    // Nothing to snap to
    return;
  }

  // Set current property value to the target value
  SetValue(targetValue);
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic SetValueUsingConvergence Function Type
typedef void (*SetValueUsingConvergenceFn)(ReplicaProperty* replicaProperty, Variant& targetValue, float targetWeight);

/// SetValueUsingConvergenceFn: Standard property type behavior
void SetValueUsingConvergenceStandard(ReplicaProperty* replicaProperty, Variant& targetValue, float targetWeight)
{
  UnusedParameter(targetWeight);
  Warn("Unable to set value using convergence - Unexpected property type");

  // Set current property value directly to the target value instead
  replicaProperty->SetValue(targetValue);
}

/// SetValueUsingConvergenceFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
void SetValueUsingConvergenceCustom(ReplicaProperty* replicaProperty, Variant& targetValue, float targetWeight)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get current and snap threshold values for comparison
  Variant        currentValue  = replicaProperty->GetValue();
  const Variant& snapThreshold = replicaPropertyType->GetSnapThreshold();

  // (Current, target, and snap threshold values should be non-empty)
  Assert(currentValue.IsNotEmpty());
  Assert(targetValue.IsNotEmpty());
  Assert(snapThreshold.IsNotEmpty());

  // For each primitive member
  for(size_t i = 0; i < PrimitiveCount; ++i)
  {
    // Get primitive members
    PrimitiveType&       currentValuePrimitiveMember  = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
    const PrimitiveType& targetValuePrimitiveMember   = targetValue.GetPrimitiveMemberOrError<PropertyType>(i);
    const PrimitiveType& snapThresholdPrimitiveMember = snapThreshold.GetPrimitiveMemberOrError<PropertyType>(i);

    // Compute converged value primitive member
    PrimitiveType convergedValuePrimitiveMember = Converge<PrimitiveType>(currentValuePrimitiveMember, targetValuePrimitiveMember, targetWeight);

    // Should this primitive member be snapped?
    // (Current value and target value primitive members differ by more than the snap threshold value primitive member?)
    bool shouldSnap = (Math::Abs(currentValuePrimitiveMember - targetValuePrimitiveMember) > snapThresholdPrimitiveMember);

    // Should snap primitive member?
    if(shouldSnap)
    {
      // Set current value primitive member to target value primitive member
      currentValuePrimitiveMember = targetValuePrimitiveMember;
    }
    // Should converge primitive member?
    else
    {
      // Set current value primitive member to converged value primitive member
      currentValuePrimitiveMember = convergedValuePrimitiveMember;
    }
  }

  // Set current property value
  replicaProperty->SetValue(currentValue);
}

/// Sets the current property value using convergence toward the target value according to the specified target weight
void SetValueUsingConvergence(ReplicaProperty* replicaProperty, Variant& targetValue, float targetWeight)
{
  // Determine the appropriate SetValueUsingConvergence function to use
  SetValueUsingConvergenceFn setValueUsingConvergenceFn = nullptr;

  // Use the custom SetValueUsingConvergence function for our property type if one exists, else use the standard SetValueUsingConvergence function
  setValueUsingConvergenceFn = BasicNativeTypeDynamicDispatch(replicaProperty->GetNativeTypeId(), NonBooleanArithmetic,
                                                              SetValueUsingConvergenceCustom, SetValueUsingConvergenceFn, SetValueUsingConvergenceStandard);

  // Set property value using convergence
  return setValueUsingConvergenceFn(replicaProperty, targetValue, targetWeight);
}
// ------------------------------------------------------------------------------------------------------------------------------

void ReplicaProperty::ConvergeActiveNow()
{
  Assert(GetConvergenceState() == ConvergenceState::Active);

  // Is property at rest? (Not actively changing?)
  if(IsResting())
  {
    // Update convergence state
    SetConvergenceState(ConvergenceState::Resting);

    // Perform resting convergence instead
    ConvergeRestingNow();
    return;
  }

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Get target value
  Variant targetValue;

  // Use interpolation?
  if(replicaPropertyType->GetUseInterpolation())
  {
    // Set target value as the current sampled value from the received change value curve
    targetValue = GetCurrentSampledValue();
  }
  // Don't use interpolation?
  else
  {
    // Set target value as the last received change value
    targetValue = GetLastReceivedChangeValue();
  }

  // No target value?
  // (This can occur if we've never received a change value)
  if(targetValue.IsEmpty())
  {
    // Nothing to converge to
    return;
  }

  // Set current value using active convergence towards target value
  float activeConvergenceWeight = replicaPropertyType->GetActiveConvergenceWeight();
  SetValueUsingConvergence(this, targetValue, activeConvergenceWeight);
}
void ReplicaProperty::ConvergeRestingNow()
{
  Assert(GetConvergenceState() == ConvergenceState::Resting);

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Set target value as the last received change value
  Variant targetValue = GetLastReceivedChangeValue();

  // No target value?
  // (This can occur if we've never received a change value)
  if(targetValue.IsEmpty())
  {
    // Nothing to converge to
    return;
  }

  // Set current value using resting convergence towards target value
  float restingConvergenceWeight = ComputeRestingInterpolant();
  SetValueUsingConvergence(this, targetValue, restingConvergenceWeight);

  // Has resting convergence duration elapsed?
  if(restingConvergenceWeight >= 1)
  {
    // Update convergence state
    SetConvergenceState(ConvergenceState::None);
  }
}

void ReplicaProperty::ReactToChanges(TimeMs timestamp, ReplicationPhase::Enum replicationPhase, TransmissionDirection::Enum direction, bool generateNotification, bool setLastValue)
{
  // Get replicator
  Replicator* replicator = GetReplicator();

  // Get replica
  Replica* replica = GetReplica();

  // Get replica channel
  ReplicaChannel* replicaChannel = GetReplicaChannel();

  // Get replica channel type
  ReplicaChannelType* replicaChannelType = replicaChannel->GetReplicaChannelType();

  // Get replica property
  ReplicaProperty* replicaProperty = this;

  //    Initialization phase?
  // OR Incoming reaction?
  bool hasChanged = false;
  if(replicationPhase == ReplicationPhase::Initialization
  || direction == TransmissionDirection::Incoming)
  {
    // Detect changes using standard inequality
    hasChanged = replicaProperty->HasChangedAtAll();
  }
  else
  {
    // Detect changes using delta thresholds
    hasChanged = replicaProperty->HasChanged();
  }

  // Property has not changed?
  if(!hasChanged)
    return;

  // Generate notification?
  if(generateNotification)
  {
    // Determine if we should notify the user of the replica channel property change
    bool shouldNotify = false;
    switch(direction)
    {
    default:
    case TransmissionDirection::Unspecified:
      Assert(false);
      break;

    case TransmissionDirection::Incoming:
      shouldNotify = replicaChannelType->GetNotifyOnIncomingPropertyChange();
      break;

    case TransmissionDirection::Outgoing:
      shouldNotify = replicaChannelType->GetNotifyOnOutgoingPropertyChange();
      break;
    }

    // Should notify?
    if(shouldNotify)
    {
      // Notify user of replica channel property change
      replicator->OnReplicaChannelPropertyChange(timestamp, replicationPhase, replica, replicaChannel, replicaProperty, direction);
    }
  }

  // Set last value?
  if(setLastValue)
  {
    // Update last value
    // (For the initialization replication phase we want to forcefully update all primitive-components in our last value to ensure a valid last value state)
    bool forceAll = (replicationPhase == ReplicationPhase::Initialization);
    replicaProperty->UpdateLastValue(forceAll);

    // Set replica, channel, and property last change timestamps
    // (Note: It's possible for this new change timestamp to be chronologically older than the current last change timestamp!
    //        This can occur using immediate transfer modes during replication and having some changes arrive out of order!)
    replicaProperty->SetLastChangeTimestamp(timestamp);
    replicaChannel->SetLastChangeTimestamp(timestamp);
    replica->SetLastChangeTimestamp(timestamp);
  }
}

//
// Internal
//

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic Serialize Function Type
typedef bool (*SerializeFn)(BitStream& bitStream, const ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll);

/// SerializeFn: Standard property type behavior
bool SerializeStandard(BitStream& bitStream, const ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get standard serialization function
  SerializeValueFn serializeValueFn = replicaPropertyType->GetSerializeValueFn();

  // Perform standard serialization
  Variant currentValue = replicaProperty->GetValue();
  return serializeValueFn(SerializeDirection::Write, bitStream, currentValue);
}

/// SerializeFn: Standard quantized property type behavior
bool SerializeQuantizedStandard(BitStream& bitStream, const ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  UnusedParameter(bitStream);
  UnusedParameter(replicaProperty);
  UnusedParameter(timestamp);
  UnusedParameter(forceAll);

  Warn("Unable to serialize quantized replica property - Unexpected property type");
  return false;
}

/// SerializeFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
bool SerializeCustom(BitStream& bitStream, const ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get current and last property values for comparison
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();

  // Get serialization settings
  SerializationMode::Enum serializationMode = replicaPropertyType->GetSerializationMode();
  bool                    useHalfFloats     = replicaPropertyType->GetUseHalfFloats();

  // (Current and last values should be non-empty)
  Assert(currentValue.IsNotEmpty());
  Assert(lastValue.IsNotEmpty());

  //    Serialize all primitive members?
  // OR Force serialization of all primitive members?
  if(serializationMode == SerializationMode::All
  || forceAll)
  {
    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive member
      PrimitiveType& currentValuePrimitiveMember = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);

      // Use half floats?
      if(useHalfFloats)
      {
        // Convert primitive member to float then to half float
        u16 halfFloat = HalfFloatConverter::ToHalfFloat((float)currentValuePrimitiveMember);

        // Write half float
        if(!bitStream.Write(halfFloat)) // Unable?
        {
          Assert(false);
          return false;
        }
      }
      // Do not use half floats?
      else
      {
        // Write primitive member
        if(!bitStream.Write(currentValuePrimitiveMember)) // Unable?
        {
          Assert(false);
          return false;
        }
      }
    }
  }
  // Serialize only the primitive members that have changed?
  else
  {
    Assert(serializationMode == SerializationMode::Changed);

    // Use delta threshold comparison?
    if(replicaPropertyType->GetUseDeltaThreshold())
    {
      // Get delta threshold value for comparison
      const Variant& deltaThreshold = replicaPropertyType->GetDeltaThreshold();

      // (Delta threshold value should be non-empty)
      Assert(deltaThreshold.IsNotEmpty());

      // For each primitive member
      for(size_t i = 0; i < PrimitiveCount; ++i)
      {
        // Get primitive members
        PrimitiveType&       currentValuePrimitiveMember   = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
        const PrimitiveType& lastValuePrimitiveMember      = lastValue.GetPrimitiveMemberOrError<PropertyType>(i);
        const PrimitiveType& deltaThresholdPrimitiveMember = deltaThreshold.GetPrimitiveMemberOrError<PropertyType>(i);

        // Has this primitive member changed?
        // (Current value and last value primitive members differ by more than the delta threshold value primitive member?)
        bool hasChanged = (Math::Abs(currentValuePrimitiveMember - lastValuePrimitiveMember) > deltaThresholdPrimitiveMember);

        // Write 'Has Changed?' Flag
        bitStream.Write(hasChanged);
        if(hasChanged) // Has changed?
        {
          // Use half floats?
          if(useHalfFloats)
          {
            // Convert primitive member to float then to half float
            u16 halfFloat = HalfFloatConverter::ToHalfFloat((float)currentValuePrimitiveMember);

            // Write half float
            if(!bitStream.Write(halfFloat)) // Unable?
            {
              Assert(false);
              return false;
            }
          }
          // Do not use half floats?
          else
          {
            // Write primitive member
            if(!bitStream.Write(currentValuePrimitiveMember)) // Unable?
            {
              Assert(false);
              return false;
            }
          }
        }
      }
    }
    // Do not use delta threshold?
    else
    {
      // For each primitive member
      for(size_t i = 0; i < PrimitiveCount; ++i)
      {
        // Get primitive members
        PrimitiveType&       currentValuePrimitiveMember = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
        const PrimitiveType& lastValuePrimitiveMember    = lastValue.GetPrimitiveMemberOrError<PropertyType>(i);

        // Has this primitive member changed?
        // (Current value and last value primitive members differ?)
        bool hasChanged = (currentValuePrimitiveMember != lastValuePrimitiveMember);

        // Write 'Has Changed?' Flag
        bitStream.Write(hasChanged);
        if(hasChanged) // Has changed?
        {
          // Use half floats?
          if(useHalfFloats)
          {
            // Convert primitive member to float then to half float
            u16 halfFloat = HalfFloatConverter::ToHalfFloat((float)currentValuePrimitiveMember);

            // Write half float
            if(!bitStream.Write(halfFloat)) // Unable?
            {
              Assert(false);
              return false;
            }
          }
          // Do not use half floats?
          else
          {
            // Write primitive member
            if(!bitStream.Write(currentValuePrimitiveMember)) // Unable?
            {
              Assert(false);
              return false;
            }
          }
        }
      }
    }
  }

  // Success
  return true;
}

/// SerializeFn: Custom (Arithmetic) quantized property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
bool SerializeQuantizedCustom(BitStream& bitStream, const ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get current and last property values for comparison
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();

  // Get serialization settings
  SerializationMode::Enum serializationMode = replicaPropertyType->GetSerializationMode();

  // Get quantization settings
  bool           useQuantization      = replicaPropertyType->GetUseQuantization();
  const Variant& quantizationRangeMin = replicaPropertyType->GetQuantizationRangeMin();
  const Variant& quantizationRangeMax = replicaPropertyType->GetQuantizationRangeMax();
  const Variant& quantum              = replicaPropertyType->GetDeltaThreshold();

  // (Current and last values should be non-empty)
  Assert(currentValue.IsNotEmpty());
  Assert(lastValue.IsNotEmpty());

  // (Quantization should be enabled and our quantization parameters should be non-empty)
  Assert(useQuantization
      && quantizationRangeMin.IsNotEmpty()
      && quantizationRangeMax.IsNotEmpty()
      && quantum.IsNotEmpty());

  //    Serialize all primitive members?
  // OR Force serialization of all primitive members?
  if(serializationMode == SerializationMode::All
  || forceAll)
  {
    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive members
      PrimitiveType&       currentValuePrimitiveMember         = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMinPrimitiveMember = quantizationRangeMin.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMaxPrimitiveMember = quantizationRangeMax.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantumPrimitiveMember              = quantum.GetPrimitiveMemberOrError<PropertyType>(i);

      // Write primitive member quantized
      if(!bitStream.WriteQuantized(currentValuePrimitiveMember, quantizationRangeMinPrimitiveMember, quantizationRangeMaxPrimitiveMember, quantumPrimitiveMember)) // Unable?
      {
        Assert(false);
        return false;
      }
    }
  }
  // Serialize only the primitive members that have changed?
  else
  {
    Assert(serializationMode == SerializationMode::Changed);

    // (Quantization requires using a delta threshold since it uses the specified delta threshold as a quantum interval value)
    Assert(replicaPropertyType->GetUseDeltaThreshold());

    // Get delta threshold value for comparison
    const Variant& deltaThreshold = replicaPropertyType->GetDeltaThreshold();

    // (Delta threshold value should be non-empty)
    Assert(deltaThreshold.IsNotEmpty());

    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive members
      PrimitiveType&       currentValuePrimitiveMember         = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& lastValuePrimitiveMember            = lastValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& deltaThresholdPrimitiveMember       = deltaThreshold.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMinPrimitiveMember = quantizationRangeMin.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMaxPrimitiveMember = quantizationRangeMax.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantumPrimitiveMember              = quantum.GetPrimitiveMemberOrError<PropertyType>(i);

      // Has this primitive member changed?
      // (Current value and last value primitive members differ by more than the delta threshold value primitive member?)
      bool hasChanged = (Math::Abs(currentValuePrimitiveMember - lastValuePrimitiveMember) > deltaThresholdPrimitiveMember);

      // Write 'Has Changed?' Flag
      bitStream.Write(hasChanged);
      if(hasChanged) // Has changed?
      {
        // Write primitive member quantized
        if(!bitStream.WriteQuantized(currentValuePrimitiveMember, quantizationRangeMinPrimitiveMember, quantizationRangeMaxPrimitiveMember, quantumPrimitiveMember)) // Unable?
        {
          Assert(false);
          return false;
        }
      }
    }
  }

  // Success
  return true;
}
// ------------------------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic Deserialize Function Type
typedef bool (*DeserializeFn)(Variant& newValue, BitStream& bitStream, ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll);

/// DeserializeFn: Standard property type behavior
bool DeserializeStandard(Variant& newValue, BitStream& bitStream, ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get standard serialization function
  SerializeValueFn serializeValueFn = replicaPropertyType->GetSerializeValueFn();

  // Perform standard serialization
  Variant currentValue = replicaProperty->GetValue();
  if(!serializeValueFn(SerializeDirection::Read, const_cast<BitStream&>(bitStream), currentValue)) // Unable?
  {
    Assert(false);
    return false;
  }

  // Set current value
  replicaProperty->SetValue(currentValue);
  return true;
}

/// DeserializeFn: Standard quantized property type behavior
bool DeserializeQuantizedStandard(Variant& newValue, BitStream& bitStream, ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  UnusedParameter(newValue);
  UnusedParameter(bitStream);
  UnusedParameter(replicaProperty);
  UnusedParameter(timestamp);
  UnusedParameter(forceAll);

  Warn("Unable to deserialize quantized replica property - Unexpected property type");
  return false;
}

/// DeserializeFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
bool DeserializeCustom(Variant& newValue, BitStream& bitStream, ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get current and last property values for comparison
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();

  // Get serialization settings
  SerializationMode::Enum serializationMode = replicaPropertyType->GetSerializationMode();
  bool                    useHalfFloats     = replicaPropertyType->GetUseHalfFloats();

  // (Current value should be non-empty)
  Assert(currentValue.IsNotEmpty());

  //    Serialize all primitive members?
  // OR Force serialization of all primitive members?
  if(serializationMode == SerializationMode::All
  || forceAll)
  {
    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive member
      PrimitiveType& currentValuePrimitiveMember = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);

      // Use half floats?
      if(useHalfFloats)
      {
        // Read half float
        u16 halfFloat;
        if(!bitStream.Read(halfFloat)) // Unable?
        {
          Assert(false);
          return false;
        }

        // Convert half float to float then to primitive member
        currentValuePrimitiveMember = (PrimitiveType)HalfFloatConverter::ToFloat(halfFloat);
      }
      // Do not use half floats?
      else
      {
        // Read primitive member
        if(!bitStream.Read(currentValuePrimitiveMember)) // Unable?
        {
          Assert(false);
          return false;
        }
      }
    }
  }
  // Serialize only the primitive members that have changed?
  else
  {
    Assert(serializationMode == SerializationMode::Changed);

    // Use delta threshold comparison?
    if(replicaPropertyType->GetUseDeltaThreshold())
    {
      // Get delta threshold value for comparison
      const Variant& deltaThreshold = replicaPropertyType->GetDeltaThreshold();

      // (Delta threshold value should be non-empty)
      Assert(deltaThreshold.IsNotEmpty());
    }

    // Set current value to the sampled value from the received value curve
    // (Since we're only given the primitive members that have changed,
    // we want to fill in the other primitive members with the interpolated values
    // we have from that point in time when this received value occurred)
    currentValue = replicaProperty->SampleCurve(timestamp);
    Assert(currentValue.IsNotEmpty());

    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive member
      PrimitiveType& currentValuePrimitiveMember = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);

      // Read 'Has Changed?' Flag
      bool hasChanged;
      bitStream.Read(hasChanged);
      if(hasChanged) // Has changed?
      {
        // Use half floats?
        if(useHalfFloats)
        {
          // Read half float
          u16 halfFloat;
          if(!bitStream.Read(halfFloat)) // Unable?
          {
            Assert(false);
            return false;
          }

          // Convert half float to float then to primitive member
          currentValuePrimitiveMember = (PrimitiveType)HalfFloatConverter::ToFloat(halfFloat);
        }
        // Do not use half floats?
        else
        {
          // Read primitive member
          if(!bitStream.Read(currentValuePrimitiveMember)) // Unable?
          {
            Assert(false);
            return false;
          }
        }
      }
    }
  }

  // Success
  newValue = ZeroMove(currentValue);
  return true;
}

/// DeserializeFn: Custom (Arithmetic) quantized property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
bool DeserializeQuantizedCustom(Variant& newValue, BitStream& bitStream, ReplicaProperty* replicaProperty, TimeMs timestamp, bool forceAll)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = replicaProperty->GetReplicaPropertyType();

  // Get current and last property values for comparison
  Variant        currentValue = replicaProperty->GetValue();
  const Variant& lastValue    = replicaProperty->GetLastValue();

  // Get serialization settings
  SerializationMode::Enum serializationMode = replicaPropertyType->GetSerializationMode();

  // Get quantization settings
  bool           useQuantization      = replicaPropertyType->GetUseQuantization();
  const Variant& quantizationRangeMin = replicaPropertyType->GetQuantizationRangeMin();
  const Variant& quantizationRangeMax = replicaPropertyType->GetQuantizationRangeMax();
  const Variant& quantum              = replicaPropertyType->GetDeltaThreshold();

  // (Current value should be non-empty)
  Assert(currentValue.IsNotEmpty());

  // (Quantization should be enabled and our quantization parameters should be non-empty)
  Assert(useQuantization
      && quantizationRangeMin.IsNotEmpty()
      && quantizationRangeMax.IsNotEmpty()
      && quantum.IsNotEmpty());

  //    Serialize all primitive members?
  // OR Force serialization of all primitive members?
  if(serializationMode == SerializationMode::All
  || forceAll)
  {
    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive members
      PrimitiveType&       currentValuePrimitiveMember         = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMinPrimitiveMember = quantizationRangeMin.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMaxPrimitiveMember = quantizationRangeMax.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantumPrimitiveMember              = quantum.GetPrimitiveMemberOrError<PropertyType>(i);

      // Read primitive member quantized
      if(!bitStream.ReadQuantized(currentValuePrimitiveMember, quantizationRangeMinPrimitiveMember, quantizationRangeMaxPrimitiveMember, quantumPrimitiveMember)) // Unable?
      {
        Assert(false);
        return false;
      }
    }
  }
  // Serialize only the primitive members that have changed?
  else
  {
    Assert(serializationMode == SerializationMode::Changed);

    // (Quantization requires using a delta threshold since it uses the specified delta threshold as a quantum interval value)
    Assert(replicaPropertyType->GetUseDeltaThreshold());

    // Set current value to the sampled value from the received value curve
    // (Since we're only given the primitive members that have changed,
    // we want to fill in the other primitive members with the interpolated values
    // we have from that point in time when this received value occurred)
    currentValue = replicaProperty->SampleCurve(timestamp);
    Assert(currentValue.IsNotEmpty());

    // For each primitive member
    for(size_t i = 0; i < PrimitiveCount; ++i)
    {
      // Get primitive members
      PrimitiveType&       currentValuePrimitiveMember         = currentValue.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMinPrimitiveMember = quantizationRangeMin.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantizationRangeMaxPrimitiveMember = quantizationRangeMax.GetPrimitiveMemberOrError<PropertyType>(i);
      const PrimitiveType& quantumPrimitiveMember              = quantum.GetPrimitiveMemberOrError<PropertyType>(i);

      // Read 'Has Changed?' Flag
      bool hasChanged;
      bitStream.Read(hasChanged);
      if(hasChanged) // Has changed?
      {
        // Read primitive member quantized
        if(!bitStream.ReadQuantized(currentValuePrimitiveMember, quantizationRangeMinPrimitiveMember, quantizationRangeMaxPrimitiveMember, quantumPrimitiveMember)) // Unable?
        {
          Assert(false);
          return false;
        }
      }
    }
  }

  // Success
  newValue = ZeroMove(currentValue);
  return true;
}
// ------------------------------------------------------------------------------------------------------------------------------

bool ReplicaProperty::Serialize(BitStream& bitStream, ReplicationPhase::Enum replicationPhase, TimeMs timestamp) const
{
  // (For the initialization replication phase we want to forcefully serialize all primitive-components to ensure a valid initial value state)
  bool forceAll = (replicationPhase == ReplicationPhase::Initialization);

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Get quantization settings
  bool           useQuantization      = replicaPropertyType->GetUseQuantization();
  const Variant& quantizationRangeMin = replicaPropertyType->GetQuantizationRangeMin();
  const Variant& quantizationRangeMax = replicaPropertyType->GetQuantizationRangeMax();
  const Variant& quantum              = replicaPropertyType->GetDeltaThreshold();

  // Should we quantize?
  // (Quantization is enabled and our quantization parameters are valid?)
  bool shouldQuantize = (useQuantization
                      && quantizationRangeMin.IsNotEmpty()
                      && quantizationRangeMax.IsNotEmpty()
                      && quantum.IsNotEmpty());

  // Determine the appropriate Serialize function to use
  SerializeFn serializeFn = nullptr;

  // Should not quantize?
  if(!shouldQuantize)
  {
    // Use the custom Serialize function for our property type if one exists, else use the standard Serialize function
    serializeFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                 SerializeCustom, SerializeFn, SerializeStandard);
  }
  // Should quantize?
  else
  {
    // Use the custom SerializeQuantized function for our property type if one exists, else use the standard SerializeQuantized function
    serializeFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                 SerializeQuantizedCustom, SerializeFn, SerializeQuantizedStandard);
  }

  // Serialize the property value
  return serializeFn(bitStream, this, timestamp, forceAll);
}
bool ReplicaProperty::Deserialize(const BitStream& bitStream, ReplicationPhase::Enum replicationPhase, TimeMs timestamp)
{
  // (For the initialization replication phase we want to forcefully deserialize all primitive-components to ensure a valid initial value state)
  bool forceAll = (replicationPhase == ReplicationPhase::Initialization);

  // Get replica property type
  ReplicaPropertyType* replicaPropertyType = GetReplicaPropertyType();

  // Get frame ID
  uint64 frameId = replicaPropertyType->GetReplicator()->GetPeer()->GetLocalFrameId();

  // Get quantization settings
  bool           useQuantization      = replicaPropertyType->GetUseQuantization();
  const Variant& quantizationRangeMin = replicaPropertyType->GetQuantizationRangeMin();
  const Variant& quantizationRangeMax = replicaPropertyType->GetQuantizationRangeMax();
  const Variant& quantum              = replicaPropertyType->GetDeltaThreshold();

  // Should we quantize?
  // (Quantization is enabled and our quantization parameters are valid?)
  bool shouldQuantize = (useQuantization
                      && quantizationRangeMin.IsNotEmpty()
                      && quantizationRangeMax.IsNotEmpty()
                      && quantum.IsNotEmpty());

  // Determine the appropriate Deserialize function to use
  DeserializeFn deserializeFn = nullptr;

  // Should not quantize?
  if(!shouldQuantize)
  {
    // Use the custom Deserialize function for our property type if one exists, else use the standard Deserialize function
    deserializeFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                   DeserializeCustom, DeserializeFn, DeserializeStandard);
  }
  // Should quantize?
  else
  {
    // Use the custom DeserializeQuantized function for our property type if one exists, else use the standard DeserializeQuantized function
    deserializeFn = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                   DeserializeQuantizedCustom, DeserializeFn, DeserializeQuantizedStandard);
  }

  // Deserialize the property value
  Variant newValue;
  bool result = deserializeFn(newValue, const_cast<BitStream&>(bitStream), this, timestamp, forceAll);

  // Is a standard deserialize function?
  if(deserializeFn == DeserializeStandard
  || deserializeFn == DeserializeQuantizedStandard)
  {
    // We're done here
    return result;
  }

  // Unable to deserialize?
  if(!result || newValue.IsEmpty())
    return false;

  // (Property type should be arithmetic if we reached this point)
  Assert(replicaPropertyType->GetNativeType()->mIsBasicNativeTypeArithmetic);

  // Use convergence?
  if(replicaPropertyType->GetUseConvergence())
  {
    // Update convergence state (as needed)
    SetConvergenceState(ConvergenceState::Active);
  }

  // Use interpolation?
  if(replicaPropertyType->GetUseInterpolation())
  {
    // Update received change value curve
    UpdateCurve(timestamp, newValue);
  }
  // Don't use interpolation?
  else
  {
    // Set last received change value
    SetLastReceivedChangeValue(newValue);
  }

  // Set last received change timestamp and frame ID
  SetLastReceivedChangeTimestamp(timestamp);
  SetLastReceivedChangeFrameId(frameId);

  // Is initialization phase?
  // (For the initialization replication phase we want to immediately set the exact deserialized value to ensure a valid initial value state)
  if(replicationPhase == ReplicationPhase::Initialization)
  {
    // Set current property value
    SetValue(newValue);
  }
  // Is other phase?
  else
  {
    // Use convergence?
    if(replicaPropertyType->GetUseConvergence())
    {
      // Converge current property value towards the received change value as configured
      ConvergeActiveNow();
    }
    // Don't use convergence?
    else
    {
      // Snap current property value to the received change value as configured
      SnapNow();
    }
  }

  // Success
  return true;
}

//---------------------------------------------------------------------------------//
//                             ReplicaPropertyIndex                                //
//---------------------------------------------------------------------------------//

ReplicaPropertyIndex::ReplicaPropertyIndex()
  : mPropertyLists(),
    mPropertyCount(0)
{
}

ReplicaPropertyIndex::~ReplicaPropertyIndex()
{
  // (Should be empty, else some replica properties weren't removed properly)
  Assert(IsEmpty());
}

bool ReplicaPropertyIndex::IsEmpty() const
{
  return (mPropertyCount == 0);
}

void ReplicaPropertyIndex::CreateLists(uint count)
{
  // (Resizing the populated array can unsafely remove lists containing properties)
  Assert(IsEmpty());

  // Create specified number of lists
  mPropertyLists.Reserve(count);
  for(size_t i = 0; i < count; ++i)
    mPropertyLists.PushBack(ValueType(new PairType(0)));
}

ReplicaPropertyList* ReplicaPropertyIndex::GetList(size_t index)
{
  // Invalid list index?
  if(mPropertyLists.Size() <= index)
    return nullptr;

  return &mPropertyLists[index]->second;
}

size_t ReplicaPropertyIndex::GetListCount() const
{
  return mPropertyLists.Size();
}

void ReplicaPropertyIndex::Insert(ReplicaProperty* property)
{
  // Specified property already has a pointer to a containing list's size?
  if(property->mIndexListSize)
  {
    Assert(false);
    return;
  }

  // Find smallest list
  ValueType* smallestPropertyList     = nullptr;
  size_t     smallestPropertyListSize = std::numeric_limits<size_t>::max();
  forRange(ValueType& propertyList, mPropertyLists.All())
    if(propertyList->first < smallestPropertyListSize)
    {
      smallestPropertyList     = &propertyList;
      smallestPropertyListSize = propertyList->first;
    }

  // Unable to find smallest list?
  if(!smallestPropertyList)
  {
    Assert(false);
    return;
  }

  // Insert property into smallest list
  (*smallestPropertyList)->second.PushBack(property);

  // Store containing list size pointer on property (used later when removing the property)
  property->mIndexListSize = &(*smallestPropertyList)->first;

  // Update list property count
  ++(*property->mIndexListSize);

  // Update total property count
  ++mPropertyCount;
}

void ReplicaPropertyIndex::Remove(ReplicaProperty* property)
{
  // Specified property does not have a pointer to a containing list's size?
  if(!property->mIndexListSize)
  {
    Assert(false);
    return;
  }

  // Update list property count
  --(*property->mIndexListSize);

  // Remove property from containing list
  ReplicaPropertyList::Unlink(property);

  // Clear containing list size pointer on property (it is no longer stored in that list)
  property->mIndexListSize = nullptr;

  // Update total property count
  --mPropertyCount;
}

//---------------------------------------------------------------------------------//
//                             ReplicaPropertyType                                 //
//---------------------------------------------------------------------------------//

ReplicaPropertyType::ReplicaPropertyType(const String& name, NativeType* nativeType, SerializeValueFn serializeValueFn, GetValueFn getValueFn, SetValueFn setValueFn)
  : mName(name),
    mNativeType(nativeType),
    mSerializeValueFn(serializeValueFn),
    mGetValueFn(getValueFn),
    mSetValueFn(setValueFn),
    mReplicator(nullptr)
{
  ResetConfig();
}

ReplicaPropertyType::~ReplicaPropertyType()
{
}

bool ReplicaPropertyType::operator ==(const ReplicaPropertyType& rhs) const
{
  return mName == rhs.mName;
}
bool ReplicaPropertyType::operator !=(const ReplicaPropertyType& rhs) const
{
  return mName != rhs.mName;
}
bool ReplicaPropertyType::operator  <(const ReplicaPropertyType& rhs) const
{
  return mName < rhs.mName;
}
bool ReplicaPropertyType::operator ==(const String& rhs) const
{
  return mName == rhs;
}
bool ReplicaPropertyType::operator !=(const String& rhs) const
{
  return mName != rhs;
}
bool ReplicaPropertyType::operator  <(const String& rhs) const
{
  return mName < rhs;
}

//
// Operations
//

const String& ReplicaPropertyType::GetName() const
{
  return mName;
}

NativeType* ReplicaPropertyType::GetNativeType() const
{
  return mNativeType;
}
NativeTypeId ReplicaPropertyType::GetNativeTypeId() const
{
  return mNativeType->mTypeId;
}

SerializeValueFn ReplicaPropertyType::GetSerializeValueFn() const
{
  return mSerializeValueFn;
}
GetValueFn ReplicaPropertyType::GetGetValueFn() const
{
  return mGetValueFn;
}
SetValueFn ReplicaPropertyType::GetSetValueFn() const
{
  return mSetValueFn;
}

bool ReplicaPropertyType::IsValid() const
{
  return (GetReplicator() != nullptr);
}

void ReplicaPropertyType::MakeValid(Replicator* replicator)
{
  // (Should not already be valid)
  Assert(!IsValid());

  // Create active property index lists to match our convergence interval
  mActivePropertyIndex.CreateLists(GetConvergenceInterval());

  // Create resting property index lists to match our convergence interval
  mRestingPropertyIndex.CreateLists(GetConvergenceInterval());

  // Set operating replicator
  SetReplicator(replicator);

  // (Should now be considered valid)
  Assert(IsValid());
}

void ReplicaPropertyType::SetReplicator(Replicator* replicator)
{
  mReplicator = replicator;
}
Replicator* ReplicaPropertyType::GetReplicator() const
{
  return mReplicator;
}

void ReplicaPropertyType::ConvergeNow()
{
  // Get peer
  Peer* peer = GetReplicator()->GetPeer();

  // Get current time
  TimeMs timestamp = peer->GetLocalTime();

  // Get current frame ID
  uint64 frameId = peer->GetLocalFrameId();

  // Converge active replica properties
  ConvergeNow(true, mActivePropertyIndex, timestamp, frameId);

  // Converge resting replica properties
  ConvergeNow(false, mRestingPropertyIndex, timestamp, frameId);
}
void ReplicaPropertyType::ConvergeNow(bool active, ReplicaPropertyIndex& replicaPropertyIndex, TimeMs timestamp, uint64 frameId)
{
  // (Should be valid)
  Assert(IsValid());

  // Replica properties of this type should not converge?
  if(!GetUseConvergence())
  {
    // Nothing to converge
    return;
  }

  // Get index list count
  uint64 listCount = replicaPropertyIndex.GetListCount();
  if(listCount == 0) // Empty?
    return;

  // Get specified convergence function
  typedef void (ReplicaProperty::*ConvergeNowFn)();
  ConvergeNowFn convergeNowFn = active
                              ? &ReplicaProperty::ConvergeActiveNow
                              : &ReplicaProperty::ConvergeRestingNow;

  // Get scheduled replica property list from index
  ReplicaPropertyList* scheduledList = replicaPropertyIndex.GetList(size_t(frameId % listCount));

  // For all scheduled replica properties in the list
  ReplicaPropertyList::range scheduledProperties = scheduledList->All();
  while(!scheduledProperties.Empty())
  {
    // Get scheduled replica property
    ReplicaProperty& scheduledProperty = scheduledProperties.Front();

    // Advance
    scheduledProperties.PopFront();

    // Last received change value was not received this frame? (We did not already converge this property this frame?)
    // (Note: We add this check to avoid an "extra" convergence jitter when a change happens to be received then converged, and is also scheduled to be converged on the same frame update)
    if(scheduledProperty.GetLastReceivedChangeFrameId() != frameId)
    {
      // Converge the scheduled replica property
      (scheduledProperty.*convergeNowFn)();
    }
  }
}

void ReplicaPropertyType::ScheduleProperty(ReplicaProperty* property)
{
  // (Should be valid)
  Assert(IsValid());

  // Get replica
  Replica* replica = property->GetReplica();

  // Invalid replica?
  if(!replica/* || !replica->IsValid()*/)
    return;

  // Get replicator
  Replicator* replicator = GetReplicator();

  // Replica properties of this type should not converge?
  if(!GetUseConvergence())
  {
    // Don't schedule change convergence for this property
    return;
  }

  //     Replica channel authority matches our role?
  // AND This replica channel type uses a fixed authority mode?
  if(uint(property->GetReplicaChannel()->GetAuthority()) == uint(replicator->GetRole())
  && property->GetReplicaChannel()->GetReplicaChannelType()->GetAuthorityMode() == AuthorityMode::Fixed)
  {
    // Don't schedule change convergence for this property
    return;
  }

  // Already scheduled?
  if(property->IsScheduled())
  {
    // (Can't be scheduled more than once at a time, duplicates are an error)
    Assert(false);
    return;
  }

  // Is active?
  if(property->GetConvergenceState() == ConvergenceState::Active)
  {
    // Add to active property index
    mActivePropertyIndex.Insert(property);
  }
  // Is resting?
  else if(property->GetConvergenceState() == ConvergenceState::Resting)
  {
    // Add to resting property index
    mRestingPropertyIndex.Insert(property);
  }
  // Is inactive?
  else
    Assert(false);
}
void ReplicaPropertyType::UnscheduleProperty(ReplicaProperty* property)
{
  // (Should be valid)
  Assert(IsValid());

  // Already unscheduled?
  if(!property->IsScheduled())
  {
    // (Nothing to do)
    return;
  }

  // Is active?
  if(property->GetConvergenceState() == ConvergenceState::Active)
  {
    // Remove from active property index
    mActivePropertyIndex.Remove(property);
  }
  // Is resting?
  else if(property->GetConvergenceState() == ConvergenceState::Resting)
  {
    // Remove from resting property index
    mRestingPropertyIndex.Remove(property);
  }
  // Is inactive?
  else
    Assert(false);
}

//
// Configuration
//

void ReplicaPropertyType::ResetConfig()
{
  SetUseDeltaThreshold();
  SetDeltaThreshold();
  SetSerializationMode();
  SetUseHalfFloats();
  SetUseQuantization();
  SetQuantizationRangeMin();
  SetQuantizationRangeMax();
  SetUseInterpolation();
  SetInterpolationCurve();
  SetSampleTimeOffset();
  SetExtrapolationLimit();
  SetUseConvergence();
  SetNotifyOnConvergenceStateChange();
  SetActiveConvergenceWeight();
  SetRestingConvergenceDuration();
  SetConvergenceInterval();
  SetSnapThreshold();
}

void ReplicaPropertyType::SetUseDeltaThreshold(bool useDeltaThreshold)
{
  // Attempting to use delta threshold?
  if(useDeltaThreshold)
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Set delta threshold
  mUseDeltaThreshold = useDeltaThreshold;

  // Using delta threshold?
  if(mUseDeltaThreshold)
  {
    // No delta threshold currently set?
    if(GetDeltaThreshold().IsEmpty())
    {
      // Set default delta threshold
      Variant deltaThreshold;
      deltaThreshold.DefaultConstruct(GetNativeType());
      SetDeltaThreshold(deltaThreshold);
    }
  }
}
bool ReplicaPropertyType::GetUseDeltaThreshold() const
{
  return mUseDeltaThreshold;
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic SetDeltaThreshold Function Type
typedef void (*SetDeltaThresholdFn)(ReplicaPropertyType* replicaPropertyType, const Variant& deltaThreshold);

/// SetDeltaThresholdFn: Standard property type behavior
void SetDeltaThresholdStandard(ReplicaPropertyType* replicaPropertyType, const Variant& deltaThreshold)
{
  // Set delta threshold verbatim
  replicaPropertyType->mDeltaThreshold = deltaThreshold;
}

/// SetDeltaThresholdFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
void SetDeltaThresholdCustom(ReplicaPropertyType* replicaPropertyType, const Variant& deltaThreshold)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Correct the given delta threshold (every primitive member must be non-zero and positive)
  Variant correctedDeltaThreshold = deltaThreshold;

  // For each primitive member
  for(size_t i = 0; i < PrimitiveCount; ++i)
  {
    // Get primitive members
    PrimitiveType& correctedDeltaThresholdPrimitiveMember = correctedDeltaThreshold.GetPrimitiveMemberOrError<PropertyType>(i);

    // Correct primitive member (make the value non-zero and positive)
    correctedDeltaThresholdPrimitiveMember = NonZeroAbs(correctedDeltaThresholdPrimitiveMember);
  }

  // Set corrected delta threshold
  replicaPropertyType->mDeltaThreshold = correctedDeltaThreshold;
}
// ------------------------------------------------------------------------------------------------------------------------------

void ReplicaPropertyType::SetDeltaThreshold(const Variant& deltaThreshold)
{
  // Attempting to use delta threshold?
  if(deltaThreshold.IsNotEmpty())
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
    // (Delta threshold type should match our replica property type)
    Assert(deltaThreshold.Is(GetNativeType()));
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Determine the appropriate SetDeltaThreshold function to use
  SetDeltaThresholdFn setDeltaThreshold = nullptr;

  // Empty delta threshold?
  if(deltaThreshold.IsEmpty())
  {
    // Use the standard SetDeltaThreshold function (which just sets the value verbatim)
    setDeltaThreshold = SetDeltaThresholdStandard;
  }
  else
  {
    // Use the custom SetDeltaThreshold function for our property type if one exists, else use the standard SetDeltaThreshold function
    setDeltaThreshold = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                       SetDeltaThresholdCustom, SetDeltaThresholdFn, SetDeltaThresholdStandard);
  }

  // Set the delta threshold
  return setDeltaThreshold(this, deltaThreshold);
}
const Variant& ReplicaPropertyType::GetDeltaThreshold() const
{
  return mDeltaThreshold;
}

void ReplicaPropertyType::SetSerializationMode(SerializationMode::Enum serializationMode)
{
  // Attempting to use serialization mode?
  if(serializationMode == SerializationMode::Changed)
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Property type has only one primitive member?
  if(GetNativeType()->mBasicNativeTypePrimitiveMembersCount <= 1)
  {
    // Force all primitive member serialization
    // (Avoids redundant has-changed checks for the single primitive member,
    // since we have to perform an overall has-changed check to serialize changes in the first place)
    serializationMode = SerializationMode::All;
  }

  // Set serialization mode
  mSerializationMode = serializationMode;
}
SerializationMode::Enum ReplicaPropertyType::GetSerializationMode() const
{
  return mSerializationMode;
}

void ReplicaPropertyType::SetUseHalfFloats(bool useHalfFloats)
{
  // Attempting to use half floats?
  if(useHalfFloats)
  {
    // (Should only be used with floating-point replica property primitive-component types)
    Assert(GetNativeType()->mIsBasicNativeTypeFloatingPoint);
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Set use half floats
  mUseHalfFloats = useHalfFloats;

  // Using half floats?
  if(mUseHalfFloats)
    SetUseQuantization(false); // Disable quantization
}
bool ReplicaPropertyType::GetUseHalfFloats() const
{
  return mUseHalfFloats;
}

void ReplicaPropertyType::SetUseQuantization(bool useQuantization)
{
  // Attempting to use quantization?
  if(useQuantization)
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Set use quantization
  mUseQuantization = useQuantization;

  // Using quantization?
  if(mUseQuantization)
  {
    SetUseDeltaThreshold(true); // Enable delta threshold
    SetUseHalfFloats(false);    // Disable half floats
  }
}
bool ReplicaPropertyType::GetUseQuantization() const
{
  return mUseQuantization;
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic IsPrimitiveMemberLessThan Function Type
typedef bool (*IsPrimitiveMemberLessThanFn)(const Variant& lhs, const Variant& rhs);

/// IsPrimitiveMemberLessThanFn: Standard property type behavior
bool IsPrimitiveMemberLessThanStandard(const Variant& lhs, const Variant& rhs)
{
  UnusedParameter(lhs);
  UnusedParameter(rhs);

  Warn("Unable to perform primitive member less-than comparison - Unexpected property type");
  return false;
}

/// IsPrimitiveMemberLessThanFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
bool IsPrimitiveMemberLessThanCustom(const Variant& lhs, const Variant& rhs)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // For each primitive member
  for(size_t i = 0; i < PrimitiveCount; ++i)
  {
    // Get primitive members
    const PrimitiveType& lhsPrimitiveMember = lhs.GetPrimitiveMemberOrError<PropertyType>(i);
    const PrimitiveType& rhsPrimitiveMember = rhs.GetPrimitiveMemberOrError<PropertyType>(i);

    // Lhs primitive is less than rhs primitive?
    if(lhsPrimitiveMember < rhsPrimitiveMember)
      return true;
  }

  // Every primitive member of lhs is greater-than or equal-to it's corresponding primitive member of rhs
  return false;
}

/// Returns true if any primitive member of lhs is less-than it's corresponding primitive member of rhs, else false
/// (Both lhs and rhs must be non-empty and storing the same type)
bool IsPrimitiveMemberLessThan(const Variant& lhs, const Variant& rhs)
{
  // Different or empty types?
  if(lhs.mNativeType != rhs.mNativeType
  || lhs.mNativeType == nullptr
  || rhs.mNativeType == nullptr)
  {
    // Invalid comparison
    return false;
  }

  // Determine the appropriate IsPrimitiveMemberLessThan function to use
  IsPrimitiveMemberLessThanFn isPrimitiveMemberLessThanFn = nullptr;

  // Use the custom IsPrimitiveMemberLessThan function for our property type if one exists, else use the standard IsPrimitiveMemberLessThan function
  isPrimitiveMemberLessThanFn = BasicNativeTypeDynamicDispatch(lhs.GetNativeTypeId(), Arithmetic,
                                                               IsPrimitiveMemberLessThanCustom, IsPrimitiveMemberLessThanFn, IsPrimitiveMemberLessThanStandard);

  // Perform comparison
  return isPrimitiveMemberLessThanFn(lhs, rhs);
}
// ------------------------------------------------------------------------------------------------------------------------------

void ReplicaPropertyType::SetQuantizationRangeMin(const Variant& quantizationRangeMin)
{
  // Attempting to use quantization?
  if(quantizationRangeMin.IsNotEmpty())
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
    // (Range minimum type should match our replica property type)
    Assert(quantizationRangeMin.Is(GetNativeType()));
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Set quantization range minimum
  mQuantizationRangeMin = quantizationRangeMin;

  // Quantization range min/max is invalid? (Types differ or max is less-than min?)
  if(mQuantizationRangeMax.GetNativeType() != mQuantizationRangeMin.GetNativeType()
  || IsPrimitiveMemberLessThan(mQuantizationRangeMax, mQuantizationRangeMin))
  {
    // Set range max to same as min
    SetQuantizationRangeMax(mQuantizationRangeMin);
  }
}
const Variant& ReplicaPropertyType::GetQuantizationRangeMin() const
{
  return mQuantizationRangeMin;
}

void ReplicaPropertyType::SetQuantizationRangeMax(const Variant& quantizationRangeMax)
{
  // Attempting to use quantization?
  if(quantizationRangeMax.IsNotEmpty())
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
    // (Range maximum type should match our replica property type)
    Assert(quantizationRangeMax.Is(GetNativeType()));
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Set quantization range maximum
  mQuantizationRangeMax = quantizationRangeMax;

  // Quantization range min/max is invalid? (Types differ or max is less-than min?)
  if(mQuantizationRangeMax.GetNativeType() != mQuantizationRangeMin.GetNativeType()
  || IsPrimitiveMemberLessThan(mQuantizationRangeMax, mQuantizationRangeMin))
  {
    // Set range min to same as max
    SetQuantizationRangeMin(mQuantizationRangeMax);
  }
}
const Variant& ReplicaPropertyType::GetQuantizationRangeMax() const
{
  return mQuantizationRangeMax;
}

void ReplicaPropertyType::SetUseInterpolation(bool useInterpolation)
{
  // Attempting to use interpolation?
  if(useInterpolation)
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  mUseInterpolation = useInterpolation;
}
bool ReplicaPropertyType::GetUseInterpolation() const
{
  return mUseInterpolation;
}

void ReplicaPropertyType::SetInterpolationCurve(Math::CurveType::Enum interpolationCurve)
{
  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  mInterpolationCurve = interpolationCurve;
}
Math::CurveType::Enum ReplicaPropertyType::GetInterpolationCurve() const
{
  return mInterpolationCurve;
}

void ReplicaPropertyType::SetSampleTimeOffset(TimeMs sampleTimeOffset)
{
  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // (Clamping within arbitrary, useful values)
  mSampleTimeOffset = Math::Clamp(sampleTimeOffset, -cOneSecondTimeMs, cOneSecondTimeMs);
}
TimeMs ReplicaPropertyType::GetSampleTimeOffset() const
{
  return mSampleTimeOffset;
}

void ReplicaPropertyType::SetExtrapolationLimit(TimeMs extrapolationLimit)
{
  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // (Clamping within arbitrary, useful values)
  mExtrapolationLimit = Math::Clamp(extrapolationLimit, TimeMs(0), cOneSecondTimeMs);
}
TimeMs ReplicaPropertyType::GetExtrapolationLimit() const
{
  return mExtrapolationLimit;
}

void ReplicaPropertyType::SetUseConvergence(bool useConvergence)
{
  // Attempting to use convergence?
  if(useConvergence)
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  mUseConvergence = useConvergence;
}
bool ReplicaPropertyType::GetUseConvergence() const
{
  return mUseConvergence;
}

void ReplicaPropertyType::SetNotifyOnConvergenceStateChange(bool notifyOnConvergenceStateChange)
{
  // Attempting to use convergence?
  if(notifyOnConvergenceStateChange)
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
  }

  mNotifyOnConvergenceStateChange = notifyOnConvergenceStateChange;
}
bool ReplicaPropertyType::GetNotifyOnConvergenceStateChange() const
{
  return mNotifyOnConvergenceStateChange;
}

void ReplicaPropertyType::SetActiveConvergenceWeight(float activeConvergenceWeight)
{
  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // (Clamping within valid values)
  mActiveConvergenceWeight = Math::Clamp(activeConvergenceWeight, float(0), float(1));
}
float ReplicaPropertyType::GetActiveConvergenceWeight() const
{
  return mActiveConvergenceWeight;
}

void ReplicaPropertyType::SetRestingConvergenceDuration(TimeMs restingConvergenceDuration)
{
  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // (Clamping within arbitrary, useful values)
  mRestingConvergenceDuration = Math::Clamp(restingConvergenceDuration, TimeMs(0), cOneSecondTimeMs);
}
TimeMs ReplicaPropertyType::GetRestingConvergenceDuration() const
{
  return mRestingConvergenceDuration;
}

void ReplicaPropertyType::SetConvergenceInterval(uint convergenceInterval)
{
  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // (Clamping within arbitrary, useful values)
  mConvergenceInterval = Math::Clamp(convergenceInterval, uint(1), uint(100));
}
uint ReplicaPropertyType::GetConvergenceInterval() const
{
  return mConvergenceInterval;
}

// ------------------------------------------------------------------------------------------------------------------------------
/// Generic SetSnapThreshold Function Type
typedef void (*SetSnapThresholdFn)(ReplicaPropertyType* replicaPropertyType, const Variant& snapThreshold);

/// SetSnapThresholdFn: Standard property type behavior
void SetSnapThresholdStandard(ReplicaPropertyType* replicaPropertyType, const Variant& snapThreshold)
{
  // Set snap threshold verbatim
  replicaPropertyType->mSnapThreshold = snapThreshold;
}

/// SetSnapThresholdFn: Custom (Arithmetic) property type behavior
template <typename PropertyType, TF_ENABLE_IF(IsBasicNativeTypeArithmetic<PropertyType>::Value)>
void SetSnapThresholdCustom(ReplicaPropertyType* replicaPropertyType, const Variant& snapThreshold)
{
  // Primitive member info
  typedef typename BasicNativeTypePrimitiveMembers<PropertyType>::Type PrimitiveType;
  constexpr size_t PrimitiveCount = BasicNativeTypePrimitiveMembers<PropertyType>::Count;

  // Correct the given snap threshold (every primitive member must be non-zero and positive)
  Variant correctedSnapThreshold = snapThreshold;

  // For each primitive member
  for(size_t i = 0; i < PrimitiveCount; ++i)
  {
    // Get primitive members
    PrimitiveType& correctedSnapThresholdPrimitiveMember = correctedSnapThreshold.GetPrimitiveMemberOrError<PropertyType>(i);

    // Correct primitive member (make the value non-zero and positive)
    correctedSnapThresholdPrimitiveMember = NonZeroAbs(correctedSnapThresholdPrimitiveMember);
  }

  // Set corrected snap threshold
  replicaPropertyType->mSnapThreshold = correctedSnapThreshold;
}
// ------------------------------------------------------------------------------------------------------------------------------

void ReplicaPropertyType::SetSnapThreshold(const Variant& snapThreshold)
{
  // Attempting to use snap threshold?
  if(snapThreshold.IsNotEmpty())
  {
    // (Should only be used with arithmetic replica property types)
    Assert(GetNativeType()->mIsBasicNativeTypeArithmetic);
    // (Snap threshold type should match our replica property type)
    Assert(snapThreshold.Is(GetNativeType()));
  }

  // Already valid?
  if(IsValid())
  {
    // Unable to modify configuration
    Error("ReplicaPropertyType is already valid, unable to modify configuration");
    return;
  }

  // Determine the appropriate SetSnapThreshold function to use
  SetSnapThresholdFn setSnapThreshold = nullptr;

  // Empty snap threshold?
  if(snapThreshold.IsEmpty())
  {
    // Use the standard SetSnapThreshold function (which just sets the value verbatim)
    setSnapThreshold = SetSnapThresholdStandard;
  }
  else
  {
    // Use the custom SetSnapThreshold function for our property type if one exists, else use the standard SetSnapThreshold function
    setSnapThreshold = BasicNativeTypeDynamicDispatch(GetNativeTypeId(), NonBooleanArithmetic,
                                                      SetSnapThresholdCustom, SetSnapThresholdFn, SetSnapThresholdStandard);
  }

  // Set the snap threshold
  return setSnapThreshold(this, snapThreshold);
}
const Variant& ReplicaPropertyType::GetSnapThreshold() const
{
  return mSnapThreshold;
}

} // namespace Zero
