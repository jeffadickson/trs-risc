using System.Security.Cryptography;
using System.Text;

namespace TrsRisc.Tools;

/// <summary>A single build-time decoded instruction, with its original Z80 listing.</summary>
/// <param name="Code">Native C operations, including the next PC.</param>
/// <param name="Assembly">Z80 mnemonic, using original absolute addresses.</param>
/// <param name="Length">Number of ROM bytes consumed, including prefixes.</param>
/// <param name="M1">Opcode-fetch count, used to advance the Z80 refresh register.</param>
public sealed record Instruction(string Code, string Assembly, int Length, int M1);

/// <summary>
/// Ahead-of-time Z80-to-C translator. C is an intermediate language compiled to
/// RV64, not a runtime opcode interpreter. Every byte offset gets an entry point
/// because an indirect jump can enter an overlapping instruction stream.
/// </summary>
/// <remarks>
/// Example: Z80 <c>21 80 40 / LD HL,0x4080</c> becomes <c>hl=16512;pc=next;</c>.
/// The original bytes and mnemonic are emitted as a comment beside that code.
/// </remarks>
public sealed class Translator(byte[] rom)
{
    private static readonly string[] Conditions = ["!(f&Z)", "f&Z", "!(f&C)", "f&C", "!(f&P)", "f&P", "!(f&S)", "f&S"];
    private static readonly string[] ConditionNames = ["NZ", "Z", "NC", "C", "PO", "PE", "P", "M"];
    private static readonly string[] Rotates = ["RLC", "RRC", "RL", "RR", "SLA", "SRA", "SLL", "SRL"];

    /// <summary>Read one ROM byte. A missing instruction operand is an error.</summary>
    /// <remarks>Z80 equivalent: reading the opcode/immediate at the original PC.</remarks>
    private int Byte(int address) => address < rom.Length ? rom[address] : throw new InvalidDataException("instruction crosses ROM boundary");

