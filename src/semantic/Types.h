
#ifndef TYPES_H
#define TYPES_H


#include <string>


// Internal representation of Bhasha's value types.
// Note: "BOOL" has no declaration keyword in our grammar (only literals
// সত্যি/মিথ্যা and comparison/logical results produce it), but we still
// need it internally to type-check conditions and printed values.
enum class ValueType
{
   INT,
   DECIMAL,
   TEXT,
   BOOL,
   UNKNOWN // used when a type error already occurred, to avoid cascading errors
};


inline std::string valueTypeName(ValueType t)
{
   switch (t)
   {
   case ValueType::INT:
       return "পূর্ণসংখ্যা";
   case ValueType::DECIMAL:
       return "দশমিকসংখ্যা";
   case ValueType::TEXT:
       return "লেখা";
   case ValueType::BOOL:
       return "বুলিয়ান";
   default:
       return "অজানা (unknown)";
   }
}


// Maps the declaration keyword lexeme to its ValueType
inline ValueType typeFromKeyword(const std::string &keyword)
{
   if (keyword == "পূর্ণসংখ্যা")
       return ValueType::INT;
   if (keyword == "দশমিকসংখ্যা")
       return ValueType::DECIMAL;
   if (keyword == "লেখা")
       return ValueType::TEXT;
   if (keyword == "বুলিয়ান")
       return ValueType::BOOL;
   return ValueType::UNKNOWN;
}


#endif

