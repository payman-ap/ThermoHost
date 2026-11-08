# Windows → Linux embedded development
Verified 2026-11-08. Reusable setup reference; no passwords or private keys.

## 1. Current configuration

| Item | Value |
|---|---|
| Windows | Work laptop, Windows 11; profile `C:\Users\pazaripasand` |
| Linux | Personal laptop `satellite`, Linux Mint **22.3 XFCE** (verified version) |
| Linux user | `karateka`; existing account with `sudo`, `plugdev`, `dialout` |
| SSH destination | `192.168.178.25:22`, alias `mint-embedded` |
| Project root | `/home/karateka/remote_proj` |
| Hardware | USB devices connect directly to Linux; no USB forwarding |

Windows runs Codex and VS Code; Linux runs toolchains, builds, uploads and debugging.
Returned source/output can enter AI context: SSH does not make AI processing local.
Both laptops use the home LAN; no router port forwarding was configured.
Keep Linux awake while plugged in. A router DHCP reservation for its address is recommended, not verified as configured.

## 2. Linux installation

Commands used in this setup:

```bash
sudo apt update
sudo apt install openssh-server
sudo systemctl enable --now ssh
mkdir -p ~/remote_proj
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi openocd gdb-multiarch
sudo usermod -aG dialout karateka
```

These helpers were also available; install missing ones on a new host:

```bash
sudo apt install git python3-venv cmake make
```

- `dialout` membership is permanent. Existing processes need a fresh login.
- Linux already supplied the CP2102 USB serial driver; we did not install one.
- OpenOCD accessed ST-Link without root after installation and USB reconnection.
- UFW was inactive when checked. SSH password/root-login hardening was not performed.
- No passwordless sudo was configured by us; package installations required the user's sudo password.

## 3. Windows SSH setup

Built-in OpenSSH was already installed at `C:\Windows\System32\OpenSSH` (9.5p2).
A dedicated key was created; the existing GitHub `id_ed25519` key was unchanged.

Ordinary PowerShell, **new setup only**; do not overwrite an existing key:

```powershell
ssh-keygen -t ed25519 -f "$env:USERPROFILE\.ssh\id_ed25519_satellite" -C "codex-satellite"
```

Administrator PowerShell, once, through the approved company process:

```powershell
Set-Service -Name ssh-agent -StartupType Automatic
Start-Service ssh-agent
```

Ordinary PowerShell; enter the key passphrase locally:

```powershell
ssh-add "$env:USERPROFILE\.ssh\id_ed25519_satellite"
ssh-add -l
```

On Linux, as `karateka`, authorize **only the public key**:

```bash
mkdir -p ~/.ssh
chmod 700 ~/.ssh
touch ~/.ssh/authorized_keys
chmod 600 ~/.ssh/authorized_keys
# Substitute the one-line contents of id_ed25519_satellite.pub:
printf '%s\n' 'PASTE_PUBLIC_KEY_HERE' >> ~/.ssh/authorized_keys
```

Windows file `C:\Users\pazaripasand\.ssh\config`:

```sshconfig
Host mint-embedded
    HostName 192.168.178.25
    User karateka
    IdentityFile ~/.ssh/id_ed25519_satellite
    IdentitiesOnly yes
    ForwardAgent no
    ServerAliveInterval 30
    ServerAliveCountMax 3
```

**What the alias means:** `mint-embedded` is a shortcut defined in this Windows user's SSH config. It is not the Linux username or a DNS hostname. SSH expands it to user `karateka`, host `192.168.178.25`, port `22`, and applies the key/options above.

```powershell
ssh mint-embedded               # Configured shortcut, including the dedicated key
ssh karateka@192.168.178.25      # Direct address: same account and machine
```

The direct command does not match `Host mint-embedded`, so it does not automatically apply that entry's key/options. It can still authenticate using available agent/default keys or a password. To explicitly select our key without the alias:

```powershell
ssh -i "$env:USERPROFILE\.ssh\id_ed25519_satellite" -o IdentitiesOnly=yes karateka@192.168.178.25
```

Keep the private key on Windows. Config-file permissions were corrected to owner, SYSTEM, and Windows Administrators because OpenSSH rejected an inherited extra-user entry.

Verify the host identity directly on Linux before first acceptance:

```bash
sudo ssh-keygen -lf /etc/ssh/ssh_host_ed25519_key.pub
```

Verified ED25519 host fingerprint:
`SHA256:NCqRlSOdq7nJDmN+bKZmsq4HcM2E5VqEBw3+5y4EWP0`

Windows remembers it in `~/.ssh/known_hosts`. Recheck locally after a Linux reinstall or host-key warning.

## 4. VS Code / PlatformIO

