using System.Diagnostics;

namespace TrsRisc.Tools;

/// <summary>Behavioral verification for native BASIC and the C memory-image tool.</summary>
public static class Verification
{
    private sealed record Result(int Code, string Output, string Error);

    // Host-side process adapter, not a ROM routine. ArgumentList preserves
    // quotes/newlines in BASIC source without passing any text through a shell.
    private static Result Run(string[] command, params string[] arguments)
    {
        if (command.Length == 0) throw new ArgumentException("expected executable command");
        var start = new ProcessStartInfo(command[0]) { UseShellExecute = false, RedirectStandardOutput = true, RedirectStandardError = true };
        foreach (string item in command.Skip(1).Concat(arguments)) start.ArgumentList.Add(item);
        using Process process = Process.Start(start) ?? throw new IOException("cannot start test process");
        Task<string> stdout = process.StandardOutput.ReadToEndAsync(), stderr = process.StandardError.ReadToEndAsync();
        if (!process.WaitForExit(30000)) { process.Kill(true); throw new TimeoutException("test exceeded 30 seconds"); }
        return new Result(process.ExitCode, stdout.GetAwaiter().GetResult().Replace("\r\n", "\n"), stderr.GetAwaiter().GetResult());
    }
    private static void Require(bool condition, string message) { if (!condition) throw new InvalidDataException(message); }

    /// <summary>Test boot and actual ROM execution on host or QEMU/RV64.</summary>
    /// <remarks>Assertions inspect ROM-generated output; they never inject expected results.</remarks>
    public static void Smoke(string[] command)
    {
        (string Name, string Source, string[] Expected)[] cases =
        [
            ("arithmetic", "PRINT 2+2\n", [" 4 "]),
            ("stored program", "10 PRINT \"HELLO FROM RISC-V\"\n20 END\nRUN\n", ["\nHELLO FROM RISC-V\nREADY"]),
            ("FOR/NEXT", "10 FOR I=1 TO 5\n20 PRINT I*I\n30 NEXT I\nRUN\n", ["\n 1 \n 4 \n 9 \n 16 \n 25 \nREADY"]),
            ("floating point", "PRINT SQR(81)\nPRINT SIN(0)\nPRINT 1/8\n", ["\n 9 \n", "\n 0 \n", "\n .125 \n"]),
            ("strings", "A$=\"TRS-80\"\nPRINT LEFT$(A$,3)\n", ["\nTRS\n"])
        ];
        foreach (var item in cases)
        {
            Result result = Run(command, "--input", "\n\n" + item.Source);
            Require(result.Code == 0 && result.Output.Contains("RADIO SHACK LEVEL II BASIC\nREADY") && !result.Output.Contains("Error"), item.Name + ": " + result.Output + result.Error);
            foreach (string expected in item.Expected) Require(result.Output.Contains(expected), item.Name + ": missing " + expected + " in " + result.Output);
            Console.WriteLine("PASS: " + item.Name);
        }
        Result raw = Run(command, "--raw", "--steps", "600000", "--screen");
        Require(raw.Code == 3 && raw.Error.Contains("MEMORY SIZE?"), "raw video boot failed: " + raw.Error);
        Console.WriteLine("PASS: raw ROM video-memory boot (expected step limit)");
        Result loop = Run(command, "--input", "\n\n10 GOTO 10\nRUN\n", "--steps", "700000");
        Require(loop.Code == 3, "infinite BASIC loop did not reach budget");
        Console.WriteLine("PASS: infinite BASIC program stops at instruction budget");
    }

    /// <summary>Check authentic program bytes, source round trip, syntax aggregation and corruption rejection.</summary>
    public static void BasicSmoke(string[] command)
    {
        string directory = Path.Combine(Path.GetTempPath(), "trs-risc-tests-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(directory);
        try
        {
            string source = Path.Combine(directory, "source.bas"), image = Path.Combine(directory, "image.basram"), listing = Path.Combine(directory, "listed.bas"), second = Path.Combine(directory, "second.basram");
            File.WriteAllText(source, Demo.Source);
            Require(Run(command, "tokenize", source, image).Code == 0, "tokenize failed");
            byte[] bytes = File.ReadAllBytes(image);
            Require(bytes.Length == 68 && bytes[4] == 0xb2, "unexpected ROM program layout or PRINT token");
            Require(Run(command, "detokenize", image, listing).Code == 0, "detokenize failed");
            Require(File.ReadAllText(listing) == Demo.Source, "LIST source differs");
            Require(Run(command, "tokenize", listing, second).Code == 0 && bytes.SequenceEqual(File.ReadAllBytes(second)), "round-trip image differs");
            Result execution = Run(command, "run", image);
            Require(execution.Code == 0 && execution.Output.Contains("HELLO FROM RISC-V\n 1  1 \n 2  4 \n 3  9 \n 4  16 \n 5  25"), "loaded image did not run");
            Console.WriteLine("PASS: 68-byte authentic ROM image; LIST/re-tokenize byte-for-byte; load/RUN");
            File.WriteAllText(source, "10 PRINT (2+\n20 FOR I=1 5\n30 IF 1 PRINT \"BAD\"\n40 A=2*\n");
            File.Delete(second);
            Result invalid = Run(command, "tokenize", source, second);
            Require(invalid.Code != 0 && !File.Exists(second), "invalid source wrote an image");
            foreach (int line in new[] { 10, 20, 30, 40 }) Require(invalid.Error.Contains("BASIC line " + line + ","), "missing error for line " + line);
            Console.WriteLine("PASS: four errors across all source lines; no output image");
            bytes[0] = 0; bytes[1] = 0; File.WriteAllBytes(image, bytes);
            Require(Run(command, "list", image).Code != 0, "corrupt linked list accepted");
            Console.WriteLine("PASS: corrupt image rejected before ROM execution");
            File.WriteAllText(source, "10 REM PRINT GOTO inside comment\n20 DATA \"A:B\",PRINT\n30 IF 0 THEN PRINT \"YES\" ELSE PRINT \"NO\"\n40 END\n");
            Require(Run(command, "tokenize", source, image).Code == 0, "REM/DATA/IF tokenization failed");
            Require(Run(command, "detokenize", image, listing).Code == 0, "REM/DATA/IF listing failed");
            Require(Run(command, "tokenize", listing, second).Code == 0 && File.ReadAllBytes(image).SequenceEqual(File.ReadAllBytes(second)), "literal/comment round trip failed");
            Console.WriteLine("PASS: REM, DATA literals, quoted colon and IF/ELSE round trip");
        }
        finally { Directory.Delete(directory, true); }
    }
}
