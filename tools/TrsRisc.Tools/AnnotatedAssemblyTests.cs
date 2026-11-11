namespace TrsRisc.Tools;

/// <summary>Dependency-free structural, negative, seeded property and mutation checks for listings.</summary>
public static class AnnotatedAssemblyTests
{
    /// <summary>Check the real compiler output and minimized synthetic inputs; seed is printed for replay.</summary>
    public static void Run(string rawPath)
    {
        const int seed = 20261024;
        int checks = 0;
        void Check(bool condition) { checks++; if (!condition) throw new InvalidOperationException($"Listing check {checks} failed, seed {seed}"); }
        void Reject(Action action)
        {
            bool rejected = false;
            try { action(); } catch (InvalidDataException) { rejected = true; }
            Check(rejected);
        }
        const string prefix = "\t.text\n\t.type\tsingle_polynomial,@function\nsingle_polynomial:\n";
        string[] Source(string _) => ["return x;", "return x*x;"];
        string minimal = prefix + "\tli\ta0, 1\n\tret\n";
        string rendered = AnnotatedAssembly.Render(minimal, Source);
        AnnotatedAssembly.Validate(minimal, rendered); Check(rendered.Contains("# Z80 14A9h"));
        Reject(() => AnnotatedAssembly.Render(prefix.Replace("single_polynomial", "unknown_function"), Source));
        Reject(() => AnnotatedAssembly.Render(prefix + "\tinvalid_opcode\ta0\n", Source));
        Reject(() => AnnotatedAssembly.Validate("", ""));
        Reject(() => AnnotatedAssembly.Validate(minimal, rendered.Replace("# FUNCTION", "# REMOVED")));
        Reject(() => AnnotatedAssembly.Validate(minimal, rendered.Replace("# Z80", "# REMOVED")));
        Reject(() => AnnotatedAssembly.Validate(minimal, rendered.Replace("li\ta0, 1", "li\ta0, 2")));
        Reject(() => AnnotatedAssembly.Validate(minimal, rendered.Replace("\t.type\tsingle_polynomial,@function", "")));
        Reject(() => AnnotatedAssembly.Render(prefix + "\t.file\t1 \"root\" \"../outside.c\"\n", Source));
        Reject(() => AnnotatedAssembly.Render(prefix + "\t.file\t1 \"root\" \"src/native32/math.c\"\n\t.loc 1 3\n", Source));
        string located = prefix + "\t.file\t1 \"root\" \"src/native32/math.c\"\n\t.loc 1 1\n\t.loc 1 1\n\t.loc 1 0\n\tret\n\t.section .debug_info,\"\",@progbits\n\t.byte 0\n";
        string withSource = AnnotatedAssembly.Render(located, Source);
        Check(withSource.Contains("# C src/native32/math.c:1: return x;"));
        Check(!withSource.Contains(".debug_info"));

        // Generate valid small assembly streams, then deliberately corrupt one instruction.
        // Every counterexample is the current immediate/iteration, so failures are reproducible.
        var random = new Random(seed);
        for (int iteration = 0; iteration < 256; iteration++)
        {
            int immediate = random.Next(-2048, 2047);
            string source = prefix + $"\tli\ta0, {immediate}\n\taddi\ta0, a0, 1\n\tret\n";
            if ((iteration & 1) == 0) source = source.Replace("\n", "\r\n");
            string output = AnnotatedAssembly.Render(source, Source);
            AnnotatedAssembly.Validate(source, output); Check(output.Contains("# RV32:"));
            Reject(() => AnnotatedAssembly.Validate(source, output.Replace("addi\ta0, a0, 1", "addi\ta0, a0, 2")));
        }
        string raw = File.ReadAllText(rawPath);
        string listing = AnnotatedAssembly.Render(raw, File.ReadAllLines);
        AnnotatedAssembly.Validate(raw, listing); Check(listing.Contains("# FUNCTION n32_math_power\n"));
        string temporary = Path.Combine(Path.GetTempPath(), "trs-risc-listing-" + Guid.NewGuid().ToString("N") + ".s");
        try
        {
            AnnotatedAssembly.Generate(rawPath, temporary);
            Check(File.ReadAllText(temporary) == listing);
        }
        finally { File.Delete(temporary); }
        Console.WriteLine($"PASS {checks} listing checks; randomized seed {seed}; 256 instruction mutations and header/ancestry/declaration mutants rejected.");
    }
}
