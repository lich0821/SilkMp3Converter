# SILK and MP3 Converter

## Usage
Download `Converter.exe` [here](https://github.com/lich0821/SilkMp3Converter/releases/latest) and run.

```sh
usage: Converter.exe in.amr out.mp3 [sample_rate]

in.amr       : Bitstream input to decoder
out.mp3      : Speech output from decoder
[Hz]         : Sampling rate of output signal in Hz; default: 24000
```

## Quick Start

### Clone the repo

```sh
git clone https://github.com/lich0821/SilkMp3Converter.git
```

### Install lame (with vcpkg)

```sh
vcpkg install mp3lame:x86-windows-static
```

Build and run.