    /// <summary>Decode one Z80 instruction and emit its explicit C semantics.</summary>
    /// <remarks>
    /// Z80 families: base, CB, ED, DD, FD, DD-CB-d, FD-CB-d. The x/y/z fields
    /// are the opcode's top two bits, middle three bits, and bottom three bits.
    /// Example: <c>ED B0 / LDIR</c> copies one byte, decrements BC, and returns
    /// to this instruction while BC is nonzero. No bytecode decode occurs at run time.
    /// </remarks>
    public Instruction Decode(int start)
    {
        int p = start, m1 = 1;
        // Z80 immediates are little endian. Fetch advances only the build-time PC.
        int Fetch() => Byte(p++);
        int Word() => Fetch() | Fetch() << 8;
        int op = Fetch(); string? index = null; int? displacement = null;
        // Repeated DD/FD prefixes: the last one selects IX or IY.
        while (op is 0xdd or 0xfd) { index = op == 0xdd ? "ix" : "iy"; op = Fetch(); m1++; }
        string Address()
        {
            if (index is null) return "hl";
            if (displacement is null) { int value = Fetch(); displacement = value >= 128 ? value - 256 : value; }
            return $"(uint16_t)({index}+({displacement}))";
        }
        string MemoryName() { Address(); return index is null ? "(HL)" : $"({index.ToUpperInvariant()}{displacement:+0;-0;+0})"; }
        // The H/L in LD H,(IX+d) and LD (IX+d),H are ordinary H/L, not IXH/IXL.
        string Reg(int r, bool ordinary = false)
        {
            if (r == 6) return $"rd({Address()})";
            if (index is not null && r is 4 or 5 && !ordinary) return r == 4 ? $"({index}>>8)" : $"({index}&255)";
            return new[] { "b", "c", "d", "e", "h", "l", "", "a" }[r];
        }
        string RegName(int r, bool ordinary = false)
        {
            if (r == 6) return MemoryName();
            if (index is not null && r is 4 or 5 && !ordinary) return index.ToUpperInvariant() + (r == 4 ? "H" : "L");
            return new[] { "B", "C", "D", "E", "H", "L", "", "A" }[r];
        }
        string Put(int r, string value, bool ordinary = false)
        {
            if (r == 6) return $"wr({Address()},{value});";
            if (index is not null && r is 4 or 5 && !ordinary)
                return $"{index}=({index}&{(r == 4 ? 255 : 65280)})|((uint16_t)(uint8_t)({value}){(r == 4 ? "<<8" : "")});";
            return $"{Reg(r, ordinary)}=(uint8_t)({value});";
        }
        string Pair(int r, bool useAf = false) => new[] { "bc", "de", index ?? "hl", useAf ? "af" : "sp" }[r];
        string PairName(int r, bool useAf = false) => Pair(r, useAf).ToUpperInvariant();
        string Cond(int r) => Conditions[r];
        string AluName(int r, string value) => new[] { $"ADD A,{value}", $"ADC A,{value}", $"SUB {value}", $"SBC A,{value}", $"AND {value}", $"XOR {value}", $"OR {value}", $"CP {value}" }[r];
        string code = "", assembly = "NOP"; bool terminal = false;
        if (op == 0xcb)
        {
            // DD CB d op: d precedes the final opcode, which is not an M1 fetch.
            string? address = index is null ? null : Address();
            if (index is null) m1++;
            int cb = Fetch(), x = cb >> 6, y = cb >> 3 & 7, z = cb & 7;
            string operand = index is null ? RegName(z) : MemoryName();
            code = $"v={(index is null ? Reg(z) : $"rd({address})")};";
            if (x == 0) { code += $"v=rotate({y},v);"; assembly = $"{Rotates[y]} {operand}"; }
            else if (x == 1)
            {
                code += $"f=(f&C)|H|(v&(1<<{y})?0:Z|P)|({y}==7?(v&S):0)|({(index is null ? "v" : $"({address}>>8)")}&(X|Y));";
                assembly = $"BIT {y},{operand}";
            }
            else { code += x == 2 ? $"v&=~(1<<{y});" : $"v|=1<<{y};"; assembly = $"{(x == 2 ? "RES" : "SET")} {y},{operand}"; }
            if (x != 1)
            {
                if (index is not null) { code += $"wr({address},v);"; if (z != 6) { code += Put(z, "v", true); assembly += $",{RegName(z, true)}"; } }
                else code += Put(z, "v");
            }
        }
        else if (op == 0xed)
        {
            // ED ignores earlier index substitution; all its HL operands are HL.
            m1++; int ed = Fetch(), x = ed >> 6, y = ed >> 3 & 7, z = ed & 7, q = y & 1, rp = y >> 1;
            index = null; assembly = $"NOP ; undefined ED 0x{ed:X2}";
            if (x == 1)
            {
                switch (z)
                {
                    case 0: code = "v=inport(bc);f=(f&C)|szp(v);"; if (y != 6) code += Put(y, "v", true); assembly = y == 6 ? "IN (C)" : $"IN {RegName(y)},(C)"; break;
                    case 1: code = $"outport(bc,{(y == 6 ? "0" : Reg(y, true))});"; assembly = $"OUT (C),{(y == 6 ? "0" : RegName(y))}"; break;
                    case 2: code = $"hl=adc16(hl,{Pair(rp)},{(q != 0 ? 0 : 1)});"; assembly = $"{(q != 0 ? "ADC" : "SBC")} HL,{PairName(rp)}"; break;
                    case 3:
                        int nn = Word(); string rr = Pair(rp);
                        code = q != 0 ? $"{rr}=rd16({nn});" : $"wr16({nn},{rr});";
                        assembly = q != 0 ? $"LD {rr.ToUpperInvariant()},(0x{nn:X4})" : $"LD (0x{nn:X4}),{rr.ToUpperInvariant()}"; break;
                    case 4: code = "v=a;a=0;alu(2,v);"; assembly = "NEG"; break;
                    case 5: code = "pc=pop();iff1=iff2;"; assembly = ed == 0x4d ? "RETI" : "RETN"; terminal = true; break;
                    case 6: int mode = new[] { 0, 0, 1, 2, 0, 0, 1, 2 }[y]; code = $"im={mode};"; assembly = $"IM {mode}"; break;
                    case 7:
                        if (y == 0) { code = "ir=a;"; assembly = "LD I,A"; }
                        else if (y == 1) { code = "refresh=a;"; assembly = "LD R,A"; }
                        else if (y is 2 or 3) { code = $"a={(y == 2 ? "ir" : "refresh")};f=(f&C)|sz(a)|(iff2?P:0);"; assembly = y == 2 ? "LD A,I" : "LD A,R"; }
                        else if (y is 4 or 5) { code = "v=rd(hl);" + (y == 4 ? "wr(hl,(a<<4)|(v>>4));a=(a&240)|(v&15);" : "wr(hl,(v<<4)|(a&15));a=(a&240)|(v>>4);") + "f=(f&C)|szp(a);"; assembly = y == 4 ? "RRD" : "RLD"; }
                        break;
                }
            }
            else if (x == 2 && y >= 4 && z <= 3)
            {
                int direction = (y & 1) != 0 ? -1 : 1; string test;
                assembly = new[] { new[] { "LDI", "CPI", "INI", "OUTI" }, new[] { "LDD", "CPD", "IND", "OUTD" }, new[] { "LDIR", "CPIR", "INIR", "OTIR" }, new[] { "LDDR", "CPDR", "INDR", "OTDR" } }[y - 4][z];
                if (z == 0) { code = $"v=rd(hl);wr(de,v);hl+={direction};de+={direction};bc--;v+=a;f=(f&(S|Z|C))|(bc?P:0)|(v&X)|((v&2)<<4);"; test = "bc"; }
                else if (z == 1) { code = $"v=rd(hl);t=(uint8_t)(a-v);hl+={direction};bc--;f=(f&C)|N|sz(t)|(((a^v^t)&16)?H:0)|(bc?P:0);v=t-((f&H)?1:0);f=(f&~(X|Y))|(v&X)|((v&2)<<4);"; test = "bc && !(f&Z)"; }
                else
                {
                    code = z == 2 ? "v=inport(bc);wr(hl,v);" : "v=rd(hl);outport(bc,v);";
                    code += $"hl+={direction};b--;t=v+({(z == 2 ? $"(uint8_t)(c+({direction}))" : "l")});f=sz(b)|(v&128?N:0)|(t>255?H|C:0)|(parity((t&7)^b)?P:0);"; test = "b";
                }
                if (y >= 6) code += $"if({test}){{pc={start};return;}}";
            }
        }
        else
        {
            int x = op >> 6, y = op >> 3 & 7, z = op & 7, q = y & 1, rp = y >> 1;
            string hp = index ?? "hl", hn = hp.ToUpperInvariant();
            if (x == 0)
            {
                switch (z)
                {
                    case 0:
                        if (y == 0) assembly = "NOP";
                        else if (y == 1) { code = "t=af;af=af2;af2=t;"; assembly = "EX AF,AF'"; }
                        else
                        {
                            int value = Fetch(), displacementValue = value >= 128 ? value - 256 : value, target = (p + displacementValue) & 65535;
                            if (y == 2) { code = $"if(--b){{pc={target};return;}}"; assembly = $"DJNZ 0x{target:X4}"; }
                            else if (y == 3) { code = $"pc={target};"; assembly = $"JR 0x{target:X4}"; terminal = true; }
                            else { code = $"if({Cond(y - 4)}){{pc={target};return;}}"; assembly = $"JR {ConditionNames[y - 4]},0x{target:X4}"; }
                        }
                        break;
                    case 1:
                        if (q == 0) { int nn = Word(); code = $"{Pair(rp)}={nn};"; assembly = $"LD {PairName(rp)},0x{nn:X4}"; }
                        else { code = $"{hp}=add16({hp},{Pair(rp)});"; assembly = $"ADD {hn},{PairName(rp)}"; } break;
                    case 2:
                        string operand, targetName;
                        if (rp < 2) { code = q != 0 ? $"a=rd({Pair(rp)});" : $"wr({Pair(rp)},a);"; operand = $"({PairName(rp)})"; targetName = "A"; }
                        else
                        {
                            int nn = Word(); operand = $"(0x{nn:X4})"; targetName = rp == 2 ? hn : "A";
                            code = rp == 2 ? (q != 0 ? $"{hp}=rd16({nn});" : $"wr16({nn},{hp});") : (q != 0 ? $"a=rd({nn});" : $"wr({nn},a);");
                        }
                        assembly = q != 0 ? $"LD {targetName},{operand}" : $"LD {operand},{targetName}"; break;
                    case 3: code = $"{Pair(rp)}{(q != 0 ? "--" : "++")};"; assembly = $"{(q != 0 ? "DEC" : "INC")} {PairName(rp)}"; break;
                    case 4: case 5: code = $"v=incdec({Reg(y)},{(z == 5 ? 1 : 0)});" + Put(y, "v"); assembly = $"{(z == 5 ? "DEC" : "INC")} {RegName(y)}"; break;
                    case 6:
                        if (y == 6) Address(); int immediate = Fetch(); code = Put(y, immediate.ToString()); assembly = $"LD {RegName(y)},0x{immediate:X2}"; break;
                    case 7:
                        assembly = new[] { "RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF" }[y];
                        code = y < 4 ? $"t=f;a=rotate({y},a);f=(t&(S|Z|P))|(f&C)|(a&(X|Y));" : y switch
                        { 4 => "daa();", 5 => "a^=255;f=(f&(S|Z|P|C))|H|N|(a&(X|Y));", 6 => "f=(f&(S|Z|P))|C|(a&(X|Y));", _ => "f=(f&(S|Z|P))|(f&C?H:C)|(a&(X|Y));" }; break;
                }
            }
            else if (x == 1)
            {
                if (y == 6 && z == 6) { code = "halted=1;"; assembly = "HALT"; }
                else { if (z == 6 || y == 6) Address(); code = Put(y, Reg(z, y == 6), z == 6); assembly = $"LD {RegName(y, z == 6)},{RegName(z, y == 6)}"; }
            }
            else if (x == 2) { code = $"alu({y},{Reg(z)});"; assembly = AluName(y, RegName(z)); }
            else
            {
                switch (z)
                {
                    case 0: code = $"if({Cond(y)}){{pc=pop();return;}}"; assembly = $"RET {ConditionNames[y]}"; break;
                    case 1:
                        if (q == 0) { code = $"{Pair(rp, true)}=pop();"; assembly = $"POP {PairName(rp, true)}"; }
                        else if (rp == 0) { code = "pc=pop();"; assembly = "RET"; terminal = true; }
                        else if (rp == 1) { code = "t=bc;bc=bc2;bc2=t;t=de;de=de2;de2=t;t=hl;hl=hl2;hl2=t;"; assembly = "EXX"; }
                        else if (rp == 2) { code = $"pc={hp};"; assembly = $"JP ({hn})"; terminal = true; }
                        else { code = $"sp={hp};"; assembly = $"LD SP,{hn}"; } break;
                    case 2: int target = Word(); code = $"if({Cond(y)}){{pc={target};return;}}"; assembly = $"JP {ConditionNames[y]},0x{target:X4}"; break;
                    case 3:
                        if (y == 0) { int nn = Word(); code = $"pc={nn};"; assembly = $"JP 0x{nn:X4}"; terminal = true; }
                        else if (y == 2) { int port = Fetch(); code = $"outport((a<<8)|{port},a);"; assembly = $"OUT (0x{port:X2}),A"; }
                        else if (y == 3) { int port = Fetch(); code = $"a=inport((a<<8)|{port});"; assembly = $"IN A,(0x{port:X2})"; }
                        else if (y == 4) { code = $"t=rd16(sp);wr16(sp,{hp});{hp}=t;"; assembly = $"EX (SP),{hn}"; }
                        else if (y == 5) { code = "t=de;de=hl;hl=t;"; assembly = "EX DE,HL"; }
                        else { code = y == 6 ? "iff1=iff2=0;" : "iff1=iff2=1;"; assembly = y == 6 ? "DI" : "EI"; } break;
                    case 4: int dest = Word(); code = $"if({Cond(y)}){{push({p});pc={dest};return;}}"; assembly = $"CALL {ConditionNames[y]},0x{dest:X4}"; break;
                    case 5:
                        if (q == 0) { code = $"push({Pair(rp, true)});"; assembly = $"PUSH {PairName(rp, true)}"; }
                        else { int nn = Word(); code = $"push({p});pc={nn};"; assembly = $"CALL 0x{nn:X4}"; terminal = true; } break;
                    case 6: int n = Fetch(); code = $"alu({y},{n});"; assembly = AluName(y, $"0x{n:X2}"); break;
                    case 7: code = $"push({p});pc={y * 8};"; assembly = $"RST 0x{y * 8:X2}"; terminal = true; break;
                }
            }
        }
        return new Instruction(code + (terminal ? "" : $"pc={p & 65535};"), assembly, p - start, m1);
    }

