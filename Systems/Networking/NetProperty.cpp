///////////////////////////////////////////////////////////////////////////////
///
/// Authors: Andrew Colean.
/// Copyright 2015, DigiPen Institute of Technology.
///
///////////////////////////////////////////////////////////////////////////////
#include "Precompiled.hpp"

namespace Zero
{

/// Converts each primitive member of the arithmetic source variant into the primitive member type of the arithmetic destination variant,
/// and then overwrites the corresponding destination member with the source member. Remaining destination members, if any, are left untouched.
/// Returns true if successful, else false
bool ConvertArithmeticVariant(const Variant& source, Variant& destination)
{
  // (Must be different objects)
  ReturnIf(&source == &destination, false, "Unable to translate variant - Source and destination variants are the same object");

  // Get source and destination native types
  NativeType* sourceNativeType = source.GetNativeType();
  NativeType* destinationNativeType = destination.GetNativeType();

  // Same type?
  if(sourceNativeType == destinationNativeType)
  {
    // Just assign source to destination (no careful type conversion necessary)
    destination = source;
    return true;
  }

  // (Both variants must be arithmetic types)
  ReturnIf(sourceNativeType == nullptr, false, "Unable to translate variant - Source variant is empty (there's no value to be translated)");
  ReturnIf(destinationNativeType == nullptr, false, "Unable to translate variant - Destination variant is empty (unknown type to be translated to)");
  ReturnIf(!sourceNativeType->mIsBasicNativeTypeArithmetic, false, "Unable to translate variant - Source variant is not an arithmetic type");
  ReturnIf(!destinationNativeType->mIsBasicNativeTypeArithmetic, false, "Unable to translate variant - Destination variant is not an arithmetic type");

  // Get the conversion function from the source type to the destination type
  ConvertArithmeticObjectFn convertArithmeticObjectFn = BasicNativeTypeDynamicDoubleDispatch(sourceNativeType->mTypeId, destinationNativeType->mTypeId,
                                                                                             Arithmetic, ConvertArithmeticObject, ConvertArithmeticObjectFn, nullptr);
  ReturnIf(convertArithmeticObjectFn == nullptr, false, "Unable to translate variant - Could not get conversion function");

  // Get source and destination data (the conversion function type is opaque so we have to pass our data this way)
  void* sourceData = source.GetData();
  void* destinationData = destination.GetData();

  // Convert and assign source primitive members to destination
  convertArithmeticObjectFn(sourceData, destinationData);
  return true;
}

//---------------------------------------------------------------------------------//
//                                 NetProperty                                     //
//---------------------------------------------------------------------------------//

ZilchDefineType(NetProperty, builder, type)
{
  // Bind tags
  ZeroBindTag(Tags::Networking);

  // Bind documentation
  ZeroBindDocumented();

  // Bind operations
  ZilchBindGetterProperty(Name);
  ZilchBindGetterProperty(NetPropertyType);
  ZilchBindGetterProperty(NetChannel);
  ZilchBindGetterProperty(LastChangeTimestamp);
  ZilchBindGetterProperty(LastChangeTimePassed);
}

NetProperty::NetProperty(const String& name, NetPropertyType* netPropertyType, const Variant& propertyData)
  : ReplicaProperty(name, netPropertyType, propertyData)
{
}

NetProperty::~NetProperty()
{
}

//
// Operations
//

const String& NetProperty::GetName() const
{
  return ReplicaProperty::GetName();
}

NetPropertyType* NetProperty::GetNetPropertyType() const
{
  return static_cast<NetPropertyType*>(ReplicaProperty::GetReplicaPropertyType());
}

NetChannel* NetProperty::GetNetChannel() const
{
  return static_cast<NetChannel*>(ReplicaProperty::GetReplicaChannel());
}

float NetProperty::GetLastChangeTimestamp() const
{
  // Get last change timestamp
  TimeMs timestamp = ReplicaProperty::GetLastChangeTimestamp();
  if(timestamp == cInvalidMessageTimestamp) // Invalid?
    return 0;

  return TimeMsToFloatSeconds(timestamp);
}

float NetProperty::GetLastChangeTimePassed() const
{
  // Get replicator
  Replicator* replicator = ReplicaProperty::GetReplicator();
  if(!replicator) // Unable?
    return 0;

  // Get current time
  TimeMs now = replicator->GetPeer()->GetLocalTime();

  // Get last change timestamp
  TimeMs timestamp = ReplicaProperty::GetLastChangeTimestamp();
  if(timestamp == cInvalidMessageTimestamp) // Invalid?
    return 0;

  // Compute time passed since last change (duration between now and last change timestamp)
  TimeMs timePassed = (now - timestamp);
  return TimeMsToFloatSeconds(timePassed);
}

//---------------------------------------------------------------------------------//
//                               NetPropertyType                                   //
//---------------------------------------------------------------------------------//

ZilchDefineType(NetPropertyType, builder, type)
{
  // Bind tags
  ZeroBindTag(Tags::Networking);

  // Bind documentation
  ZeroBindDocumented();

  // Bind operations
  ZilchBindGetterProperty(Name);

  // Bind configuration
  ZilchBindMethod(ResetConfig);
  ZilchBindMethod(SetConfig);
}

NetPropertyType::NetPropertyType(const String& name, NativeType* nativeType, SerializeValueFn serializeValueFn, GetValueFn getValueFn, SetValueFn setValueFn)
  : ReplicaPropertyType(name, nativeType, serializeValueFn, getValueFn, setValueFn)
{
  ResetConfig();
}

NetPropertyType::~NetPropertyType()
{
}

//
// Operations
//

const String& NetPropertyType::GetName() const
{
  return ReplicaPropertyType::GetName();
}

BasicNetType::Enum NetPropertyType::GetBasicNetType() const
{
  return BasicNativeTypeToBasicNetTypeEnum((BasicNativeType::Enum)ReplicaPropertyType::GetNativeTypeId());
}

//
// Configuration
//

void NetPropertyType::ResetConfig()
{
  // Not valid yet?
  if(!IsValid())
  {
    // Set non-runtime config options
    SetDeltaThreshold();
    SetUseDeltaThreshold();
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
    SetActiveConvergenceWeight();
    SetRestingConvergenceDuration();
    SetConvergenceInterval();
    SetSnapThreshold();
  }

  // Set runtime config options
  SetNotifyOnConvergenceStateChange();
}

void NetPropertyType::SetConfig(NetPropertyConfig* netPropertyConfig)
{
  // Get config's target network property type
  BasicNetType::Enum configBasicNetType = netPropertyConfig->GetBasicNetType();

  // Non-arithmetic type?
  if(configBasicNetType == BasicNetType::Other
  || configBasicNetType == BasicNetType::Boolean
  || configBasicNetType == BasicNetType::String)
    return; // Unable to translate

  // Get our network property type
  BasicNetType::Enum ourBasicNetType = GetBasicNetType();

  // Non-arithmetic type?
  if(ourBasicNetType == BasicNetType::Other
  || ourBasicNetType == BasicNetType::Boolean
  || ourBasicNetType == BasicNetType::String)
    return; // Unable to translate

  // Not valid yet?
  if(!IsValid())
  {
    // Get config variant property types
    Variant configDeltaThreshold       = netPropertyConfig->mDeltaThreshold;
    Variant configQuantizationRangeMin = netPropertyConfig->mQuantizationRangeMin;
    Variant configQuantizationRangeMax = netPropertyConfig->mQuantizationRangeMax;
    Variant configSnapThreshold        = netPropertyConfig->mSnapThreshold;

    // Need to translate config variant property types?
    if(ourBasicNetType != configBasicNetType)
    {
      // Translate each config variant property
      Variant convertedConfigDeltaThreshold = NetPropertyConfig::GetDefaultDeltaThreshold(ourBasicNetType);
      ConvertArithmeticVariant(configDeltaThreshold, convertedConfigDeltaThreshold);
      configDeltaThreshold = convertedConfigDeltaThreshold;

      Variant convertedConfigQuantizationRangeMin = NetPropertyConfig::GetDefaultQuantizationRangeMin(ourBasicNetType);
      ConvertArithmeticVariant(configQuantizationRangeMin, convertedConfigQuantizationRangeMin);
      configQuantizationRangeMin = convertedConfigQuantizationRangeMin;

      Variant convertedConfigQuantizationRangeMax = NetPropertyConfig::GetDefaultQuantizationRangeMax(ourBasicNetType);
      ConvertArithmeticVariant(configQuantizationRangeMax, convertedConfigQuantizationRangeMax);
      configQuantizationRangeMax = convertedConfigQuantizationRangeMax;

      Variant convertedConfigSnapThreshold = NetPropertyConfig::GetDefaultSnapThreshold(ourBasicNetType);
      ConvertArithmeticVariant(configSnapThreshold, convertedConfigSnapThreshold);
      configSnapThreshold = convertedConfigSnapThreshold;
    }

    // Set non-runtime config options
    SetDeltaThreshold(configDeltaThreshold);
    SetUseDeltaThreshold(netPropertyConfig->mUseDeltaThreshold);
    SetSerializationMode(netPropertyConfig->mSerializationMode);
    SetUseHalfFloats(netPropertyConfig->mUseHalfFloats);
    SetUseQuantization(netPropertyConfig->mUseQuantization);
    SetQuantizationRangeMin(configQuantizationRangeMin);
    SetQuantizationRangeMax(configQuantizationRangeMax);
    SetUseInterpolation(netPropertyConfig->mUseInterpolation);
    SetInterpolationCurve(netPropertyConfig->mInterpolationCurve);
    SetSampleTimeOffset(FloatSecondsToTimeMs(netPropertyConfig->mSampleTimeOffset));
    SetExtrapolationLimit(FloatSecondsToTimeMs(netPropertyConfig->mExtrapolationLimit));
    SetUseConvergence(netPropertyConfig->mUseConvergence);
    SetActiveConvergenceWeight(netPropertyConfig->mActiveConvergenceWeight);
    SetRestingConvergenceDuration(FloatSecondsToTimeMs(netPropertyConfig->mRestingConvergenceDuration));
    SetConvergenceInterval(netPropertyConfig->mConvergenceInterval);
    SetSnapThreshold(configSnapThreshold);
  }

  // Set runtime config options
  SetNotifyOnConvergenceStateChange(netPropertyConfig->mEventOnConvergenceStateChange);
}

//---------------------------------------------------------------------------------//
//                              NetPropertyConfig                                  //
//---------------------------------------------------------------------------------//

#define DefaultValueCaseForArithmeticTypes(property) \
DefaultValueCaseForType(property, Integer);          \
DefaultValueCaseForType(property, DoubleInteger);    \
DefaultValueCaseForType(property, Integer2);         \
DefaultValueCaseForType(property, Integer3);         \
DefaultValueCaseForType(property, Integer4);         \
DefaultValueCaseForType(property, Real);             \
DefaultValueCaseForType(property, DoubleReal);       \
DefaultValueCaseForType(property, Real2);            \
DefaultValueCaseForType(property, Real3);            \
DefaultValueCaseForType(property, Real4);            \
DefaultValueCaseForType(property, Quaternion)

#define DefaultValueCaseForType(property, type) \
case BasicNetType::##type:                      \
  return Variant(sDefault##property##type)

// Variant Configuration Helper Macros
#define DefineVariantGetSetForArithmeticTypes(property, defaultInt, defaultReal)                                            \
DefineVariantGetSetForType(property, Integer,       Integer(defaultInt));                                                   \
DefineVariantGetSetForType(property, DoubleInteger, DoubleInteger(defaultInt));                                             \
DefineVariantGetSetForType(property, Integer2,      Integer2(defaultInt, defaultInt));                                      \
DefineVariantGetSetForType(property, Integer3,      Integer3(defaultInt, defaultInt, defaultInt));                          \
DefineVariantGetSetForType(property, Integer4,      Integer4(defaultInt, defaultInt, defaultInt, defaultInt));              \
DefineVariantGetSetForType(property, Real,          Real(defaultReal));                                                     \
DefineVariantGetSetForType(property, DoubleReal,    DoubleReal(defaultReal));                                               \
DefineVariantGetSetForType(property, Real2,         Real2(defaultReal, defaultReal));                                       \
DefineVariantGetSetForType(property, Real3,         Real3(defaultReal, defaultReal, defaultReal));                          \
DefineVariantGetSetForType(property, Real4,         Real4(defaultReal, defaultReal, defaultReal, defaultReal));             \
DefineVariantGetSetForType(property, Quaternion,    Quaternion(defaultReal, defaultReal, defaultReal, defaultReal));        \
Variant NetPropertyConfig::GetDefault##property(BasicNetType::Enum basicNetType)                                            \
{                                                                                                                           \
  switch(basicNetType)                                                                                                      \
  {                                                                                                                         \
  /* Non-arithmetic type? Return a default constructed value. */                                                            \
  default:                                                                                                                  \
    {                                                                                                                       \
      Variant result;                                                                                                       \
                                                                                                                            \
      if(NativeType* nativeType = GetNativeTypeByConstantId((NativeTypeId)BasicNetTypeToBasicNativeTypeEnum(basicNetType))) \
        result.DefaultConstruct(nativeType);                                                                                \
                                                                                                                            \
      Assert(result.IsNotEmpty());                                                                                          \
      return result;                                                                                                        \
    }                                                                                                                       \
                                                                                                                            \
  /* Arithmetic type? Return the explicitly declared default value for this property. */                                    \
  DefaultValueCaseForArithmeticTypes(property);                                                                             \
  }                                                                                                                         \
}

#define DefineVariantGetSetForType(property, type, defaultValue)       \
const type NetPropertyConfig::sDefault##property##type = defaultValue; \
void NetPropertyConfig::Set##property##type(type value)                \
{                                                                      \
  m##property = value;                                                 \
}                                                                      \
type NetPropertyConfig::Get##property##type() const                    \
{                                                                      \
  return m##property.GetOrDefault<type>(sDefault##property##type);     \
}

#define BindVariantGetSetForArithmeticTypes(property) \
BindVariantGetSetForType(property, Integer);          \
BindVariantGetSetForType(property, DoubleInteger);    \
BindVariantGetSetForType(property, Integer2);         \
BindVariantGetSetForType(property, Integer3);         \
BindVariantGetSetForType(property, Integer4);         \
BindVariantGetSetForType(property, Real);             \
BindVariantGetSetForType(property, DoubleReal);       \
BindVariantGetSetForType(property, Real2);            \
BindVariantGetSetForType(property, Real3);            \
BindVariantGetSetForType(property, Real4);            \
BindVariantGetSetForType(property, Quaternion)

#define BindVariantGetSetForType(property, type)                             \
ZilchBindGetterSetterProperty(property##type)->Add(new PropertyFilter##type)

#define DefinePropertyFilterForType(type)                                             \
ZilchDefineType(PropertyFilter##type, builder, type)                                  \
{                                                                                     \
}                                                                                     \
bool PropertyFilter##type::Filter(Member* prop, HandleParam instance)                 \
{                                                                                     \
  return (instance.Get<NetPropertyConfig*>()->mBasicNetType == BasicNetType::##type); \
}

// Variant Configuration Property Filters
DefinePropertyFilterForType(Other);
DefinePropertyFilterForType(Boolean);
DefinePropertyFilterForType(Integer);
DefinePropertyFilterForType(DoubleInteger);
DefinePropertyFilterForType(Integer2);
DefinePropertyFilterForType(Integer3);
DefinePropertyFilterForType(Integer4);
DefinePropertyFilterForType(Real);
DefinePropertyFilterForType(DoubleReal);
DefinePropertyFilterForType(Real2);
DefinePropertyFilterForType(Real3);
DefinePropertyFilterForType(Real4);
DefinePropertyFilterForType(Quaternion);
DefinePropertyFilterForType(String);

ZilchDefineType(PropertyFilterMultiPrimitiveTypes, builder, type)
{
}

bool PropertyFilterMultiPrimitiveTypes::Filter(Member* prop, HandleParam instance)
{
  switch(instance.Get<NetPropertyConfig*>()->mBasicNetType)
  {
  default:
  case BasicNetType::Other:
  case BasicNetType::Boolean:
  case BasicNetType::Integer:
  case BasicNetType::DoubleInteger:
  case BasicNetType::Real:
  case BasicNetType::DoubleReal:
  case BasicNetType::String:
    return false;

  case BasicNetType::Integer2:
  case BasicNetType::Integer3:
  case BasicNetType::Integer4:
  case BasicNetType::Real2:
  case BasicNetType::Real3:
  case BasicNetType::Real4:
  case BasicNetType::Quaternion:
    return true;
  }
}

ZilchDefineType(PropertyFilterFloatingPointTypes, builder, type)
{
}

bool PropertyFilterFloatingPointTypes::Filter(Member* prop, HandleParam instance)
{
  switch(instance.Get<NetPropertyConfig*>()->mBasicNetType)
  {
  default:
  case BasicNetType::Other:
  case BasicNetType::Boolean:
  case BasicNetType::Integer:
  case BasicNetType::DoubleInteger:
  case BasicNetType::Integer2:
  case BasicNetType::Integer3:
  case BasicNetType::Integer4:
  case BasicNetType::String:
    return false;

  case BasicNetType::Real:
  case BasicNetType::DoubleReal:
  case BasicNetType::Real2:
  case BasicNetType::Real3:
  case BasicNetType::Real4:
  case BasicNetType::Quaternion:
    return true;
  }
}

ZilchDefineType(PropertyFilterArithmeticTypes, builder, type)
{
}

bool PropertyFilterArithmeticTypes::Filter(Member* prop, HandleParam instance)
{
  switch(instance.Get<NetPropertyConfig*>()->mBasicNetType)
  {
  default:
  case BasicNetType::Other:
  case BasicNetType::Boolean:
  case BasicNetType::String:
    return false;

  case BasicNetType::Integer:
  case BasicNetType::DoubleInteger:
  case BasicNetType::Integer2:
  case BasicNetType::Integer3:
  case BasicNetType::Integer4:
  case BasicNetType::Real:
  case BasicNetType::DoubleReal:
  case BasicNetType::Real2:
  case BasicNetType::Real3:
  case BasicNetType::Real4:
  case BasicNetType::Quaternion:
    return true;
  }
}

ZilchDefineType(NetPropertyConfig, builder, type)
{
  // Bind tags
  ZeroBindTag(Tags::Networking);

  // Bind documentation
  ZeroBindDocumented();

  // Bind setup (can be added in the editor)
  ZeroBindSetup(SetupMode::DefaultSerialization);

  // Bind data members
  ZilchBindGetterSetterProperty(BasicNetType)->AddAttribute(PropertyAttributes::cInvalidatesObject);
  ZilchBindGetterSetterProperty(UseDeltaThreshold)->Add(new PropertyFilterArithmeticTypes);
  BindVariantGetSetForArithmeticTypes(DeltaThreshold);
  ZilchBindGetterSetterProperty(SerializationMode)->Add(new PropertyFilterMultiPrimitiveTypes);
  ZilchBindGetterSetterProperty(UseHalfFloats)->AddAttributeChainable(PropertyAttributes::cInvalidatesObject)->Add(new PropertyFilterFloatingPointTypes);
  ZilchBindGetterSetterProperty(UseQuantization)->AddAttributeChainable(PropertyAttributes::cInvalidatesObject)->Add(new PropertyFilterArithmeticTypes);
  BindVariantGetSetForArithmeticTypes(QuantizationRangeMin);
  BindVariantGetSetForArithmeticTypes(QuantizationRangeMax);
  ZilchBindGetterSetterProperty(UseInterpolation)->Add(new PropertyFilterArithmeticTypes);
  //ZilchBindGetterSetterProperty(InterpolationCurve)->Add(new PropertyFilterArithmeticTypes);
  ZilchBindGetterSetterProperty(SampleTimeOffset)->Add(new PropertyFilterArithmeticTypes);
  ZilchBindGetterSetterProperty(ExtrapolationLimit)->Add(new PropertyFilterArithmeticTypes);
  ZilchBindGetterSetterProperty(UseConvergence)->Add(new PropertyFilterArithmeticTypes);
  ZilchBindGetterSetterProperty(EventOnConvergenceStateChange)->Add(new PropertyFilterArithmeticTypes);
  ZilchBindGetterSetterProperty(ActiveConvergenceWeight)->Add(new PropertyFilterArithmeticTypes);
  ZilchBindGetterSetterProperty(RestingConvergenceDuration)->Add(new PropertyFilterArithmeticTypes);
  ZilchBindGetterSetterProperty(ConvergenceInterval)->Add(new PropertyFilterArithmeticTypes);
  BindVariantGetSetForArithmeticTypes(SnapThreshold);
}

NetPropertyConfig::NetPropertyConfig()
  : mBasicNetType(BasicNetType::Other),
    mUseDeltaThreshold(false),
    mDeltaThreshold(),
    mSerializationMode(SerializationMode::All),
    mUseHalfFloats(false),
    mUseQuantization(false),
    mQuantizationRangeMin(),
    mQuantizationRangeMax(),
    mUseInterpolation(false),
    mInterpolationCurve(Math::CurveType::Linear),
    mSampleTimeOffset(0),
    mExtrapolationLimit(0),
    mUseConvergence(false),
    mEventOnConvergenceStateChange(false),
    mActiveConvergenceWeight(0),
    mRestingConvergenceDuration(0),
    mConvergenceInterval(0),
    mSnapThreshold()
{
}

//
// Data Resource Interface
//

void NetPropertyConfig::Serialize(Serializer& stream)
{
  // Serialize data members (default variant properties to assume "Real" type)
  SerializeEnumNameDefault(BasicNetType, mBasicNetType, BasicNetType::Real);
  SerializeNameDefault(mUseDeltaThreshold, false);
  SerializeNameDefault(mDeltaThreshold, Variant(sDefaultDeltaThresholdReal));
  SerializeEnumNameDefault(SerializationMode, mSerializationMode, SerializationMode::All);
  SerializeNameDefault(mUseHalfFloats, false);
  SerializeNameDefault(mUseQuantization, false);
  SerializeNameDefault(mQuantizationRangeMin, Variant(sDefaultQuantizationRangeMinReal));
  SerializeNameDefault(mQuantizationRangeMax, Variant(sDefaultQuantizationRangeMaxReal));
  SerializeNameDefault(mUseInterpolation, false);
  //SerializeEnumNameDefault(Math::CurveType, mInterpolationCurve, Math::CurveType::CatmullRom);
  SerializeNameDefault(mSampleTimeOffset, float(0.1));
  SerializeNameDefault(mExtrapolationLimit, float(1));
  SerializeNameDefault(mUseConvergence, false);
  SerializeNameDefault(mEventOnConvergenceStateChange, false);
  SerializeNameDefault(mActiveConvergenceWeight, float(0.1));
  SerializeNameDefault(mRestingConvergenceDuration, float(0.05));
  SerializeNameDefault(mConvergenceInterval, uint(1));
  SerializeNameDefault(mSnapThreshold, Variant(sDefaultSnapThresholdReal));

  // TODO: Fix non-linear interpolation curve functionality. For now we'll just force linear interpolation.
  mInterpolationCurve = Math::CurveType::Linear;

  // Loading?
  if(stream.GetMode() == SerializerMode::Loading)
  {
    // Translate read in variant properties, just in case their types don't match our target type
    TranslateVariantProperties();
  }
}

//
// Operations
//

const String& NetPropertyConfig::GetName() const
{
  return Name;
}

void NetPropertyConfig::TranslateVariantProperties()
{
  // Non-arithmetic type?
  if(mBasicNetType == BasicNetType::Other
  || mBasicNetType == BasicNetType::Boolean
  || mBasicNetType == BasicNetType::String)
    return; // Unable to translate

  // Convert each variant property to our current target type
  Variant convertedDeltaThreshold = NetPropertyConfig::GetDefaultDeltaThreshold(mBasicNetType);
  ConvertArithmeticVariant(mDeltaThreshold, convertedDeltaThreshold);
  mDeltaThreshold = convertedDeltaThreshold;

  Variant convertedQuantizationRangeMin = NetPropertyConfig::GetDefaultQuantizationRangeMin(mBasicNetType);
  ConvertArithmeticVariant(mQuantizationRangeMin, convertedQuantizationRangeMin);
  mQuantizationRangeMin = convertedQuantizationRangeMin;

  Variant convertedQuantizationRangeMax = NetPropertyConfig::GetDefaultQuantizationRangeMax(mBasicNetType);
  ConvertArithmeticVariant(mQuantizationRangeMax, convertedQuantizationRangeMax);
  mQuantizationRangeMax = convertedQuantizationRangeMax;

  Variant convertedSnapThreshold = NetPropertyConfig::GetDefaultSnapThreshold(mBasicNetType);
  ConvertArithmeticVariant(mSnapThreshold, convertedSnapThreshold);
  mSnapThreshold = convertedSnapThreshold;
}
void NetPropertyConfig::DefaultVariantProperties()
{
  // Default each variant property to our current target type's default value for the property
  mDeltaThreshold       = NetPropertyConfig::GetDefaultDeltaThreshold(mBasicNetType);
  mQuantizationRangeMin = NetPropertyConfig::GetDefaultQuantizationRangeMin(mBasicNetType);
  mQuantizationRangeMax = NetPropertyConfig::GetDefaultQuantizationRangeMax(mBasicNetType);
  mSnapThreshold        = NetPropertyConfig::GetDefaultSnapThreshold(mBasicNetType);
}

//
// Configuration
//

void NetPropertyConfig::SetBasicNetType(BasicNetType::Enum basicNetType)
{
  mBasicNetType = basicNetType;

  // TODO: Display a destructive action dialog to confirm the user wants to proceed.
  DefaultVariantProperties();
}
BasicNetType::Enum NetPropertyConfig::GetBasicNetType() const
{
  return mBasicNetType;
}

void NetPropertyConfig::SetUseDeltaThreshold(bool useDeltaThreshold)
{
  mUseDeltaThreshold = useDeltaThreshold;

  // Not using delta threshold?
  if(!mUseDeltaThreshold)
    SetUseQuantization(false); // Disable quantization
}
bool NetPropertyConfig::GetUseDeltaThreshold() const
{
  return mUseDeltaThreshold;
}

DefineVariantGetSetForArithmeticTypes(DeltaThreshold, int(1), float(1));

void NetPropertyConfig::SetSerializationMode(SerializationMode::Enum serializationMode)
{
  mSerializationMode = serializationMode;
}
SerializationMode::Enum NetPropertyConfig::GetSerializationMode() const
{
  return mSerializationMode;
}

void NetPropertyConfig::SetUseHalfFloats(bool useHalfFloats)
{
  mUseHalfFloats = useHalfFloats;

  // Using half floats?
  if(mUseHalfFloats)
    SetUseQuantization(false); // Disable quantization
}
bool NetPropertyConfig::GetUseHalfFloats() const
{
  return mUseHalfFloats;
}

void NetPropertyConfig::SetUseQuantization(bool useQuantization)
{
  mUseQuantization = useQuantization;

  // Using quantization?
  if(mUseQuantization)
  {
    SetUseDeltaThreshold(true); // Enable delta threshold
    SetUseHalfFloats(false);    // Disable half floats
  }
}
bool NetPropertyConfig::GetUseQuantization() const
{
  return mUseQuantization;
}

DefineVariantGetSetForArithmeticTypes(QuantizationRangeMin, int(-1), float(-1));

DefineVariantGetSetForArithmeticTypes(QuantizationRangeMax, int(+1), float(+1));

void NetPropertyConfig::SetUseInterpolation(bool useInterpolation)
{
  mUseInterpolation = useInterpolation;
}
bool NetPropertyConfig::GetUseInterpolation() const
{
  return mUseInterpolation;
}

void NetPropertyConfig::SetInterpolationCurve(Math::CurveType::Enum interpolationCurve)
{
  mInterpolationCurve = interpolationCurve;
}
Math::CurveType::Enum NetPropertyConfig::GetInterpolationCurve() const
{
  return mInterpolationCurve;
}

void NetPropertyConfig::SetSampleTimeOffset(float sampleTimeOffset)
{
  mSampleTimeOffset = sampleTimeOffset;
}
float NetPropertyConfig::GetSampleTimeOffset() const
{
  return mSampleTimeOffset;
}

void NetPropertyConfig::SetExtrapolationLimit(float extrapolationLimit)
{
  mExtrapolationLimit = extrapolationLimit;
}
float NetPropertyConfig::GetExtrapolationLimit() const
{
  return mExtrapolationLimit;
}

void NetPropertyConfig::SetUseConvergence(bool useConvergence)
{
  mUseConvergence = useConvergence;
}
bool NetPropertyConfig::GetUseConvergence() const
{
  return mUseConvergence;
}

void NetPropertyConfig::SetEventOnConvergenceStateChange(bool eventOnConvergenceStateChange)
{
  mEventOnConvergenceStateChange = eventOnConvergenceStateChange;
}
bool NetPropertyConfig::GetEventOnConvergenceStateChange() const
{
  return mEventOnConvergenceStateChange;
}

void NetPropertyConfig::SetActiveConvergenceWeight(float activeConvergenceWeight)
{
  mActiveConvergenceWeight = activeConvergenceWeight;
}
float NetPropertyConfig::GetActiveConvergenceWeight() const
{
  return mActiveConvergenceWeight;
}

void NetPropertyConfig::SetRestingConvergenceDuration(float restingConvergenceDuration)
{
  mRestingConvergenceDuration = restingConvergenceDuration;
}
float NetPropertyConfig::GetRestingConvergenceDuration() const
{
  return mRestingConvergenceDuration;
}

void NetPropertyConfig::SetConvergenceInterval(uint convergenceInterval)
{
  mConvergenceInterval = convergenceInterval;
}
uint NetPropertyConfig::GetConvergenceInterval() const
{
  return mConvergenceInterval;
}

DefineVariantGetSetForArithmeticTypes(SnapThreshold, int(10), float(10));

// Variant Configuration Helper Macros
#undef DefineVariantGetSetForArithmeticTypes
#undef DefineVariantGetSetForType
#undef BindVariantGetSetForArithmeticTypes
#undef BindVariantGetSetForType
#undef DefinePropertyFilterForType

//---------------------------------------------------------------------------------//
//                           NetPropertyConfigManager                              //
//---------------------------------------------------------------------------------//

ImplementResourceManager(NetPropertyConfigManager, NetPropertyConfig);
NetPropertyConfigManager::NetPropertyConfigManager(BoundType* resourceType)
  : ResourceManager(resourceType)
{
  AddLoader("NetPropertyConfig", new TextDataFileLoader<NetPropertyConfigManager>());
  mCategory = "Networking";
  mCanAddFile = true;
  mOpenFileFilters.PushBack(FileDialogFilter("*.NetPropertyConfig.data"));
  DefaultResourceName = "Default";
  mCanCreateNew = true;
  mCanDuplicate = true;
  mExtension = DataResourceExtension;
}

//---------------------------------------------------------------------------------//
//                               NetPropertyInfo                                   //
//---------------------------------------------------------------------------------//

ZilchDefineType(NetPropertyInfo, builder, type)
{
  // Bind documentation
  ZeroBindDocumented();

  // // Bind default constructor
  // ZilchBindDefaultConstructor();

  // Bind property interface
  ZilchBindCustomGetterPropertyAs(GetComponentName, "Component");
  ZilchBindCustomGetterPropertyAs(GetPropertyName,  "Property");
  ZilchBindGetterSetterProperty(NetChannelConfig);
  ZilchBindGetterSetterProperty(NetPropertyConfig);
}

NetPropertyInfo::NetPropertyInfo()
  : mComponentType(nullptr),
    mPropertyName(),
    mNetChannelConfig(),
    mNetPropertyConfig()
{
}
NetPropertyInfo::NetPropertyInfo(BoundType* componentType, StringParam propertyName)
  : mComponentType(componentType),
    mPropertyName(propertyName),
    mNetChannelConfig(NetChannelConfigManager::GetInstance()->FindOrNull("Default")),
    mNetPropertyConfig(NetPropertyConfigManager::GetInstance()->FindOrNull("Default"))
{
}

bool NetPropertyInfo::operator ==(const NetPropertyInfo& rhs) const
{
  return mComponentType == rhs.mComponentType
      && mPropertyName  == rhs.mPropertyName;
}
bool NetPropertyInfo::operator !=(const NetPropertyInfo& rhs) const
{
  return !(*this == rhs);
}
bool NetPropertyInfo::operator ==(const Pair<BoundType*, String>& rhs) const
{
  return mComponentType == rhs.first
      && mPropertyName  == rhs.second;
}
bool NetPropertyInfo::operator !=(const Pair<BoundType*, String>& rhs) const
{
  return !(*this == rhs);
}

//
// Serialization Interface
//

void NetPropertyInfo::Serialize(Serializer& stream)
{
  // Serialize target component name
  String componentName = GetComponentName();
  stream.SerializeFieldDefault("ComponentName", componentName, String());
  if(stream.GetMode() == SerializerMode::Loading) // Is loading?
    SetComponentName(componentName); // Set component type from the name we loaded

  // Serialize target property name
  SerializeNameDefault(mPropertyName, String());

  // Serialize net channel and property config resources
  SerializeResourceNameDefault(mNetChannelConfig, NetChannelConfigManager, "Default");
  SerializeResourceNameDefault(mNetPropertyConfig, NetPropertyConfigManager, "Default");
}

void NetPropertyInfo::SetDefaults()
{
}

//
// Property Interface
//

void NetPropertyInfo::SetComponentName(StringParam componentName)
{
  mComponentType = MetaDatabase::GetInstance()->FindType(componentName);
}
String NetPropertyInfo::GetComponentName()
{
  return mComponentType ? mComponentType->Name : String();
}

String NetPropertyInfo::GetPropertyName()
{
  return mPropertyName;
}

void NetPropertyInfo::SetNetChannelConfig(NetChannelConfig* netChannelConfig)
{
  mNetChannelConfig = netChannelConfig;
}
NetChannelConfig* NetPropertyInfo::GetNetChannelConfig()
{
  return mNetChannelConfig;
}

void NetPropertyInfo::SetNetPropertyConfig(NetPropertyConfig* netPropertyConfig)
{
  mNetPropertyConfig = netPropertyConfig;
}
NetPropertyConfig* NetPropertyInfo::GetNetPropertyConfig()
{
  return mNetPropertyConfig;
}

//---------------------------------------------------------------------------------//
//                          NetPropertyChangedEventIds                             //
//---------------------------------------------------------------------------------//

void NetPropertyChangedEventIds::CreateEventIds(StringParam channelName, StringParam componentName, StringParam propertyName)
{
  // (Channel, component, and property names should never be empty)
  Assert(!channelName.Empty());
  Assert(!componentName.Empty());
  Assert(!propertyName.Empty());

  // Build the event IDs with channel prepended
  {
    StringBuilder builder;
    builder.Append(channelName);
    builder.Append('_');
    builder.Append(cNetPropertyChanged);

    // Ex. "InputChannel_NetPropertyChanged"
    mChannelEventId = builder.ToString();

    builder.Append('_');
    builder.Append(componentName);

    // Ex. "InputChannel_NetPropertyChanged_Player"
    mChannelComponentEventId = builder.ToString();

    builder.Append('_');
    builder.Append(propertyName);

    // Ex. "InputChannel_NetPropertyChanged_Player_InputJump"
    mChannelComponentPropertyEventId = builder.ToString();
  }

  // Build the event IDs without channel prepended
  {
    StringBuilder builder;
    builder.Append(cNetPropertyChanged);
    builder.Append('_');
    builder.Append(componentName);

    // Ex. "NetPropertyChanged_Player"
    mComponentEventId = builder.ToString();

    builder.Append('_');
    builder.Append(propertyName);

    // Ex. "NetPropertyChanged_Player_InputJump"
    mComponentPropertyEventId = builder.ToString();
  }
}

} // namespace Zero
