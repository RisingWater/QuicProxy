using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;
using Microsoft.Win32;

namespace QuicProxyUI
{
    public partial class Form1 : Form
    {
        [DllImport("kernel32.dll")]
        private static extern int GetPrivateProfileString(
                string section,
                string keyName,
                string defaultValue,
                StringBuilder returnValue,
                int size,
                string filepath);

        [DllImport("kernel32.dll")]
        private static extern int WritePrivateProfileString(
            string section,
            string key,
            string val,
            string filepath);

        private Process ServerProcess = null;
        private bool DisableSelecting = false;

        private void Log(String str)
        {
            logBox.Items.Add(str);
            logBox.SelectedIndex = logBox.Items.Count - 1;
        }


        private void LoadToReg()
        {
            RegistryKey hklm = Registry.CurrentUser;
            RegistryKey hkConfig = hklm.OpenSubKey(@"SOFTWARE\QuicProxy", true);
            if (hkConfig != null)
            {
                object project = hkConfig.GetValue("ServerPort");
                if (project != null)
                {
                    ServerPortInput.Text = project.ToString();
                }

                project = hkConfig.GetValue("ProxyAddress");
                if (project != null)
                {
                    ClientProxyAddress.Text = project.ToString();
                }

                project = hkConfig.GetValue("ProxyPort");
                if (project != null)
                {
                    ClientProxyPort.Value = Convert.ToDecimal(project.ToString());
                }

                project = hkConfig.GetValue("RemoteAddress");
                if (project != null)
                {
                    RemoteAddressInput.Text = project.ToString();
                }

                project = hkConfig.GetValue("RemotePort");
                if (project != null)
                {
                    RemoteTCPPortInput.Value = Convert.ToDecimal(project.ToString());
                }

                project = hkConfig.GetValue("LocalPort");
                if (project != null)
                {
                    LocalPort.Value = Convert.ToDecimal(project.ToString());
                }
            }
        }

        private void SaveToReg()
        {
            RegistryKey hklm = Registry.CurrentUser;
            RegistryKey hkConfig = hklm.OpenSubKey(@"SOFTWARE\QuicProxy", true);
            if (hkConfig == null)
            {
                hkConfig = hklm.CreateSubKey(@"SOFTWARE\QuicProxy");
            }

            hkConfig.SetValue("ServerPort", ServerPortInput.Text);
            hkConfig.SetValue("ProxyAddress", ClientProxyAddress.Text);
            hkConfig.SetValue("RemoteAddress", RemoteAddressInput.Text);
            hkConfig.SetValue("RemotePort", RemoteTCPPortInput.Value.ToString());
            hkConfig.SetValue("LocalPort", LocalPort.Value.ToString());

            hkConfig.Close();
            hklm.Close();
        }

        public Form1()
        {
            Control.CheckForIllegalCrossThreadCalls = false;
            InitializeComponent();
            LoadToReg();
        }

        private void UpdateUI(bool startProcess)
        {
            if (startProcess)
            {
                DisableSelecting = true;

                ServerStartButton.Enabled = false;
                ServerStopButton.Enabled = true;

                ServerPortInput.Enabled = false;

                ClientStartButton.Enabled = false;
                ClientStopButton.Enabled = true;

                ClientProxyAddress.Enabled = false;
                ClientProxyPort.Enabled = false;
                RemoteAddressInput.Enabled = false;
                RemoteTCPPortInput.Enabled = false;
                LocalPort.Enabled = false;
            }
            else
            {
                DisableSelecting = false;

                ServerStartButton.Enabled = true;
                ServerStopButton.Enabled = false;

                ServerPortInput.Enabled = true;

                ClientStartButton.Enabled = true;
                ClientStopButton.Enabled = false;

                ClientProxyAddress.Enabled = true;
                ClientProxyPort.Enabled = true;
                RemoteAddressInput.Enabled = true;
                RemoteTCPPortInput.Enabled = true;
                LocalPort.Enabled = true;
            }
        }

        private void ProxyProcess(string configfile)
        {
            ServerProcess = new Process();
            ServerProcess.StartInfo.FileName = Application.StartupPath + "\\QuicProxy.exe";
            ServerProcess.StartInfo.Arguments = configfile;
            ServerProcess.StartInfo.UseShellExecute = false;
            ServerProcess.StartInfo.RedirectStandardOutput = true;
            ServerProcess.StartInfo.CreateNoWindow = true;

            ServerProcess.Start();

            while (!ServerProcess.StandardOutput.EndOfStream)
            {
                string line = ServerProcess.StandardOutput.ReadLine();
                Log(line);
            }

            UpdateUI(false);
        }

        private void ServerStartButton_Click(object sender, EventArgs e)
        {
            SaveToReg();

            string tmpPath = System.Environment.GetEnvironmentVariable("TMP");
            tmpPath += "\\" + "QuicProxyServer.ini";

            WritePrivateProfileString("config", "role", "proxyserver", tmpPath);
            WritePrivateProfileString("config", "proxy_server", "127.0.0.1:" + ServerPortInput.Value, tmpPath);

            UpdateUI(true);

            Thread ProcessThread = new Thread(
                delegate()
                {
                    ProxyProcess(tmpPath);
                }
            );

            ProcessThread.IsBackground = true;
            ProcessThread.Start();

        }

        private void ServerStopButton_Click(object sender, EventArgs e)
        {
            if (ServerProcess != null)
            {
                ServerProcess.Kill();
            }
        }

        private void ControlTab_Selecting(object sender, TabControlCancelEventArgs e)
        {
            if (DisableSelecting)
            {
                e.Cancel = true;
            }
        }

        private void ClientStartButton_Click(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(ClientProxyAddress.Text))
            {
                Log("请填写代理服务器地址");
                return;
            }

            if (string.IsNullOrEmpty(RemoteAddressInput.Text))
            {
                Log("请填写远程tcp服务地址");
                return;
            }

            SaveToReg();

            string tmpPath = System.Environment.GetEnvironmentVariable("TMP");
            tmpPath += "\\" + "QuicProxyClient.ini";

            WritePrivateProfileString("config", "role", "proxyclient", tmpPath);
            WritePrivateProfileString("config", "proxy_server", 
                ClientProxyAddress.Text + ":" + ClientProxyPort.Value, tmpPath);
            WritePrivateProfileString("config", "service", "test", tmpPath);

            WritePrivateProfileString("test", "ip", RemoteAddressInput.Text, tmpPath);
            WritePrivateProfileString("test", "remote_port", RemoteTCPPortInput.Value.ToString(), tmpPath);
            WritePrivateProfileString("test", "local_port", LocalPort.Value.ToString(), tmpPath);

            UpdateUI(true);

            Thread ProcessThread = new Thread(
                delegate()
                {
                    ProxyProcess(tmpPath);
                }
            );

            ProcessThread.IsBackground = true;
            ProcessThread.Start();
        }

        private void ClientStopButton_Click(object sender, EventArgs e)
        {
            if (ServerProcess != null)
            {
                ServerProcess.Kill();
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (ServerProcess != null)
            {
                ServerProcess.Kill();
            }
        }
    }
}
