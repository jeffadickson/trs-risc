/*
 * Conservative, side-effect-free Level II syntax pass over ROM tokens.
 * This is a NEW host parser, NOT the ROM's runtime evaluator. It reports all
 * reachable tokenized statements without executing BASIC. Unsupported disk,
 * tape, editor, or device syntax is rejected explicitly rather than guessed.
 *
 * Z80/token correspondence (ROM keyword table starts at 1650h):
 *   80 END, 81 FOR, 87 NEXT, 88 DATA, 8F IF, 93 REM, B2 PRINT,
 *   BD TO, CA THEN, CB NOT, CC STEP, CD..D6 operators, D7..FA functions.
 * Error recovery advances to the next ':' outside a quoted string, allowing
 * independent errors on later statements and lines to be reported in one pass.
 */
typedef struct {const unsigned char *pos,*end,*begin;unsigned line;int failed;} Syntax;
static int syntax_errors;

/* No Z80 counterpart: collect diagnostics rather than taking BASIC's global
   ?SN error jump, which aborts execution at its first encountered error. */
static void syntax_error(Syntax *parser,const char *message){
    if(!parser->failed){fprintf(stderr,"BASIC line %u, token byte %zu: %s\n",parser->line,(size_t)(parser->pos-parser->begin)+1,message);syntax_errors++;}
    parser->failed=1;
}

/* Z80's CHRGET skips spaces while advancing its text pointer. Here the pointer
   is host memory and is bounded by the validated line terminator. */
static int peek_token(Syntax *parser){while(parser->pos<parser->end&&*parser->pos==' ')parser->pos++;return parser->pos<parser->end?*parser->pos:0;}
static int accept_token(Syntax *parser,int token){if(peek_token(parser)==token){parser->pos++;return 1;}return 0;}
static void require_token(Syntax *parser,int token,const char *message){if(!accept_token(parser,token))syntax_error(parser,message);}
static int statement_end(Syntax *parser){int token=peek_token(parser);return !token||token==':'||token==0x95;}

/* Z80 evaluator equivalents: OR/AND, comparisons, ADD/SUB, MUL/DIV, and '['
   exponentiation. These numbers control parsing only; no arithmetic is done. */
static int precedence(int token){
    if(token==0xd3)return 1;if(token==0xd2)return 2;
    if(token>=0xd4&&token<=0xd6)return 3;
    if(token==0xcd||token==0xce)return 4;
    if(token==0xcf||token==0xd0)return 5;
    if(token==0xd1)return 6;return 0;
}
static void expression(Syntax *parser,int minimum);

/* Parse A, A$, A(1,2), etc. Z80 variable lookup/type checking is deferred to
   the ROM at RUN time; this function checks spelling and subscript syntax. */
static void variable(Syntax *parser){
    int token=peek_token(parser);
    if(!isalpha(token)){syntax_error(parser,"expected variable name");return;}
    while(parser->pos<parser->end&&isalnum(*parser->pos))parser->pos++;
    token=peek_token(parser);if(token=='$'||token=='%'||token=='!'||token=='#')parser->pos++;
    if(accept_token(parser,'(')){
        expression(parser,1);while(accept_token(parser,','))expression(parser,1);
        require_token(parser,')',"missing ')' in subscript");
    }
}

/* Parse the same surface operands consumed by BASIC's expression evaluator:
   strings, numeric literals, variables, parentheses and builtin/FN/USR calls.
   Types, function arity and numeric range are intentionally runtime concerns. */
static void primary(Syntax *parser){
    int token=peek_token(parser);if(parser->failed)return;
    if(token==0xcd||token==0xce||token==0xcb){parser->pos++;expression(parser,token==0xcb?3:6);return;}
    if(accept_token(parser,'(')){expression(parser,1);require_token(parser,')',"missing ')'");return;}
    if(accept_token(parser,'"')){
        while(parser->pos<parser->end&&*parser->pos!='"')parser->pos++;
        require_token(parser,'"',"unterminated string");return;
    }
    if(isdigit(token)||token=='.'){
        int digits=0;
        while(parser->pos<parser->end&&isdigit(*parser->pos)){parser->pos++;digits++;}
        if(accept_token(parser,'.'))while(parser->pos<parser->end&&isdigit(*parser->pos)){parser->pos++;digits++;}
        if(!digits){syntax_error(parser,"number has no digits");return;}
        token=peek_token(parser);
        if(token=='E'||token=='D'){
            parser->pos++;token=peek_token(parser);if(token=='+'||token=='-'||token==0xcd||token==0xce)parser->pos++;
            if(!isdigit(peek_token(parser)))syntax_error(parser,"missing exponent digits");
            while(parser->pos<parser->end&&isdigit(*parser->pos))parser->pos++;
        }
        token=peek_token(parser);if(token=='!'||token=='#'||token=='%')parser->pos++;
        return;
    }
    if(token==0xc2||token==0xc3||token==0xc7||token==0xc8||token==0xc9){parser->pos++;return;}
    if((token>=0xd7&&token<=0xfa)||token==0xc0||token==0xc1||token==0xc4||token==0xc5||token==0xc6||token==0xbe){
        parser->pos++;
        if(token==0xbe){if(!isalpha(peek_token(parser))){syntax_error(parser,"FN requires a name");return;}while(isalnum(peek_token(parser)))parser->pos++;}
        if(token==0xc1&&isdigit(peek_token(parser)))parser->pos++;
        require_token(parser,'(',"function requires '('");
        expression(parser,1);while(accept_token(parser,','))expression(parser,1);
        require_token(parser,')',"missing ')' in function call");return;
    }
    if(isalpha(token)){variable(parser);return;}
    syntax_error(parser,"expected expression");
}

