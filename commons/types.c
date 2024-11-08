#include <stdio.h>
#include <string.h>
#include "types.h"

const char* varTypeToString(VarType type) {
    switch (type)
    {
        case (VarType_Void):
            return "VarType_Void"; //Assigned only for functions with no return
            break;
        case (VarType_Int):
            return "VarType_Int";
            break;
        case (VarType_Float):
            return "VarType_Float";
            break;
        case (VarType_Char):
            return "VarType_Char";
            break;
        case (VarType_Error):
            return "VarType_Error"; //Assigned when program detects invalid var type
            break;
        default:
            return "UNHANDLED_VARTYPE_ERROR";   //Should only occur when type is UNASSIGNED
            break;                              //(i.e. garbage data from unassigned type, different in nature than "VarType_Error")
    }
}

VarType stringToVarType(const char* typeString) {
    if (strcmp(typeString, "void") == 0) {
        return VarType_Void;
    } else if (strcmp(typeString, "int") == 0) {
        return VarType_Int;
    } else if (strcmp(typeString, "float") == 0) {
        return VarType_Float;
    } else if (strcmp(typeString, "char") == 0) {
        return VarType_Char;
    } else {
        fprintf(stderr, "COMMONS/TYPES.C: Invalid type assigned to variable");
        return VarType_Error;
    }
}