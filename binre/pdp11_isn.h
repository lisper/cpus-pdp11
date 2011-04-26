enum {
    ISN_HALT,
    ISN_WAIT,
    ISN_RTI,
    ISN_BPT,
    ISN_IOT,
    ISN_RESET,
    ISN_RTT,
    ISN_MFPT,
    ISN_JMP,
    ISN_RTS,
    ISN_SPL,
    ISN_C,
    ISN_CLC,
    ISN_CLV,
    ISN_CLZ,
    ISN_CLN,
    ISN_CCC,
    ISN_S,
    ISN_SEC,
    ISN_SEV,
    ISN_SEZ,
    ISN_SEN,
    ISN_SCC,
    ISN_SWAB,
    ISN_BR,
    ISN_BNE,
    ISN_BEQ,
    ISN_BGE,
    ISN_BLT,
    ISN_BGT,
    ISN_BLE,
    ISN_JSR,
    ISN_CLR,
    ISN_COM,
    ISN_INC,
    ISN_DEC,
    ISN_NEG,
    ISN_ADC,
    ISN_SBC,
    ISN_TST,
    ISN_ROR,
    ISN_ROL,
    ISN_ASR,
    ISN_ASL,
    ISN_MARK,
    ISN_MFPI,
    ISN_MTPI,
    ISN_SXT,
    ISN_CSM,
    ISN_MOV,
    ISN_CMP,
    ISN_BIT,
    ISN_BIC,
    ISN_BIS,
    ISN_ADD,
    ISN_MUL,
    ISN_DIV,
    ISN_ASH,
    ISN_ASHC,
    ISN_XOR,
    ISN_SOB,
    ISN_BPL,
    ISN_BMI,
    ISN_BHI,
    ISN_BLOS,
    ISN_BVC,
    ISN_BVS,
    ISN_BCC,
    ISN_BHIS,
    ISN_BCS,
    ISN_BLO,
    ISN_EMT,
    ISN_TRAP,
    ISN_CLRB,
    ISN_COMB,
    ISN_INCB,
    ISN_DECB,
    ISN_NEGB,
    ISN_ADCB,
    ISN_SBCB,
    ISN_TSTB,
    ISN_RORB,
    ISN_ROLB,
    ISN_ASRB,
    ISN_ASLB,
    ISN_MTPS,
    ISN_MFPD,
    ISN_MTPD,
    ISN_MFPS,
    ISN_MOVB,
    ISN_CPMB,
    ISN_BITB,
    ISN_BICB,
    ISN_BISB,
    ISN_SUB,
};

enum {
    I_SO = 1,
    I_SOB,
    I_DO,
    I_DOB,
    I_MS,
    I_PC,
    I_CC,
};

enum {
    R_NONE = 0,
    R_DD,
    R_SS,
    R_SSDD,
    R_R,
    R_RSS,
    R_RDD,
    R_N,
    R_NN,
    R_8OFF,
    R_R6OFF,
};


typedef struct {
    int isn_num;
    char *isn_name;
    int isn_type;
    int isn_regs;
    int isn_opcode;
    int isn_opcode2;
} raw_isn_t;

enum {
    M_REG = 0,
    M_REG_DEF = 1,
    M_AUTOINC = 2,
    M_AUTOINC_DEF = 3,
    M_AUTODEC = 4,
    M_AUTODEC_DEF = 5,
    M_INDEXED = 6,
    M_INDEX_DEF = 7
};



/*
 * Local Variables:
 * indent-tabs-mode:nil
 * c-basic-offset:4
 * End:
*/