/* Precedence climbing checks expression structure without touching AF, HL,
   BASIC variables, or host I/O. '[' is the ROM's actual power token spelling. */
static void expression(Syntax *parser,int minimum){
    primary(parser);
    while(!parser->failed){
        int token=peek_token(parser),level=precedence(token);if(level<minimum||!level)return;
        parser->pos++;
        if(level==3){int second=peek_token(parser);if(second>=0xd4&&second<=0xd6)parser->pos++;}
        expression(parser,level+(token==0xd1?0:1));
    }
}

/* Parse comma-separated references used by READ, NEXT, DIM and INPUT. */
static void variables(Syntax *parser){variable(parser);while(!parser->failed&&accept_token(parser,','))variable(parser);}
static void statement(Syntax *parser);

/* Z80 IF/THEN can transfer to a line number or execute an inline statement.
   Consume each side structurally, even though execution would choose only one. */
static void branch(Syntax *parser){
    if(isdigit(peek_token(parser))){while(isdigit(peek_token(parser)))parser->pos++;}
    else statement(parser);
}

/* Match the tokenized statement families accepted by this static checker.
   Disk/tape/editor statements get explicit coverage errors. This deliberate
   restriction prevents an unchecked construct from being called "valid". */
static void statement(Syntax *parser){
    int token=peek_token(parser);if(parser->failed||statement_end(parser))return;
    if(isalpha(token)){variable(parser);require_token(parser,0xd5,"assignment requires '='");expression(parser,1);return;}
    parser->pos++;
    switch(token){
    case 0x80:case 0x84:case 0x92:case 0x94:case 0x96:case 0x97:case 0xae:case 0xb3:case 0xbb:
        return; /* END CLS RETURN STOP TRON TROFF SYSTEM CONT NEW */
    case 0x93:case 0xfb:parser->pos=parser->end;return; /* REM / apostrophe */
    case 0x88: /* DATA's payload is literal text, not an expression list. */
        {int quoted=0;while(parser->pos<parser->end){int ch=*parser->pos;if(ch=='"')quoted=!quoted;if(ch==':'&&!quoted)break;parser->pos++;}
        if(quoted)syntax_error(parser,"unterminated DATA string");}return;
    case 0x8c:case 0xab:case 0xac: /* LET, LSET, RSET */
        variable(parser);require_token(parser,0xd5,"assignment requires '='");expression(parser,1);return;
    case 0x81: /* FOR v=lo TO hi [STEP delta] */
        variable(parser);require_token(parser,0xd5,"FOR requires '='");expression(parser,1);
        require_token(parser,0xbd,"FOR requires TO");expression(parser,1);
        if(accept_token(parser,0xcc))expression(parser,1);return;
    case 0x87:if(!statement_end(parser))variables(parser);return;
    case 0x8a:case 0x8b:variables(parser);return; /* DIM READ */
    case 0x89: /* INPUT ["prompt";] v,... */
        if(peek_token(parser)=='"'){primary(parser);require_token(parser,';',"INPUT prompt requires ';'");}variables(parser);return;
    case 0x9c:require_token(parser,0x89,"only LINE INPUT is covered");
        if(peek_token(parser)=='"'){primary(parser);require_token(parser,';',"LINE INPUT prompt requires ';'");}variable(parser);return;
    case 0x8d:case 0x91:case 0x9e:expression(parser,1);return; /* GOTO GOSUB ERROR */
    case 0x8e:case 0x90:case 0x9f:case 0x86: /* RUN RESTORE RESUME RANDOM */
        if(!statement_end(parser))expression(parser,1);return;
    case 0x8f: /* IF expr THEN/GOTO branch [ELSE branch] */
        expression(parser,1);
        if(!accept_token(parser,0xca)&&!accept_token(parser,0x8d))syntax_error(parser,"IF requires THEN or GOTO");
        branch(parser);
        if(peek_token(parser)==':'&&parser->pos+1<parser->end&&parser->pos[1]==0x95)parser->pos++;
        if(accept_token(parser,0x95))branch(parser);return;
    case 0xa1: /* ON expr GOTO/GOSUB targets; ON ERROR GOTO target */
        if(accept_token(parser,0x9e))require_token(parser,0x8d,"ON ERROR requires GOTO");
        else {expression(parser,1);if(!accept_token(parser,0x8d)&&!accept_token(parser,0x91))syntax_error(parser,"ON requires GOTO or GOSUB");}
        expression(parser,1);while(accept_token(parser,','))expression(parser,1);return;
    case 0x82:case 0x83: /* RESET(x,y), SET(x,y) */
        require_token(parser,'(',"SET/RESET requires '('");expression(parser,1);require_token(parser,',',"SET/RESET requires two coordinates");expression(parser,1);require_token(parser,')',"missing ')'");return;
    case 0xa0:case 0xb1: /* OUT port,value; POKE address,value */
        expression(parser,1);require_token(parser,',',"expected two comma-separated expressions");expression(parser,1);return;
    case 0xb8: /* CLEAR [string_space[,memory_top]] */
        if(!statement_end(parser)){expression(parser,1);if(accept_token(parser,','))expression(parser,1);}return;
    case 0xb2:case 0xaf: /* PRINT / LPRINT */
        if(accept_token(parser,'@')){expression(parser,1);require_token(parser,',',"PRINT @ requires ','");}
        if(accept_token(parser,0xbf)){expression(parser,1);require_token(parser,';',"PRINT USING requires ';'");}
        while(!parser->failed&&!statement_end(parser)){
            if(accept_token(parser,';')||accept_token(parser,','))continue;
            if(accept_token(parser,0xbc)){expression(parser,1);require_token(parser,')',"TAB requires ')'");}
            else expression(parser,1);
            if(!statement_end(parser)&&peek_token(parser)!=';'&&peek_token(parser)!=',')syntax_error(parser,"PRINT items require ',' or ';'");
        }return;
    case 0x98:case 0x99:case 0x9a:case 0x9b: /* DEFSTR/INT/SNG/DBL letter ranges */
        do {if(!isalpha(peek_token(parser))){syntax_error(parser,"DEF type requires letter range");return;}parser->pos++;
            if(accept_token(parser,0xce)){if(!isalpha(peek_token(parser))){syntax_error(parser,"range requires a letter");return;}parser->pos++;}
        }while(accept_token(parser,','));return;
    case 0xb0: /* DEF FNname(parameter)=expression */
        require_token(parser,0xbe,"DEF requires FN");variable(parser);require_token(parser,0xd5,"DEF FN requires '='");expression(parser,1);return;
    default:syntax_error(parser,"unsupported or unknown statement in static-check grammar");return;
    }
}

