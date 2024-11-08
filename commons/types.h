#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

typedef enum VarType {
    VarType_Void,
    VarType_Int,
    VarType_Float,
    VarType_Char,
    VarType_Error
}VarType;

const char* varTypeToString(VarType type);
VarType stringToVarType(const char* typeString);

#endif