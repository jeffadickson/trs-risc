namespace TrsRisc.Tools;

/// <summary>Single entry point for the C# build, demo and verification tools.</summary>
public static class Program
{
    /// <summary>Run a subcommand without invoking a shell or importing Python tools.</summary>
    public static int Main(string[] args)
    {
        try
        {
            if (args.Length == 0) throw new ArgumentException("commands: translate, fetch-rom, fixtures, demo, smoke, basic-smoke, size-report, serve");
            switch (args[0])
            {
                case "translate" when args.Length == 3: Translator.TranslateFile(args[1], args[2]); break;
                case "fetch-rom" when args.Length == 2: BuildTools.FetchRom(args[1]); break;
                case "fixtures" when args.Length == 2: BuildTools.Fixtures(args[1]); break;
                case "serve" when args.Length == 3: Server.Run(args[1], int.Parse(args[2])); break;
                case "demo": return Demo.Run(args[1..]);
                case "smoke": Verification.Smoke(args[1..]); break;
                case "basic-smoke": Verification.BasicSmoke(args[1..]); break;
                case "size-report" when args.Length == 4: SizeReport.Write(args[1], args[2], args[3]); break;
                default: throw new ArgumentException("unknown command or wrong argument count");
            }
            return 0;
        }
        catch (Exception error) { Console.Error.WriteLine("trs-risc tools: " + error.Message); return 1; }
    }
}
