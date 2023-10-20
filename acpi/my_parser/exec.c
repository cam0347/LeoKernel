#include <include/types.h>
#include <acpi/my_parser/include/ns.h>
#include <tty/include/tty.h>
#include <acpi/include/acpi.h>
#include <include/mem.h>
#include <acpi/my_parser/include/vm.h>
#include <acpi/my_parser/include/exec.h>
#include <include/string.h>
#include <acpi/my_parser/include/opcode_args.h>

bool parse_table(void *table) {
    acpi_table_header *header = (acpi_table_header *) table;
    acpi_vm_state_t *state = acpi_vm_state();
    state->size = header->length - sizeof(acpi_table_header);
    state->aml = header + sizeof(acpi_table_header);
    state->pc = state->aml;
    state->scope = acpi_ns_root();

    char *sign = header->signature;
    if (strncmp(sign, "DSDT", 4) && strncmp(sign, "SSDT", 4)) {
        printf("Not an AML encoded table... %s\n", sign);
        return false;
    }

    printf("Parsing %d bytes of code\n", state->size);
    printf("pc: 0x%x, limit: 0x%x\n", state->pc, state->aml + state->size);

    while(state->pc < state->aml + state->size) {
        uint16_t opcode;
        void *args; //pointer to the arguments of the opcode
        uint32_t args_size; //this will be set once the operands are parsed
        acpi_vm_fetch(state, &opcode, &args); //get the next opcode and pointer to it's arguments
        acpi_vm_exec(opcode, args, &args_size); //execute the opcode and save the size of the instruction to update the vm state
        state->pc += args_size;
    }
}

void acpi_vm_fetch(acpi_vm_state_t *state, uint16_t *opcode, void **args) {
    *opcode = *(uint8_t *) state->pc & 0xFF;
    uint8_t opcode_cont = *(uint8_t *)(state->pc + 1);

    if (*opcode == AML_EXTOP_PREFIX || *opcode == 0x92 && opcode_cont >= 93 && opcode_cont <= 95) {
        *opcode <<= 8;
        *opcode |= opcode_cont;
        *(uint64_t *) state->pc++;
    }

    *(uint64_t *) state->pc++;
    *args = state->pc; //the updated pc points now to the operands
}

