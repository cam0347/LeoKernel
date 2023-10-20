#pragma once
#include <include/types.h>
#include <acpi/my_parser/include/vm.h>
#define AML_DUALNAME_PREFIX 0x2E
#define AML_MULTINAME_PREFIX 0x2F
#define AML_BYTE_PREFIX 0x0A
#define AML_WORD_PREFIX 0x0B
#define AML_DWORD_PREFIX 0x0C
#define AML_QWORD_PREFIX 0x0E
#define AML_STRING_PREFIX 0x0D
#define IS_LEAD_NAME_CHAR(c) (c >= 'A' && c <= 'Z' || c >= 'a' && c <= 'z' || c == '_')
#define IS_NAME_CHAR(c) (IS_LEAD_NAME_CHAR(c) || c >= '0' && c <= '9')
#define IS_ASCII_CHAR(c) (c >= 0x01 && c <= 0x7F)

//object names
uint32_t acpi_parse_namestring(char *str, char **out);
uint32_t acpi_parse_namepath(char *str, char **);
uint32_t acpi_parse_nameseg(char *str, char **);
uint32_t acpi_parse_dualnamepath(char *str, char **);
uint32_t acpi_parse_multinamepath(char *str, char **);

//number constants
uint32_t acpi_parse_byteconst(uint8_t *aml, uint8_t **byte);
uint32_t acpi_parse_wordconst(uint8_t *aml, uint16_t **word);
uint32_t acpi_parse_dwordconst(uint8_t *aml, uint32_t **dword);
uint32_t acpi_parse_qwordconst(uint8_t *aml, uint64_t **qword);
uint32_t acpi_parse_constobject(uint8_t *aml, uint8_t **constobj);

//other
uint32_t acpi_parse_asciicharlist(char *str, char **out);
uint32_t acpi_parse_computationaldata(uint8_t *aml, void **out);
uint32_t acpi_parse_pkglength(uint8_t *aml, void *out);