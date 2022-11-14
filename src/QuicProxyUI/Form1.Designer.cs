namespace QuicProxyUI
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.ControlTab = new System.Windows.Forms.TabControl();
            this.ProxyServerTab = new System.Windows.Forms.TabPage();
            this.ServerStopButton = new System.Windows.Forms.Button();
            this.ServerStartButton = new System.Windows.Forms.Button();
            this.ServerPortInput = new System.Windows.Forms.NumericUpDown();
            this.ServerPortLabel = new System.Windows.Forms.Label();
            this.ProxyClientTab = new System.Windows.Forms.TabPage();
            this.ClientStopButton = new System.Windows.Forms.Button();
            this.LocalPort = new System.Windows.Forms.NumericUpDown();
            this.RemoteTCPPortInput = new System.Windows.Forms.NumericUpDown();
            this.RemoteAddressInput = new System.Windows.Forms.TextBox();
            this.RemoteIPAddress = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.ClientStartButton = new System.Windows.Forms.Button();
            this.ClientProxyAddress = new System.Windows.Forms.TextBox();
            this.ClientProxyPort = new System.Windows.Forms.NumericUpDown();
            this.label1 = new System.Windows.Forms.Label();
            this.ClientPortLabel = new System.Windows.Forms.Label();
            this.logBox = new System.Windows.Forms.ListBox();
            this.label4 = new System.Windows.Forms.Label();
            this.ControlTab.SuspendLayout();
            this.ProxyServerTab.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ServerPortInput)).BeginInit();
            this.ProxyClientTab.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.LocalPort)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.RemoteTCPPortInput)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.ClientProxyPort)).BeginInit();
            this.SuspendLayout();
            // 
            // ControlTab
            // 
            this.ControlTab.Controls.Add(this.ProxyServerTab);
            this.ControlTab.Controls.Add(this.ProxyClientTab);
            this.ControlTab.Location = new System.Drawing.Point(13, 13);
            this.ControlTab.Name = "ControlTab";
            this.ControlTab.SelectedIndex = 0;
            this.ControlTab.Size = new System.Drawing.Size(559, 178);
            this.ControlTab.TabIndex = 0;
            this.ControlTab.Selecting += new System.Windows.Forms.TabControlCancelEventHandler(this.ControlTab_Selecting);
            // 
            // ProxyServerTab
            // 
            this.ProxyServerTab.Controls.Add(this.ServerStopButton);
            this.ProxyServerTab.Controls.Add(this.ServerStartButton);
            this.ProxyServerTab.Controls.Add(this.ServerPortInput);
            this.ProxyServerTab.Controls.Add(this.ServerPortLabel);
            this.ProxyServerTab.Location = new System.Drawing.Point(4, 22);
            this.ProxyServerTab.Name = "ProxyServerTab";
            this.ProxyServerTab.Padding = new System.Windows.Forms.Padding(3);
            this.ProxyServerTab.Size = new System.Drawing.Size(551, 152);
            this.ProxyServerTab.TabIndex = 0;
            this.ProxyServerTab.Text = "代理服务器";
            this.ProxyServerTab.UseVisualStyleBackColor = true;
            // 
            // ServerStopButton
            // 
            this.ServerStopButton.Enabled = false;
            this.ServerStopButton.Location = new System.Drawing.Point(457, 37);
            this.ServerStopButton.Name = "ServerStopButton";
            this.ServerStopButton.Size = new System.Drawing.Size(75, 21);
            this.ServerStopButton.TabIndex = 3;
            this.ServerStopButton.Text = "停止代理";
            this.ServerStopButton.UseVisualStyleBackColor = true;
            this.ServerStopButton.Click += new System.EventHandler(this.ServerStopButton_Click);
            // 
            // ServerStartButton
            // 
            this.ServerStartButton.Location = new System.Drawing.Point(457, 10);
            this.ServerStartButton.Name = "ServerStartButton";
            this.ServerStartButton.Size = new System.Drawing.Size(75, 21);
            this.ServerStartButton.TabIndex = 2;
            this.ServerStartButton.Text = "启动代理";
            this.ServerStartButton.UseVisualStyleBackColor = true;
            this.ServerStartButton.Click += new System.EventHandler(this.ServerStartButton_Click);
            // 
            // ServerPortInput
            // 
            this.ServerPortInput.Font = new System.Drawing.Font("SimSun", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.ServerPortInput.Location = new System.Drawing.Point(110, 10);
            this.ServerPortInput.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.ServerPortInput.Name = "ServerPortInput";
            this.ServerPortInput.Size = new System.Drawing.Size(97, 21);
            this.ServerPortInput.TabIndex = 1;
            this.ServerPortInput.Value = new decimal(new int[] {
            7896,
            0,
            0,
            0});
            // 
            // ServerPortLabel
            // 
            this.ServerPortLabel.AutoSize = true;
            this.ServerPortLabel.Location = new System.Drawing.Point(15, 12);
            this.ServerPortLabel.Name = "ServerPortLabel";
            this.ServerPortLabel.Size = new System.Drawing.Size(53, 12);
            this.ServerPortLabel.TabIndex = 0;
            this.ServerPortLabel.Text = "监听端口";
            // 
            // ProxyClientTab
            // 
            this.ProxyClientTab.Controls.Add(this.ClientStopButton);
            this.ProxyClientTab.Controls.Add(this.LocalPort);
            this.ProxyClientTab.Controls.Add(this.RemoteTCPPortInput);
            this.ProxyClientTab.Controls.Add(this.RemoteAddressInput);
            this.ProxyClientTab.Controls.Add(this.RemoteIPAddress);
            this.ProxyClientTab.Controls.Add(this.label3);
            this.ProxyClientTab.Controls.Add(this.label2);
            this.ProxyClientTab.Controls.Add(this.ClientStartButton);
            this.ProxyClientTab.Controls.Add(this.ClientProxyAddress);
            this.ProxyClientTab.Controls.Add(this.ClientProxyPort);
            this.ProxyClientTab.Controls.Add(this.label1);
            this.ProxyClientTab.Controls.Add(this.ClientPortLabel);
            this.ProxyClientTab.Location = new System.Drawing.Point(4, 22);
            this.ProxyClientTab.Name = "ProxyClientTab";
            this.ProxyClientTab.Padding = new System.Windows.Forms.Padding(3);
            this.ProxyClientTab.Size = new System.Drawing.Size(551, 152);
            this.ProxyClientTab.TabIndex = 1;
            this.ProxyClientTab.Text = "代理客户端";
            this.ProxyClientTab.UseVisualStyleBackColor = true;
            // 
            // ClientStopButton
            // 
            this.ClientStopButton.Enabled = false;
            this.ClientStopButton.Location = new System.Drawing.Point(457, 37);
            this.ClientStopButton.Name = "ClientStopButton";
            this.ClientStopButton.Size = new System.Drawing.Size(75, 21);
            this.ClientStopButton.TabIndex = 12;
            this.ClientStopButton.Text = "断开代理";
            this.ClientStopButton.UseVisualStyleBackColor = true;
            this.ClientStopButton.Click += new System.EventHandler(this.ClientStopButton_Click);
            // 
            // LocalPort
            // 
            this.LocalPort.Font = new System.Drawing.Font("SimSun", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.LocalPort.Location = new System.Drawing.Point(110, 119);
            this.LocalPort.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.LocalPort.Name = "LocalPort";
            this.LocalPort.Size = new System.Drawing.Size(100, 21);
            this.LocalPort.TabIndex = 11;
            this.LocalPort.Value = new decimal(new int[] {
            8399,
            0,
            0,
            0});
            // 
            // RemoteTCPPortInput
            // 
            this.RemoteTCPPortInput.Font = new System.Drawing.Font("SimSun", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.RemoteTCPPortInput.Location = new System.Drawing.Point(110, 92);
            this.RemoteTCPPortInput.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.RemoteTCPPortInput.Name = "RemoteTCPPortInput";
            this.RemoteTCPPortInput.Size = new System.Drawing.Size(100, 21);
            this.RemoteTCPPortInput.TabIndex = 10;
            this.RemoteTCPPortInput.Value = new decimal(new int[] {
            8399,
            0,
            0,
            0});
            // 
            // RemoteAddressInput
            // 
            this.RemoteAddressInput.Location = new System.Drawing.Point(110, 65);
            this.RemoteAddressInput.Name = "RemoteAddressInput";
            this.RemoteAddressInput.Size = new System.Drawing.Size(100, 21);
            this.RemoteAddressInput.TabIndex = 9;
            this.RemoteAddressInput.Text = "127.0.0.1";
            // 
            // RemoteIPAddress
            // 
            this.RemoteIPAddress.AutoSize = true;
            this.RemoteIPAddress.Location = new System.Drawing.Point(15, 68);
            this.RemoteIPAddress.Name = "RemoteIPAddress";
            this.RemoteIPAddress.Size = new System.Drawing.Size(95, 12);
            this.RemoteIPAddress.TabIndex = 8;
            this.RemoteIPAddress.Text = "远程TCP服务地址";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(15, 94);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(95, 12);
            this.label3.TabIndex = 7;
            this.label3.Text = "远程TCP服务端口";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(15, 121);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(77, 12);
            this.label2.TabIndex = 6;
            this.label2.Text = "本地监听端口";
            // 
            // ClientStartButton
            // 
            this.ClientStartButton.Location = new System.Drawing.Point(457, 9);
            this.ClientStartButton.Name = "ClientStartButton";
            this.ClientStartButton.Size = new System.Drawing.Size(75, 21);
            this.ClientStartButton.TabIndex = 5;
            this.ClientStartButton.Text = "连接代理";
            this.ClientStartButton.UseVisualStyleBackColor = true;
            this.ClientStartButton.Click += new System.EventHandler(this.ClientStartButton_Click);
            // 
            // ClientProxyAddress
            // 
            this.ClientProxyAddress.Location = new System.Drawing.Point(110, 9);
            this.ClientProxyAddress.Name = "ClientProxyAddress";
            this.ClientProxyAddress.Size = new System.Drawing.Size(100, 21);
            this.ClientProxyAddress.TabIndex = 4;
            // 
            // ClientProxyPort
            // 
            this.ClientProxyPort.Font = new System.Drawing.Font("SimSun", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(134)));
            this.ClientProxyPort.Location = new System.Drawing.Point(110, 38);
            this.ClientProxyPort.Maximum = new decimal(new int[] {
            65535,
            0,
            0,
            0});
            this.ClientProxyPort.Name = "ClientProxyPort";
            this.ClientProxyPort.Size = new System.Drawing.Size(100, 21);
            this.ClientProxyPort.TabIndex = 3;
            this.ClientProxyPort.Value = new decimal(new int[] {
            7896,
            0,
            0,
            0});
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(15, 12);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(89, 12);
            this.label1.TabIndex = 2;
            this.label1.Text = "代理服务器地址";
            // 
            // ClientPortLabel
            // 
            this.ClientPortLabel.AutoSize = true;
            this.ClientPortLabel.Location = new System.Drawing.Point(15, 40);
            this.ClientPortLabel.Name = "ClientPortLabel";
            this.ClientPortLabel.Size = new System.Drawing.Size(89, 12);
            this.ClientPortLabel.TabIndex = 1;
            this.ClientPortLabel.Text = "代理服务器端口";
            // 
            // logBox
            // 
            this.logBox.FormattingEnabled = true;
            this.logBox.HorizontalScrollbar = true;
            this.logBox.ItemHeight = 12;
            this.logBox.Location = new System.Drawing.Point(13, 222);
            this.logBox.Name = "logBox";
            this.logBox.Size = new System.Drawing.Size(555, 280);
            this.logBox.TabIndex = 1;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(13, 204);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(53, 12);
            this.label4.TabIndex = 2;
            this.label4.Text = "运行日志";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(584, 511);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.logBox);
            this.Controls.Add(this.ControlTab);
            this.MaximumSize = new System.Drawing.Size(600, 550);
            this.MinimumSize = new System.Drawing.Size(600, 550);
            this.Name = "Form1";
            this.Text = "Quic代理";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.ControlTab.ResumeLayout(false);
            this.ProxyServerTab.ResumeLayout(false);
            this.ProxyServerTab.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.ServerPortInput)).EndInit();
            this.ProxyClientTab.ResumeLayout(false);
            this.ProxyClientTab.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.LocalPort)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.RemoteTCPPortInput)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.ClientProxyPort)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TabControl ControlTab;
        private System.Windows.Forms.TabPage ProxyServerTab;
        private System.Windows.Forms.Button ServerStartButton;
        private System.Windows.Forms.NumericUpDown ServerPortInput;
        private System.Windows.Forms.Label ServerPortLabel;
        private System.Windows.Forms.TabPage ProxyClientTab;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label ClientPortLabel;
        private System.Windows.Forms.NumericUpDown LocalPort;
        private System.Windows.Forms.NumericUpDown RemoteTCPPortInput;
        private System.Windows.Forms.TextBox RemoteAddressInput;
        private System.Windows.Forms.Label RemoteIPAddress;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button ClientStartButton;
        private System.Windows.Forms.TextBox ClientProxyAddress;
        private System.Windows.Forms.NumericUpDown ClientProxyPort;
        private System.Windows.Forms.Button ServerStopButton;
        private System.Windows.Forms.ListBox logBox;
        private System.Windows.Forms.Button ClientStopButton;
        private System.Windows.Forms.Label label4;
    }
}