void acpi_vm_exec(uint16_t opcode, void *args, uint32_t *args_size) {
    *args_size = 0;

    switch(opcode) {
        case 0x0000:
            //printf("ZeroOp\n");
            break;

        case 0x0001:
            //printf("OneOp\n");
            break;

        case 0x0006:
            tty_clear();
            printf("AliasOp\n");

            char *name1, *name2;
            *args_size = acpi_parse_namestring((char *) args, &name1);
            if (*args_size == 0) {acpi_vm_panic_wrapper(__FILE__, __LINE__, "unexpected size of parameters\n");} /*error*/;
            args += *args_size;
            *args_size = acpi_parse_namestring((char *) args + *args_size, &name2);
            if (*args_size == 0) {acpi_vm_panic_wrapper(__FILE__, __LINE__, "unexpected size of parameters\n");} /*error*/;
            printf("String 1: %s\nString 2: %s\n", name1, name2);
            break;

        case 0x0008:
            printf("NameOp\n");
            char *name;
            *args_size = acpi_parse_namestring((char *) args, &name);
            break;

        case 0x0010:
            printf("ScopeOp\n");
            break;

        case 0x0011:
            printf("BufferOp\n");
            break;

        case 0x0012:
            //printf("PackageOp\n");
            break;

        case 0x0013:
            //printf("VarPackageOp\n");
            break;

        case 0x0014:
            //printf("MethodOp\n");
            break;
            
        case 0x0015:
            //printf("ExternalOp\n");
            break;

        case 0x5B01:
            //printf("MutexOp\n");
            break;

        case 0x5B02:
            //printf("EventOp\n");
            break;

        case 0x5B12:
            //printf("CondRefOfOp\n");
            break;

        case 0x5B13:
            //printf("CreateFieldOp\n");
            break;

        case 0x5B1F:
            //printf("LoadTableOp\n");
            break;

        case 0x5B20:
            //printf("LoadOp\n");
            break;

        case 0x5B21:
            //printf("StallOp\n");
            break;

        case 0x5B22:
            //printf("SleepOp\n");
            break;

        case 0x5B23:
            //printf("AcquireOp\n");
            break;

        case 0x5B24:
            //printf("SignalOp\n");
            break;

        case 0x5B25:
            //printf("WaitOp\n");
            break;

        case 0x5B26:
            //printf("ResetOp\n");
            break;

        case 0x5B27:
            //printf("ReleaseOp\n");
            break;

        case 0x5B28:
            //printf("FromBCDOp\n");
            break;

        case 0x5B29:
            //printf("ToBCD\n");
            break;

        case 0x5B30:
            //printf("RevisionOp\n");
            break;

        case 0x5B31:
            //printf("DebugOp\n");
            break;

        case 0x5B32:
            //printf("FatalOp\n");
            break;

        case 0x5B33:
            //printf("TimerOp\n");
            break;

        case 0x5B80:
            //printf("OpRegionOp\n");
            break;

        case 0x5B81:
            //printf("FieldOp\n");
            break;

        case 0x5B82:
            //printf("DeviceOp\n");
            break;

        case 0x5B83:
            //printf("ProcessorOp\n");
            break;

        case 0x5B84:
            //printf("PowerResOp\n");
            break;

        case 0x5B85:
            //printf("ThermalZoneOp\n");
            break;

        case 0x5B86:
            //printf("IndexFieldOp\n");
            break;

        case 0x5B87:
            //printf("BankFieldOp\n");
            break;

        case 0x5B88:
            //printf("DataRegionOp\n");
            break;

        case 0x0060:
            //printf("Local0Op\n");
            break;

        case 0x0061:
            //printf("Local1Op\n");
            break;

        case 0x0062:
            //printf("Local2Op\n");
            break;

        case 0x0063:
            //printf("Local3Op\n");
            break;

        case 0x0064:
            //printf("Local4Op\n");
            break;

        case 0x0065:
            //printf("Local5Op\n");
            break;

        case 0x0066:
            //printf("Local6Op\n");
            break;

        case 0x0067:
            //printf("Local7Op\n");
            break;

        case 0x0068:
            //printf("Arg0Op\n");
            break;

        case 0x0069:
            //printf("Arg1Op\n");
            break;

        case 0x006A:
            //printf("Arg2Op\n");
            break;

        case 0x006B:
            //printf("Arg3Op\n");
            break;

        case 0x006C:
            //printf("Arg4Op\n");
            break;

        case 0x006D:
            //printf("Arg5Op\n");
            break;

        case 0x006E:
            //printf("Arg6Op\n");
            break;

        case 0x0070:
            //printf("StoreOp\n");
            break;

        case 0x0071:
            //printf("RefOfOp\n");
            break;

        case 0x0072:
            //printf("AddOp\n");
            break;

        case 0x0073:
            //printf("ConcatOp\n");
            break;

        case 0x0074:
            //printf("SubtractOp\n");
            break;

        case 0x0075:
            //printf("IncrementOp\n");
            break;

        case 0x0076:
            //printf("DecrementOp\n");
            break;

        case 0x0077:
            //printf("MultiplyOp\n");
            break;

        case 0x0078:
            //printf("DivideOp\n");
            break;

        case 0x0079:
            //printf("ShiftLeftOp\n");
            break;

        case 0x007A:
            //printf("ShiftRightOp\n");
            break;

        case 0x007B:
            //printf("AndOp\n");
            break;

        case 0x007C:
            //printf("NandOp\n");
            break;

        case 0x007D:
            //printf("OrOp\n");
            break;

        case 0x007E:
            //printf("NorOp\n");
            break;

        case 0x007F:
            //printf("XorOp\n");
            break;

        case 0x0080:
            //printf("NotOp\n");
            break;

        case 0x0081:
            //printf("FindSetLeftBitOp\n");
            break;

        case 0x0082:
            //printf("FindSetRightBitOp\n");
            break;

        case 0x0083:
            //printf("DerefOfOp\n");
            break;

        case 0x0084:
            //printf("ConcatResOp\n");
            break;

        case 0x0085:
            //printf("ModOp\n");
            break;

        case 0x0086:
            //printf("NotifyOp\n");
            break;

        case 0x0087:
            //printf("SizeOfOp\n");
            break;

        case 0x0088:
            //printf("IndexOp\n");
            break;

        case 0x0089:
            //printf("MatchOp\n");
            break;

        case 0x008A:
            //printf("CreateDWordFieldOp\n");
            break;

        case 0x008B:
            //printf("CreateWordFieldOp\n");
            break;

        case 0x008C:
            //printf("CreateByteFieldOp\n");
            break;

        case 0x008D:
            //printf("CreateBitFieldOp\n");
            break;

        case 0x008E:
            //printf("ObjectTypeOp\n");
            break;

        case 0x008F:
            //printf("CreateQWordFieldOp\n");
            break;

        case 0x0090:
            //printf("LandOp\n");
            break;

        case 0x0091:
            //printf("LorOp\n");
            break;

        case 0x0092:
            //printf("LNotOp\n");

        case 0x9293:
            //printf("LNotEqualOp\n");
            break;

        case 0x9294:
            //printf("LLessEqualOp\n");
            break;

        case 0x9295:
            //printf("LGreaterEqualOp\n");
            break;

        case 0x0093:
            //printf("LEqualOp\n");
            break;

        case 0x0094:
            //printf("LGreaterOp\n");
            break;

        case 0x0095:
            //printf("LLessOp\n");
            break;

        case 0x0096:
            //printf("ToBufferOp\n");
            break;

        case 0x0097:
            //printf("ToDecimalStringOp\n");
            break;

        case 0x0098:
            //printf("ToHexStringOp\n");
            break;

        case 0x0099:
            //printf("ToIntegerOp\n");
            break;

        case 0x009C:
            //printf("ToStringOp\n");
            break;

        case 0x009D:
            //printf("CopyObjectOp\n");
            break;

        case 0x009E:
            //printf("MidOp\n");
            break;

        case 0x009F:
            //printf("ContinueOp\n");
            break;

        case 0x00A0:
            //printf("IfOp\n");
            break;

        case 0x00A1:
            //printf("ElseOp\n");
            break;

        case 0x00A2:
            //printf("WhileOp\n");
            break;

        case 0x00A3:
            //printf("NoopOp\n");
            break;

        case 0x00A4:
            //printf("ReturnOp\n");
            break;

        case 0x00A5:
            //printf("BreakOp\n");
            break;

        case 0x00CC:
            //printf("BreakPointOp\n");
            break;

        case 0x00FF:
            //printf("OnesOp\n");
            break;
            
        default:
        //printf("Unknown opcode: %x\n", opcode);
        break;
    }
}