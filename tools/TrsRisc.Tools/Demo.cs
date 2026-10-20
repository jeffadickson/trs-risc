using System.Diagnostics;

namespace TrsRisc.Tools;

/// <summary>Feed the historical demo to a native-translated BASIC executable.</summary>
/// <remarks>
/// This is a keyboard-input driver, not an implementation of BASIC. The ROM
/// receives the numbered text, tokenizes it into its own linked-list memory
/// format, then interprets that data when RUN is entered. Multiplication and
/// loop control come from the ROM's native-translated Z80 routines.
/// </remarks>
public static class Demo
{
    /// <summary>The actual BASIC source; no expected output is injected.</summary>
    public const string Source = "10 PRINT \"HELLO FROM RISC-V\"\n"
        + "20 FOR I=1 TO 5\n30 PRINT I;I*I\n40 NEXT I\n50 END\n";

    /// <summary>Run through a host binary, or a QEMU command followed by RV64 ELF.</summary>
    /// <remarks>
    /// The leading two Enter keys complete ROM startup. Z80 CALL 002Bh reads
    /// input; CALL 0033h prints the ROM's actual output through the C adapter.
    /// Example: tools demo qemu-riscv64 build/trs80-riscv64.
    /// </remarks>
    public static int Run(string[] command)
    {
        if (command.Length == 0) throw new ArgumentException("demo needs an executable (optionally preceded by QEMU)");
        var start = new ProcessStartInfo(command[0]) { UseShellExecute = false };
        foreach (string argument in command.Skip(1)) start.ArgumentList.Add(argument);
        start.ArgumentList.Add("--input");
        start.ArgumentList.Add("\n\n" + Source + "RUN\n");
        using Process child = Process.Start(start) ?? throw new IOException("could not launch BASIC");
        child.WaitForExit();
        return child.ExitCode;
    }
}
