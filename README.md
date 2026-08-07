# 🛡️ Bastion-FlipperZero - Grade Your Badge Security Instantly

## 🚀 Getting Started

[![Download Bastion](https://img.shields.io/badge/Download-Bastion_FlipperZero-blue?style=for-the-badge&logo=github)](https://github.com/couchant-nightbird919/Bastion-FlipperZero)

Visit this link to download the application.

## 📖 What Is Bastion-FlipperZero?

Bastion-FlipperZero is a security grading tool for your Flipper Zero device. It reads 125 kHz badges (like EM4100, HID Prox, Indala, AWID, ioProx, Gallagher, or Nexwatch) and tells you how secure they are in plain English. No technical knowledge required—just hold a badge to the back of your Flipper Zero and get a full report.

## 🎯 Who Is This For?

- Security professionals checking badge systems
- Physical security testers (red teams)
- Facility managers evaluating access control
- Anyone curious about their badge's security level

## ✨ Key Features

- **Instant Grading**: Hold any compatible 125 kHz badge to your Flipper Zero and receive a security grade (A through F) with explanations.
- **Comprehensive Reports**: Each grade includes a detailed breakdown of why the badge earned that score, including vulnerabilities and recommendations.
- **Supports Multiple Badge Types**: Works with EM4100, HID Prox, Indala, AWID, ioProx, Gallagher, and Nexwatch badges.
- **Read-Only Operation**: Your Flipper Zero only reads badges—it never writes or clones them. Safe and non-intrusive.
- **Plain-English Output**: No technical jargon. Reports are written for everyday users.
- **Lightweight & Fast**: Runs directly on your Flipper Zero without additional software.

## 📋 System Requirements

- **Flipper Zero device** (any firmware version)
- **Compatible 125 kHz badge** (EM4100, HID Prox, Indala, AWID, ioProx, Gallagher, Nexwatch)
- **No computer needed** for operation—everything runs on the device

## 📦 How to Download and Install

1. **Visit the download page**: [https://github.com/couchant-nightbird919/Bastion-FlipperZero](https://github.com/couchant-nightbird919/Bastion-FlipperZero)
2. **Download the application**: Visit this link to download the application.
3. **Transfer to your Flipper Zero**: Copy the downloaded file to the `apps` folder on your Flipper Zero's SD card.
4. **Run Bastion**: On your Flipper Zero, navigate to Apps → Bastion and select it.

## 🎮 How to Use Bastion-FlipperZero

1. **Launch Bastion** on your Flipper Zero.
2. **Hold a badge** to the back of the device (where the RFID antenna is located).
3. **Wait 2-3 seconds** while Bastion reads the badge data.
4. **View your report**: The screen will display:
   - Security grade (A, B, C, D, or F)
   - Badge type detected
   - Explanation of the grade
   - Recommendations for improvement
5. **Press the Back button** to return to the main menu and test another badge.

## 📊 Understanding Security Grades

| Grade | Meaning | Example Report |
|-------|---------|----------------|
| **A** | Excellent security | "This EM4100 badge uses strong encryption and is resistant to cloning." |
| **B** | Good security | "This HID Prox badge has moderate security features but could be improved." |
| **C** | Fair security | "This Indala badge has basic protection but is vulnerable to replay attacks." |
| **D** | Poor security | "This AWID badge is easily cloned and should be upgraded." |
| **F** | Critical risk | "This ioProx badge has no security measures. Replace immediately." |

## 🛠️ Supported Badge Types

Bastion-FlipperZero works with the following 125 kHz badge formats:

- **EM4100** - Common in older systems
- **HID Prox** - Industry standard for access control
- **Indala** - Used in many corporate environments
- **AWID** - Popular in security systems
- **ioProx** - Found in newer installations
- **Gallagher** - Used in high-security settings
- **Nexwatch** - Integrated security systems

## ❓ Frequently Asked Questions

**Q: Will this damage my badge?**
A: No. Bastion is read-only and only reads the badge's data—it cannot write or modify it.

**Q: Do I need a computer to use this?**
A: No. Bastion runs entirely on your Flipper Zero. The computer is only needed for the initial download.

**Q: What if my badge isn't supported?**
A: Bastion supports the most common 125 kHz badge types. If yours isn't listed, try it anyway—it may still work.

**Q: How accurate is the grading?**
A: Grades are based on industry-standard security assessments for each badge type. They are meant as a guide, not a definitive security audit.

**Q: Can I use this for penetration testing?**
A: Yes. Bastion is a valuable tool for red teams and physical security testers evaluating badge systems.

## 🔒 Security & Privacy

- **No data collection**: Bastion does not send any data off your device.
- **Read-only operation**: Never writes to badges.
- **Open source**: Code is available for review on GitHub.

## 🆘 Support & Troubleshooting

**Bastion doesn't detect my badge:**
- Ensure the badge is properly positioned on the back of the Flipper Zero.
- Try a different badge to confirm the device is working.
- Restart your Flipper Zero.

**Error message appears:**
- Write down the error code and visit the GitHub repository for help.

**Need more help?**
- Open an issue on the GitHub repository: [https://github.com/couchant-nightbird919/Bastion-FlipperZero](https://github.com/couchant-nightbird919/Bastion-FlipperZero)

## 📜 License

This project is open source. See the LICENSE file on GitHub for details.

## 🏆 Acknowledgments

Built for the Flipper Zero community. Special thanks to all contributors and testers.

Keywords: 125khz,access-control,em4100,embedded-c,fap,flipper-zero,flipperzero,hid-prox,lfrfid,pentesting,physical-security,red-team,rfid,security