/* Walk every stored line, independent of flow control. Unlike executing RUN,
   this finds structural errors in both IF branches and after END/GOTO as well.
   One diagnostic is emitted per failing statement, then recovery continues. */
static int check_program(const unsigned char *bytes,size_t length){
    (void)length;syntax_errors=0;size_t offset=0;
    while(bytes[offset]||bytes[offset+1]){
        unsigned next=(bytes[offset]|(bytes[offset+1]<<8))-BASIC_BASE;
        Syntax parser={bytes+offset+4,bytes+next-1,bytes+offset+4,
            (unsigned)(bytes[offset+2]|(bytes[offset+3]<<8)),0};
        while(peek_token(&parser)){
            const unsigned char *before=parser.pos;
            if(peek_token(&parser)==0x95)syntax_error(&parser,"ELSE without a matching IF");
            statement(&parser);
            if(!parser.failed&&!statement_end(&parser))syntax_error(&parser,"unexpected text after statement");
            if(parser.failed){int quoted=0;while(parser.pos<parser.end){if(*parser.pos=='"')quoted=!quoted;if(*parser.pos==':'&&!quoted)break;parser.pos++;}}
            if(accept_token(&parser,':')){parser.failed=0;continue;}
            if(parser.pos==before||parser.failed||peek_token(&parser)==0x95)break;
        }
        offset=next;
    }
    return syntax_errors;
}
