// Adapted from https://stackoverflow.com/a/202511/1519878
// Author: StackOverflow user Suma (https://stackoverflow.com/users/16673)
// Licence: Creative Commons Attribution-ShareAlike (CC-BY-SA)

// expansion macro for enum value definition
#define ENUM_VALUE(name,assign) name assign,

// expansion macro for enum to string conversion
#define ENUM_CASE(name,assign) case name: return #name;

// expansion macro for string to enum conversion
#define ENUM_STRCMP(name,assign) if (!strcmp(str,#name)) return name;

/// declare the access function and define enum values
#define DECLARE_ENUM(EnumType,ENUM_DEF) \
  typedef enum EnumType { \
    ENUM_DEF(ENUM_VALUE) \
  } EnumType; \
  const char *EnumType##_string(EnumType value); \
  EnumType EnumType##_value(const char *string); \

/// define the access function names
#define DEFINE_ENUM(EnumType,ENUM_DEF) \
  const char *EnumType##_string(EnumType value) \
  { \
    switch(value) \
    { \
      ENUM_DEF(ENUM_CASE) \
      default: return ""; /* handle input error */ \
    } \
  } \
  EnumType EnumType##_value(const char *str) \
  { \
    ENUM_DEF(ENUM_STRCMP) \
    return (EnumType)0; /* handle input error */ \
  } \

