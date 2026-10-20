using System.Net;

namespace TrsRisc.Tools;

/// <summary>Local static HTML/WASM server; no Python server or external hosting.</summary>
public static class Server
{
    /// <summary>Serve a directory on localhost, with the WebAssembly MIME type.</summary>
    /// <remarks>This hosts static bytes only. All BASIC execution happens in the browser.</remarks>
    public static void Run(string directory, int port)
    {
        string root = Path.GetFullPath(directory) + Path.DirectorySeparatorChar;
        using var listener = new HttpListener();
        listener.Prefixes.Add($"http://localhost:{port}/");
        // Windows may prefer IPv6 localhost while WSL forwards only IPv4.
        // Accept the explicit loopback host too, without exposing a LAN prefix.
        listener.Prefixes.Add($"http://127.0.0.1:{port}/");
        listener.Start();
        Console.WriteLine($"trs-risc browser BASIC: http://localhost:{port}/ or http://127.0.0.1:{port}/ (Ctrl-C to stop)");
        while (true)
        {
            HttpListenerContext context = listener.GetContext();
            try
            {
                string requested = Uri.UnescapeDataString(context.Request.Url!.AbsolutePath).TrimStart('/');
                string path = Path.GetFullPath(Path.Combine(root, requested.Length == 0 ? "index.html" : requested));
                if (!path.StartsWith(root, StringComparison.Ordinal) || !File.Exists(path)) { context.Response.StatusCode = 404; continue; }
                context.Response.ContentType = Path.GetExtension(path) switch
                { ".html" => "text/html; charset=utf-8", ".mjs" or ".js" => "text/javascript; charset=utf-8", ".css" => "text/css; charset=utf-8", ".wasm" => "application/wasm", _ => "application/octet-stream" };
                byte[] bytes = File.ReadAllBytes(path); context.Response.ContentLength64 = bytes.Length;
                context.Response.OutputStream.Write(bytes);
            }
            finally { context.Response.Close(); }
        }
    }
}
