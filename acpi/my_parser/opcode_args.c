/*
This file defines the function to interpret the opcode arguments.
Each function takes 2 parameters: a pointer to the aml code to interpret and a pointer to return the value to.
Each function returns the length of the data in bytes so that the interpreter can increment the program counter correctly.
*/

#include <include/types.h>
#include <acpi/my_parser/include/opcode_args.h>
#include <acpi/my_parser/include/vm.h>
#include <acpi/my_parser/include/exec.h>

uint32_t acpi_parse_namestring(char *str, char **out) {
    uint32_t prefix_length = 0;

    while(*(str + prefix_length) == '\\' || *(str + prefix_length) == '^') {
        prefix_length++;
    }

    char *namePath;
    uint32_t namepath_length = acpi_parse_namepath(str + prefix_length, &namePath);

    if (namepath_length > 0) {
        *out = namePath;
        return prefix_length + namepath_length;
    } else {
        *out = null;
        return 0;
    }
}

/*
NamePath: NameSeg | DualNamePath | MultiNamePath | NullName
*/
uint32_t acpi_parse_namepath(char *str, char **out) {
    uint32_t ret = 0;

    if (*str == AML_DUALNAME_PREFIX) { //is a DualNamePath
        printf("dualnamepath\n");
        ret = acpi_parse_dualnamepath(str, out);
    } else if (*str == AML_MULTINAME_PREFIX) { //is a MultiNamePath
        printf("multinamepath\n");
        ret = acpi_parse_multinamepath(str, out);
    } else if (*str == 0x00) { //is a NullName
        printf("nullname");
        *out = str;
        ret = 1;
    } else { //is a NameSeg
        printf("nameseg\n");
        ret = acpi_parse_nameseg(str, out);
    }

    if (ret > 0) {
        return ret;
    } else {
        *out = null;
        return 0;
    }
}

/*
NameSeg: <LeadNameChar NameChar NameChar NameChar>
*/
uint32_t acpi_parse_nameseg(char *str, char **out) {
    if (IS_LEAD_NAME_CHAR(*str) && IS_NAME_CHAR(*(str + 1)) && IS_NAME_CHAR(*(str + 2)) && IS_NAME_CHAR(*(str + 3))) {
        *out = str;
        return 4;
    } else {
        *out = null;
        return 0;
    }
}

/*
DualNamePath: DualNamePrefix NameSeg NameSeg
*/
uint32_t acpi_parse_dualnamepath(char *str, char **out) {
    *out = null;
    if (*str != AML_DUALNAME_PREFIX) return 0;
    char *tmp;
    uint32_t nameSeg1_length = acpi_parse_nameseg(str + 1, &tmp);
    uint32_t nameSeg2_length = acpi_parse_nameseg(str + 1 + nameSeg1_length, &tmp);
    
    if (nameSeg1_length == 0 || nameSeg2_length == 0) {
        return 0;
    }

    *out = str;
    return nameSeg1_length + nameSeg1_length + 1;
}

/*
MultiNamePath: MultiNamePrefix SegCount NameSeg(SegCount)
*/
uint32_t acpi_parse_multinamepath(char *str, char **out) {
    *out = null;
    if (*str != AML_MULTINAME_PREFIX) return 0;
    uint8_t segcount = *(uint8_t *)(str + 1);
    if (segcount == 0) return 0; //segcount must be between 1 and 255
    uint32_t ret = 0;
    
    for (uint8_t i = 0; i < segcount; i++) {
        char *tmp;
        uint32_t l = acpi_parse_nameseg(str + 1 + ret, &tmp);
        
        if (l == 0) {
            return 0;
        }

        ret += l;
    }

    *out = str;
    return ++ret;
}

/*
String: StringPrefix AsciiCharList NullChar
*/
uint32_t acpi_parse_string(char *str, char **out) {
    *out = null;
    if (*str != AML_STRING_PREFIX) return 0;
    char *tmp;
    uint32_t charlist_length = acpi_parse_asciicharlist(str + 1, &tmp);
    if (*(str + 1 + charlist_length) != 0x00) return 0; //the string must be null terminated (not included in the charlist)
    *out = str;
    return charlist_length + 2;
}

/*
AsciiCharList: nothing | <AsciiChar AsciiCharList>
*/
uint32_t acpi_parse_asciicharlist(char *str, char **out) {
    uint32_t ret = 0;
    while(IS_ASCII_CHAR(*(str + ret))) {
        ret++;
    }

    *out = str;
    return ret;
}