    /// <summary>Emit C and mixed Z80/native comments for all byte entry points.</summary>
    /// <remarks>Z80 example: <c>0000: F3 DI</c> immediately precedes <c>iff1=iff2=0</c>.
    /// Offsets inside data are possible entry points, not asserted ROM function boundaries.</remarks>
    public string Generate()
    {
        var text = new StringBuilder("/* Generated by TrsRisc.Tools; do not edit. */\n");
        text.Append("static const uint8_t rom_image[]={").AppendJoin(',', rom.Select(value => value.ToString())).Append("};\n");
        text.Append("/* Z80 PC dispatch; executes native statements, never decodes an opcode. */\nstatic void translated_step(void){uint16_t t;uint8_t v;switch(pc){\n");
        for (int pc = 0; pc < rom.Length; pc++)
        {
            try
            {
                Instruction instruction = Decode(pc);
                string bytes = Convert.ToHexString(rom.AsSpan(pc, instruction.Length));
                text.Append($"/* Z80 {pc:X4}: {bytes}  {instruction.Assembly} */\n");
                text.Append($"case {pc}: refresh=(refresh&128)|((refresh+{instruction.M1})&127); {instruction.Code} return;\n");
            }
            catch (InvalidDataException)
            {
                text.Append($"/* Z80 {pc:X4}: truncated instruction; no operand is invented. */\ncase {pc}: fail(\"truncated ROM instruction\",{pc}); return;\n");
            }
        }
        return text.Append("default: ram_hook();return;\n}}\n").ToString();
    }

    /// <summary>Write translated C; log byte count and digest for reproducibility.</summary>
    public static void TranslateFile(string input, string output)
    {
        byte[] image = File.ReadAllBytes(input);
        if (image.Length is 0 or > 0x3800) throw new InvalidDataException("expected a ROM below keyboard memory");
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(output))!);
        File.WriteAllText(output, new Translator(image).Generate());
        Console.WriteLine($"ROM={image.Length} entry_points={image.Length} sha256={Convert.ToHexString(SHA256.HashData(image)).ToLowerInvariant()}");
    }
}
