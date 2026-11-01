using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Threading;
using System.Windows.Forms;

internal static class UsbLauncher
{
    [STAThread]
    private static void Main()
    {
        string root = AppDomain.CurrentDomain.BaseDirectory;
        string profile = Path.Combine(root, @"data\usb-profile.json");
        string node = Path.Combine(root, @"runtime\node.exe");
        string server = Path.Combine(root, @"local-server\server.js");
        string app = Path.Combine(root, @"app\Ipponboard-Meschede.exe");
        if (!File.Exists(profile) || !File.Exists(node) || !File.Exists(server) || !File.Exists(app))
        {
            MessageBox.Show("USB-Paket unvollständig. Bitte eine vollständige Export-ZIP verwenden.", "Ipponboard", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }
        Process service = null;
        try
        {
            var info = new ProcessStartInfo(node, "\"" + server + "\"");
            info.WorkingDirectory = root;
            info.UseShellExecute = false;
            info.CreateNoWindow = true;
            info.EnvironmentVariables["IPPONBOARD_USB_OFFLINE"] = "1";
            info.EnvironmentVariables["IPPONBOARD_DATA_DIR"] = Path.Combine(root, "data");
            info.EnvironmentVariables["HOST"] = "127.0.0.1";
            info.EnvironmentVariables["PORT"] = "43181";
            service = Process.Start(info);
            bool ready = false;
            for (int i = 0; i < 60; i++)
            {
                if (service.HasExited) break;
                try
                {
                    var request = (HttpWebRequest)WebRequest.Create("http://127.0.0.1:43181/api/masterdata");
                    request.Timeout = 500;
                    request.Proxy = null;
                    using (var response = (HttpWebResponse)request.GetResponse())
                        if (response.StatusCode == HttpStatusCode.OK) { ready = true; break; }
                }
                catch (WebException) { }
                Thread.Sleep(200);
            }
            if (!ready)
            {
                MessageBox.Show("Der lokale Verwaltungsserver konnte nicht gestartet werden.", "Ipponboard", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            var desktop = new ProcessStartInfo(app);
            desktop.WorkingDirectory = Path.Combine(root, "app");
            using (var process = Process.Start(desktop)) process.WaitForExit();
        }
        catch (Exception e)
        {
            MessageBox.Show(e.Message, "Ipponboard", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
        finally
        {
            if (service != null)
            {
                try { if (!service.HasExited) service.Kill(); } catch (InvalidOperationException) { }
                service.Dispose();
            }
        }
    }
}
