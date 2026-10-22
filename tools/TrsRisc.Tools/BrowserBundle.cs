using System.Text.Json;
namespace TrsRisc.Tools;
/// <summary>Bundle the real RV32 CPU and guest ELF images into one offline HTML.
/// No server, .NET or network is needed to open the resulting browser artifact.</summary>
public static class BrowserBundle
{
    public static void Generate(string output)
    {
        const string root = "web/";
        var images = new Dictionary<string,string>();
        foreach (string name in new[] { "rv32-board.wasm", "native32-rom.elf", "native32-rom-compact.elf" })
            images[name] = Convert.ToBase64String(File.ReadAllBytes(root + name));
        string script = "<script type=\"application/json\" id=\"embedded-images\">" + JsonSerializer.Serialize(images) + "</script>\n<script type=\"module\">" + File.ReadAllText(root + "rv32-demo.mjs") + "</script>";
        string html = File.ReadAllText(root + "rv32-demo.html").Replace("<script type=\"module\" src=\"rv32-demo.mjs\"></script>", script);
        html = html.Replace("href=\"math-comparison.html\"", "href=\"native32-math-comparison.html\"");
        File.WriteAllText(output, html);
        File.WriteAllText(Path.Combine(Path.GetDirectoryName(Path.GetFullPath(output))!, "native32-math-comparison.html"), File.ReadAllText(root + "math-comparison.html").Replace("href=\"rv32-demo.html\"", "href=\"native32-simulator.html\""));Console.WriteLine($"Offline simulator: {output}");
    }
}