Windows: VS Code + Microsoft **Remote - SSH**.
Connect: `Ctrl+Shift+P → Remote-SSH: Connect to Host… → mint-embedded → Linux`.
Open the desired remote project folder.

Remote - SSH installed VS Code Server on Linux. PlatformIO IDE was installed **remotely**.
Its Core CLI is `~/.platformio/penv/bin/pio` (verified 6.2.0); it need not be on PATH.
PlatformIO downloads its own frameworks, compilers, OpenOCD and debuggers.
Verified platforms: STM32 20.0.0; Espressif32 8.0.0. ESP-DSP was already bundled.

```bash
cd ~/remote_proj/esp32_max6675_wifi  # substitute your project
~/.platformio/penv/bin/pio run
~/.platformio/penv/bin/pio run -t upload
~/.platformio/penv/bin/pio device monitor -b 115200
```

Close serial monitors before uploading. VS Code also provides Build, Upload and PIO Debug.
A key-passphrase prompt uses the SSH key passphrase; an SSH login-password prompt uses the Linux password.

## 5. Existing projects / hardware

All folders below are under `/home/karateka/remote_proj`.

| Folder | Purpose |
|---|---|
| `bluepill_blink` | Bare-metal STM32, Make |
| `bluepill_platformio_blink` | STM32Cube HAL; ST-Link upload and VS Code breakpoints verified |
| `esp32_platformio_blink` | ESP32-WROOM-32D, external GPIO23 LED + series resistor |
| `esp32_max6675_wifi` | Thermocouple, local web graph, DSP, recording and CSV |

- **Blue Pill:** ST-Link SWDIO → PA13/DIO, SWCLK → PA14/CLK, GND → GND. Powered from ST-Link 3.3 V. PC13 LED; SWD speed 100 kHz.
- **ESP32:** CP2102 USB UART, usually `/dev/ttyUSB0`. Identify it with `ls -l /dev/serial/by-id/`. Classic ESP32 breakpoints require a suitable separate JTAG probe; USB UART handles upload/logs.
- **MAX6675:** 3.3 V, GND, SCK → GPIO18, SO → GPIO19, CS → GPIO27.
- Wi-Fi credentials: `esp32_max6675_wifi/include/wifi_secrets.h`, Git-ignored with owner-only permissions. Do not print/share it; compiled firmware also contains credentials. ESP32 requires **2.4 GHz**.
- Current thermometer: `http://192.168.178.38/`; DHCP may change it, so read the serial log. Recording is browser-memory-only: export before refresh/close/new recording.

## 6. Checks and fixes

Windows:

```powershell
Test-NetConnection 192.168.178.25 -Port 22
ssh -G mint-embedded | Select-String '^(hostname|user|identityfile) '
ssh mint-embedded "whoami; hostname; pwd"
Get-Service ssh-agent
ssh-add -l
```

Linux:

```bash
ip -brief -4 address
systemctl is-active ssh
systemctl is-enabled ssh
sudo ufw status verbose
id
lsusb
ls -l /dev/serial/by-id/ /dev/ttyUSB*
fuser /dev/ttyUSB0
```

| Problem | Fix/check |
|---|---|
| SSH unreachable | Host IP, power/suspend, LAN, SSH service, firewall. |
| Alias cannot resolve | Remove erroneous duplicate alias entries. An earlier `HostName mint-embedded` overrode the IP. |
| Codex cannot access agent | The Windows sandbox denied agent access; approved execution outside it worked. Use the native OpenSSH path above; respect managed permissions. |
| VS Code serial permission denied after adding `dialout` | Save files; **Remote-SSH: Kill VS Code Server on Host…** → `mint-embedded`; reconnect. Closing/reopening alone reused old process groups. |
| ESP32 stub/connection error | Retry once. If needed: hold BOOT, press/release EN/RST, release BOOT, retry. Repeated errors: cable, USB port/hub, power. Successful crystal detection was 40 MHz. |
| ST-Link sees no target | Check SWD wiring, ground, power, reset and SWD speed. |
| Windows device Code 28 | Driver missing: STSW-LINK009 for ST-Link; Silicon Labs CP210x for ESP32. Needs approved system installation; DLL copying/PATH changes do not solve it. |

Local Windows copies exist at `C:\Payman\my_projects\bluepill_platformio_blink` and `...\esp32_platformio_blink`.
Windows builds passed, but missing USB drivers blocked direct uploads. These copies are not synchronized with Linux.

## 7. Handoff to another AI

Use `ssh mint-embedded`; work in the chosen Linux project. Use PlatformIO's managed CLI path.
Codex may need approved execution outside its Windows sandbox to access the SSH agent.
Check USB detection/port before uploading. Keep credentials out of logs/chat/Git.
Preserve projects and recordings; test UI in a separate tab. Do not install work-laptop software/drivers or bypass company controls.

