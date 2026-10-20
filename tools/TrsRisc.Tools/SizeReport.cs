using System.Globalization;
using System.Text;

namespace TrsRisc.Tools;

/// <summary>Measure the actual ELF64 sections and symbols, not source line counts.</summary>
/// <remarks>
/// Z80 ROM bytes include code AND data; translated_step contains native code for
/// every byte-offset entry point, including overlaps/data offsets. The comparison
/// is therefore a measured expansion of this AOT design, not an ISA density benchmark.
/// </remarks>
public static class SizeReport
{
    private sealed record Section(uint NameOffset, uint Type, ulong Flags, ulong Offset, ulong Size, uint Link, ulong EntrySize);

    /// <summary>Write a Sphinx reStructuredText report directly from an unstripped RV64 ELF.</summary>
    public static void Write(string romPath, string elfPath, string output)
    {
        long original = new FileInfo(romPath).Length;
        using var file = File.OpenRead(elfPath); using var reader = new BinaryReader(file);
        byte[] header = reader.ReadBytes(64);
        if (header.Length != 64 || header[0] != 0x7f || header[1] != 'E' || header[2] != 'L' || header[3] != 'F' || header[4] != 2 || header[5] != 1)
            throw new InvalidDataException("expected little-endian ELF64");
        if (BitConverter.ToUInt16(header, 18) != 243) throw new InvalidDataException("ELF is not RISC-V");
        ulong offset = BitConverter.ToUInt64(header, 40); int stride = BitConverter.ToUInt16(header, 58), count = BitConverter.ToUInt16(header, 60);
        if (stride < 64 || count == 0 || offset > (ulong)file.Length || (ulong)count * (ulong)stride > (ulong)file.Length - offset)
            throw new InvalidDataException("invalid/extended ELF section table");
        var sections = new List<Section>();
        for (int index = 0; index < count; index++)
        {
            file.Position = checked((long)(offset + (ulong)(index * stride)));
            uint name = reader.ReadUInt32(), type = reader.ReadUInt32(); ulong flags = reader.ReadUInt64();
            reader.ReadUInt64(); ulong dataOffset = reader.ReadUInt64(), size = reader.ReadUInt64(); uint link = reader.ReadUInt32();
            reader.ReadUInt32(); reader.ReadUInt64(); ulong entrySize = reader.ReadUInt64();
            sections.Add(new Section(name, type, flags, dataOffset, size, link, entrySize));
        }
        byte[] ReadSection(Section section)
        {
            if (section.Size > int.MaxValue || section.Offset + section.Size > (ulong)file.Length) throw new InvalidDataException("invalid ELF section extent");
            file.Position = checked((long)section.Offset); return reader.ReadBytes(checked((int)section.Size));
        }
        string StringAt(byte[] table, uint index)
        {
            if (index >= table.Length) throw new InvalidDataException("invalid ELF string offset");
            int end = Array.IndexOf(table, (byte)0, (int)index);
            if (end < 0) throw new InvalidDataException("unterminated ELF string");
            return Encoding.UTF8.GetString(table, (int)index, end - (int)index);
        }
        int namesIndex = BitConverter.ToUInt16(header, 62);
        if (namesIndex >= sections.Count) throw new InvalidDataException("invalid ELF name table");
        byte[] names = ReadSection(sections[namesIndex]);
        ulong text = 0, readonlyData = 0, writableData = 0, bss = 0;
        foreach (Section section in sections)
        {
            string name = StringAt(names, section.NameOffset);
            if (name == ".text") text = section.Size;
            if (name == ".rodata") readonlyData = section.Size;
            if (name == ".data") writableData = section.Size;
            if (name == ".bss") bss = section.Size;
        }
        ulong translated = 0, embedded = 0;
        foreach (Section section in sections.Where(item => item.Type == 2)) // SHT_SYMTAB
        {
            if (section.EntrySize < 24 || section.Link >= sections.Count) throw new InvalidDataException("invalid symbol table");
            byte[] symbols = ReadSection(section), strings = ReadSection(sections[(int)section.Link]);
            for (ulong position = 0; position + 24 <= (ulong)symbols.Length; position += section.EntrySize)
            {
                int symbol = checked((int)position); string name = StringAt(strings, BitConverter.ToUInt32(symbols, symbol));
                ulong size = BitConverter.ToUInt64(symbols, symbol + 16);
                if (name == "translated_step") translated = size;
                if (name == "rom_image") embedded = size;
            }
        }
        if (translated == 0 || embedded != (ulong)original) throw new InvalidDataException("keep ELF symbols and do not inline/remove translated_step for this measurement");
        string Ratio(ulong value) => ((double)value / original).ToString("F2", CultureInfo.InvariantCulture) + "x";
        var result = new StringBuilder("Measured RV64 build\n===================\n\n");
        result.Append("Generated from the unstripped ELF by the C# size-report command.\n\n.. list-table:: Z80 ROM versus RV64 artifact\n   :header-rows: 1\n\n   * - Measurement\n     - Bytes\n     - / Z80 ROM\n");
        void Row(string name, ulong amount) => result.Append($"   * - {name}\n     - {amount:N0}\n     - {Ratio(amount)}\n");
        Row("Original Z80 ROM (code + tables + strings)", (ulong)original);
        Row("Native translated_step symbol", translated);
        Row("Entire .text (translated code + runtime + libc)", text);
        Row("Embedded original rom_image symbol (inside .rodata)", embedded);
        Row("Entire .rodata (includes ROM and dispatch data)", readonlyData);
        Row("Initialized .data", writableData); Row("Zero-filled .bss (memory, not file payload)", bss);
        Row("Unstripped ELF file (includes symbols/debug metadata)", (ulong)file.Length);
        result.Append("\nThe embedded ROM is already inside .rodata; these rows overlap and must not\nbe added together. .bss occupies RAM but contributes no payload bytes to the\nfile. The original ROM's 12 KB includes data as well as instructions; this\nis not a like-for-like code-density experiment or a speed benchmark.\n");
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(output))!); File.WriteAllText(output, result.ToString());
        Console.WriteLine($"Z80 ROM={original}; RV64 translated_step={translated}; ratio={Ratio(translated)}; ELF={file.Length}");
    }
}