/*
ByteConst: BytePrefix ByteData
*/
uint32_t acpi_parse_byteconst(uint8_t *aml, uint8_t **byte) {
    *byte = null;
    if (*aml != AML_BYTE_PREFIX) return 0;
    *byte = aml + 1;
    return 1;
}

/*
WordConst: WordPrefix WordData
*/
uint32_t acpi_parse_wordconst(uint8_t *aml, uint16_t **word) {
    *word = null;
    if (*aml != AML_WORD_PREFIX) return 0;
    *word = (uint16_t *)(aml + 1);
    return 2;
}

/*
DWordConst: DWordPrefix DWordData
*/
uint32_t acpi_parse_dwordconst(uint8_t *aml, uint32_t **dword) {
    *dword = null;
    if (*aml != AML_DWORD_PREFIX) return 0;
    *dword = (uint32_t *)(aml + 1);
    return 4;
}

/*
QWordConst: QWordPrefix QWordData
*/
uint32_t acpi_parse_qwordconst(uint8_t *aml, uint64_t **qword) {
    *qword = null;
    if (*aml != AML_QWORD_PREFIX) return 0;
    *qword = (uint64_t *)(aml + 1);
    return 8;
}

/*
ConstObject: ZeroOp | OneOp | OnesOp
*/
uint32_t acpi_parse_constobject(uint8_t *aml, uint8_t **constobj) {
    *constobj = 0x00;

    if (*aml == 0x00 || *aml == 0x01 || *aml == 0xFF) {
        *constobj = aml;
        return 1;
    }

    return 0;
}

/*
ComputationalData: ByteConst | WordConst | DWordConst | QWordConst | String | ConstObj | RevisionOp | DefBuffer
*/
uint32_t acpi_parse_computationaldata(uint8_t *aml, void **out) {
    *out = null;
    uint32_t ret = 0;

    //switch prefix
    switch(*aml) {
        case AML_BYTE_PREFIX:
            uint8_t *byte;
            ret = acpi_parse_byteconst(aml, &byte);
            *(uint8_t **) out = byte;
            break;

        case AML_WORD_PREFIX:
            uint16_t *word;
            ret = acpi_parse_wordconst(aml, &word);
            *(uint16_t **) out = word;
            break;

        case AML_DWORD_PREFIX:
            uint32_t *dword;
            ret = acpi_parse_dwordconst(aml, &dword);
            *(uint32_t **) out = dword;
            break;

        case AML_QWORD_PREFIX:
            uint64_t *qword;
            ret = acpi_parse_qwordconst(aml, &qword);
            *(uint64_t **) out = qword;
            break;

        case AML_STRING_PREFIX:
            char *str;
            ret = acpi_parse_string((char *) aml, &str);
            *(char **) out = str;
            break;

        case 0x00:
        case 0x01:
        case 0xFF:
            uint8_t *obj;
            ret = acpi_parse_constobject(aml, &obj);
            *(uint8_t **) out = obj;
            break;

        case AML_EXTOP_PREFIX:
            if (*(aml + 1) == 0x30) {
                ret = 2;
                *out = aml;
            } else {
                ret = 0;
            }
            break;

        case 0x11:
            uint32_t tmp;
            if ((ret = acpi_parse_pkglength(aml, &tmp)) > 0) {
                *out = aml;
            } else {
                ret = 0;
            }

        default:
            ret = 0;
            break;
    }

    return ret;
}

/*
PkgLength: PkgLeadByte | <PkgLeadByte ByteData> | <PkgLeadByte ByteData ByteData> | <PkgLeadByte ByteData ByteData ByteData>
This function differs, it returns the package size through out.
*/
uint32_t acpi_parse_pkglength(uint8_t *aml, void *out) {
    *(uint8_t *) out = 0;
    uint8_t lead_byte = *aml;
    uint8_t following_bytes = lead_byte >> 6 & 0xFF;
    uint32_t pkgLength = 0;

    if (following_bytes == 0) {
        *(uint8_t *) out = lead_byte & 0x3F;  
        return 1;      
    }
    
    uint8_t lsb = lead_byte & 0x0F;
    uint32_t total_length = lsb;
    uint8_t n_bytes = 1;

    if (following_bytes > 0) {
        total_length |= *(aml + 1) << 4;
        n_bytes++;
    }

    if (following_bytes > 1) {
        total_length |= *(aml + 2) << 12;
        n_bytes++;
    }

    if (following_bytes > 2) {
        total_length |= *(aml + 3) << 20;
        n_bytes++;
    }

    *(uint32_t *) out = total_length;
    return n_bytes;
}