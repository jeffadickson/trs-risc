using System.Text;
using System.Text.RegularExpressions;

namespace TrsRisc.Tools;

/// <summary>Produce a readable, reassemblable RV32 math listing with documented Z80 ancestry.</summary>
/// <remarks>Annotations describe an algorithm port, never instruction-for-instruction equivalence.</remarks>
public static class AnnotatedAssembly
{
    private static readonly Regex Function = new(@"^\s*\.type\s+(\w+),\s*@function\s*$");
    private static readonly Regex Instruction = new(@"^\s+([a-z][a-z0-9.]*)\s*(.*)$");
    private static readonly Regex Location = new(@"^\s*\.loc\s+(\d+)\s+(\d+)");
    private static readonly Regex SourceFile = new("^\\s*\\.file\\s+(\\d+)\\s+\"[^\"]*\"\\s+\"([^\"]+)\"");

    /// <summary>Annotate compiler assembly from the repository root; validate before publishing.</summary>
    public static void Generate(string input, string output)
    {
        string raw = File.ReadAllText(input);
        string listing = Render(raw, path => File.ReadAllLines(path));
        Validate(raw, listing);
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(output))!);
        File.WriteAllText(output, listing, new UTF8Encoding(false));
        Console.WriteLine($"Annotated {FunctionNames(raw).Length} RV32 functions: {output}");
    }

    /// <summary>Render comments while preserving instruction, label, and data directive order.</summary>
    public static string Render(string raw, Func<string, string[]> readSource)
    {
        var result = new StringBuilder("# RV32IMA math study listing; Zig 0.14.1, -Oz -fno-inline, soft-float ILP32.\n" +
            "# Generated from math.c, math_kernel.inc and exp_double.inc.\n" +
            "# Z80 excerpts are documented algorithm/call-site excerpts, not full ROM disassembly.\n" +
            "# -fno-inline preserves teaching boundaries; this is NOT the linked firmware layout.\n" +
            "# a0-a7: integer/pointer arguments; a0/a1 also carry binary64 words.\n" +
            "# s0-s11: callee-saved; ra: return address; sp: 16-byte-aligned stack.\n" +
            "# No F/D instructions: __*sf* / __*df* calls perform software IEEE arithmetic.\n" +
            "# SINGLE rounds at binary32 operations; DOUBLE uses binary64.\n\n");
        var files = new Dictionary<int, (string Path, string[] Lines)>();
        string previousSource = "";
        foreach (string line in raw.Replace("\r", "").Split('\n'))
        {
            // Debug payload is trailing compiler metadata, not executable text or constants.
            if (Regex.IsMatch(line, @"^\s*\.section\s+\.?\""?\.debug")) break;
            Match file = SourceFile.Match(line);
            if (file.Success)
            {
                string path = file.Groups[2].Value;
                if (!path.StartsWith("src/native32/", StringComparison.Ordinal) || path.Contains(".."))
                    throw new InvalidDataException("Unexpected assembly source path");
                files[int.Parse(file.Groups[1].Value)] = (path, readSource(path));
                continue;
            }
            Match location = Location.Match(line);
            if (location.Success)
            {
                int number = int.Parse(location.Groups[2].Value);
                if (number != 0)
                {
                    var source = files[int.Parse(location.Groups[1].Value)];
                    if (number > source.Lines.Length) throw new InvalidDataException("Source line outside file");
                    string comment = $"# C {source.Path}:{number}: {source.Lines[number - 1].Trim()}";
                    if (comment != previousSource) result.AppendLine(comment);
                    previousSource = comment;
                }
                continue;
            }
            if (Regex.IsMatch(line, @"^\s*\.(file|cfi_\w+)\b")) continue;
            Match function = Function.Match(line);
            if (function.Success)
            {
                string name = function.Groups[1].Value;
                result.AppendLine("\n# ======================================================================");
                result.AppendLine($"# FUNCTION {name}");
                foreach (string comment in Ancestry(name).Split('\n')) result.AppendLine("# " + comment);
                result.AppendLine("# RV32: saved-register spills preserve caller state; branches implement C control flow.");
                result.AppendLine("# Z80 addresses refer to the pinned Model I Level II ROM (see native32-math.rst).");
                result.AppendLine("# ======================================================================");
            }
            Match instruction = Instruction.Match(line);
            if (instruction.Success)
            {
                string operands = instruction.Groups[2].Value.Split('#')[0].Trim();
                result.AppendLine($"{line} # RV32: {Explain(instruction.Groups[1].Value, operands)}");
            }
            else result.AppendLine(line);
        }
        result.AppendLine("\n.section .note.GNU-stack,\"\",@progbits");
        return result.ToString().Replace("\r", "");
    }

    private static string Ancestry(string name)
    {
        string key = Regex.Replace(name, @"^(single_|double_)", "");
        string precision = name.StartsWith("single_") ? "binary32" : "binary64";
        return key switch
        {
            "polynomial" => $"Horner polynomial, {precision}; coefficient count includes the leading term.\nZ80 14A9h: CALL 09A4h ; save argument\nZ80 14ACh: LD A,(HL); INC HL; CALL 09B1h ; load coefficient count/value\nZ80 14BBh: CALL 0847h ; accumulator *= x\nZ80 14C3h: CALL 0716h ; accumulator += coefficient\nRV32 loop advances native coefficient words; no MBF byte decoder or Z80 flags.",
            "odd" => $"Odd polynomial x*P(x*x), {precision}.\nZ80 149Ah: CALL 09A4h ; save x\nZ80 14A5h: CALL 0847h ; square saved x\nZ80 14A8h: POP HL ; fall through into Horner at 14A9h; stacked return later multiplies by saved x\nRV32 retains x across the polynomial call using stack/callee-saved registers.",
            "logarithm" => $"LOG, {precision}: normalize x=m*2^k; rational transform; odd polynomial.\nZ80 0809h: CALL 0955h; OR A; JP PE,1E4Ah ; reject invalid argument\nZ80 0810h: LD HL,4124h; LD A,(HL) ; accumulator exponent\nZ80 0814h: LD BC,8035h; LD DE,04F3h\nZ80 081Ah: SUB B; PUSH AF; LD (HL),B ; save exponent, normalize\nZ80: LD HL,07F8h; CALL 0710h ; table value minus accumulator\nZ80: LD HL,07FCh; CALL 149Ah ; z*P(z*z)\nRV32 extracts IEEE exponent fields and uses word-float add/divide/multiply helpers.",
            "exponential" => $"EXP, {precision}: exponent reduction, polynomial, then power-of-two scaling.\nZ80 144Dh: CALL 0B40h ; floor after multiplication by log2(e)\nZ80: CALL 070Bh; CALL 0841h; CALL 0713h; CALL 0982h ; reduction\nZ80: LD HL,1479h; CALL 14A9h ; exp(-r) polynomial\n" + (name.StartsWith("double_") ? "RV32 DOUBLE refinement: split ln(2), 18 coefficients, final subnormal rounding.\nThe ROM uses eight coefficients; these are NOT the same constant table." : "RV32 SINGLE retains eight decoded ROM coefficients and binary32 rounding."),
            "sine" => $"SIN, {precision}: reduce to turns, fold quarter/half turns, odd polynomial.\nZ80 1547h: divide by ROM 2*pi; floor and subtract (algorithm summary)\nZ80: CALL 0710h ; table value MINUS accumulator, not the reverse\nZ80: LD HL,1593h; JP 149Ah ; five-coefficient odd polynomial\nRV32 branches preserve folding order; large-angle precision remains limited.",
            "arctangent" => $"ATN, {precision}: save sign; reciprocal reduction when |x|>=1.\nZ80 15BDh: sign/exponent tests and reciprocal (algorithm summary)\nZ80: LD HL,15E3h; CALL 149Ah ; nine-coefficient odd polynomial\nZ80 return through 0710h ; pi/2 minus result for reciprocal branch\nRV32 restores the original sign after evaluation and optional pi/2 subtraction.",
            "math" => $"Token dispatch and result calculation, {precision}.\nZ80 SQR 13E7h: CALL 09A4h; LD HL,1380h; CALL 09B1h; JR 13F5h\nZ80 COS 1541h: LD HL,158Bh; CALL 070Bh ; add pi/2, fall into SIN\nZ80 TAN 15A8h: calls SIN/COS; JP 08A0h ; divide sine by cosine\nRV32 uses a token switch, explicit error returns and native output pointers.",
            "floor_word" => "IEEE binary64 floor helper; native replacement for the ROM INT path.\nZ80 caller excerpt: CALL 0B40h ; INT/floor during EXP reduction\nZ80 0AFBh family shifts MBF bytes and corrects negative fractions (summary).\nNo identical Z80 implementation: RV32 masks IEEE fraction bits and handles large integers.",
            "fraction_single" or "fraction_double" => "IEEE mantissa/exponent extraction for LOG; positive input precondition.\nZ80 LOG 0810h: LD HL,4124h; LD A,(HL)\nZ80 0814h: LD BC,8035h; LD DE,04F3h\nZ80 081Ah: SUB B; PUSH AF; LD (HL),B ; normalize with B=80h\nNo identical Z80 helper: RV32 masks IEEE words and scales subnormals first.",
            "n32_math" => "Public checked math entry: type/domain/finite checks precede result publication.\nZ80 LOG excerpt: CALL 0955h; OR A; JP PE,1E4Ah ; argument rejection\nZ80 entries: LOG 0809h, EXP 1439h, SQR 13E7h, SIN 1547h, COS 1541h, TAN 15A8h, ATN 15BDh.\nNo identical Z80 dispatcher: native tagged values and status codes replace ROM workspace/errors.",
            "n32_math_power" => "Checked fractional power using EXP(y*LOG(|x|)); preserve sign for odd integral y.\nZ80 13E7h: CALL 09A4h ; save argument\nZ80 13EAh: LD HL,1380h; CALL 09B1h; JR 13F5h ; load one half and enter power\nZ80 algorithm call sites: LOG 0809h and EXP 1439h.\nNative zero/domain checks and wide parity avoid narrowing exponents to Z80 byte values.",
            _ => throw new InvalidDataException($"Missing Z80 ancestry for function {name}")
        };
    }

    private static string Explain(string op, string operands) => op switch
    {
        "call" or "tail" => $"{(op == "tail" ? "tail-transfer to" : "call")} {operands}; " +
            (operands.StartsWith("__") ? "soft-float/compiler ABI helper; IEEE bits travel in integer registers." : "arguments in a-registers; preserve any live caller-saved values."),
        "ret" => "return to ra; a0 (and a1 for binary64) carries the result.",
        "lw" or "sw" => "transfer a 32-bit word; sp-relative slots hold saved registers/temporaries, other bases address values/tables.",
        "lbu" or "lb" => "load a byte (unsigned/signed respectively); tagged type/token fields are byte-sized.",
        "sb" => "store the low byte to a type/status field.",
        "li" => "materialize an integer, mask, token, or IEEE word fragment; this is an assembler pseudo-instruction.",
        "lui" => "materialize upper 20 bits of an address or constant; paired lower bits complete the word.",
        "mv" => "copy an integer register or floating-point bit-pattern word without numeric conversion.",
        "addi" or "add" or "sub" or "neg" => "integer address/exponent/stack arithmetic; floating arithmetic is performed by helper calls.",
        "and" or "andi" or "or" or "ori" or "xor" or "xori" or "not" => "bitwise mask/combine/sign manipulation of native words; no Z80 flags are synthesized.",
        "sll" or "slli" or "srl" or "srli" or "sra" or "srai" => "shift a native word to align IEEE fields, build masks, or index tables; SRA/SRAI right shifts preserve sign.",
        "j" or "jr" => "unconditional control transfer to a basic block or switch target.",
        "beq" or "bne" or "beqz" or "bnez" or "blt" or "bge" or "bltu" or "bgeu" or "blez" or "bgez" or "bgtz" or "bltz" => "conditional branch on integer registers/helper comparison results; u-suffixed comparisons are unsigned.",
        "slt" or "slti" or "sltu" or "sltiu" or "seqz" or "snez" or "sgtz" or "sltz" => "produce a 0/1 predicate for native control flow.",
        "mul" or "mulhu" => "multiply integer words for indexing or software arithmetic, not a hardware floating multiply.",
        _ => throw new InvalidDataException($"Unannotated RV32 opcode {op}")
    };

    private static string[] FunctionNames(string text) => text.Replace("\r", "").Split('\n')
        .Select(line => Function.Match(line)).Where(match => match.Success).Select(match => match.Groups[1].Value).ToArray();

    /// <summary>Fail if annotation loses an instruction or any function's Z80 header.</summary>
    public static void Validate(string raw, string listing)
    {
        string[] Instructions(string text) => text.Replace("\r", "").Split('\n')
            .Select(line => line.Split('#')[0]).Where(line => Instruction.IsMatch(line))
            .Select(line => Regex.Replace(line.Trim(), @"\s+", " ")).ToArray();
        if (!Instructions(raw).SequenceEqual(Instructions(listing))) throw new InvalidDataException("Instruction stream changed");
        string[] names = FunctionNames(raw);
        if (names.Length == 0 || !names.SequenceEqual(FunctionNames(listing))) throw new InvalidDataException("Function declarations changed/missing");
        foreach (string name in names)
        {
            int header = listing.IndexOf($"# FUNCTION {name}\n", StringComparison.Ordinal);
            int declaration = listing.IndexOf($"\t.type\t{name},@function", StringComparison.Ordinal);
            if (header < 0 || declaration < header || !listing[header..declaration].Contains("# Z80"))
                throw new InvalidDataException($"Missing Z80 header for {name}");
        }
    }
}
