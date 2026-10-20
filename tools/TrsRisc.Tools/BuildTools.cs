using System.Security.Cryptography;

namespace TrsRisc.Tools;

/// <summary>Reproducible ROM retrieval and synthetic Z80 instruction fixtures.</summary>
public static class BuildTools
{
    /// <summary>Pinned upstream ROM revision; original rights remain with its owners.</summary>
    public const string RomUrl = "https://raw.githubusercontent.com/lkesteloot/trs80/b8ea902112de747764adcd4483eb48044f785bb7/packages/trs80-emulator/roms/model1-level2.rom";
    /// <summary>SHA-256 of the complete 12 KB Level II image used by this port.</summary>
    public const string RomDigest = "3cd3824d2ac2743750d2179951ed812794302b885ccb4f98aa2579b352c4b00e";

    /// <summary>Download BASIC as ROM bytes; verify length/digest before writing.</summary>
    /// <remarks>
    /// These bytes supply BOTH the instruction translator and rom_image[] data.
    /// For example Z80 keyword-table bytes C5 4E 44 mean the token spelling END.
    /// Runtime code copies rom_image[] into emulated addresses 0000h..2FFFh.
    /// </remarks>
    public static void FetchRom(string path)
    {
        using var client = new HttpClient { Timeout = TimeSpan.FromSeconds(60) };
        byte[] bytes = client.GetByteArrayAsync(RomUrl).GetAwaiter().GetResult();
        if (bytes.Length != 12288 || Convert.ToHexString(SHA256.HashData(bytes)).ToLowerInvariant() != RomDigest)
            throw new InvalidDataException("ROM integrity check failed; output not written");
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        File.WriteAllBytes(path, bytes);
        Console.WriteLine($"Fetched {bytes.Length} verified bytes: {RomDigest}");
    }

    /// <summary>Generate seven opcode families in eight-byte fixture slots.</summary>
    /// <remarks>
    /// Families: base / CB / ED / DD / FD / DD CB d / FD CB d. Eight-byte slots
    /// allow immediate nn operands and repeated/ignored prefixes without
    /// overlapping the next tested instruction. No historical ROM is used.
    /// </remarks>
    public static void Fixtures(string directory)
    {
        var image = new List<byte>();
        for (int family = 0; family < 7; family++)
        for (int op = 0; op < 256; op++)
        {
            byte[] instruction = family switch
            {
                0 => [(byte)op, 0x80, 0x60], 1 => [0xcb, (byte)op],
                2 => [0xed, (byte)op, 0x80, 0x60],
                3 or 4 => [(byte)(family == 3 ? 0xdd : 0xfd), (byte)op, 0x80, 0x60],
                _ => [(byte)(family == 5 ? 0xdd : 0xfd), 0xcb, 0x80, (byte)op]
            };
            image.AddRange(instruction);
            image.AddRange(new byte[8 - instruction.Length]);
        }
        Directory.CreateDirectory(directory);
        File.WriteAllText(Path.Combine(directory, "translated.inc"), new Translator(image.ToArray()).Generate());
    }
}
