# PhotoRec 7.3-WIP - Non-Interactive Distribution

A specialized distribution of PhotoRec 7.3-WIP engineered for headless environments and automated workflows. This version eliminates ncurses dependencies while introducing enhanced command-line functionality and structured logging capabilities.

## Key Features

- **No UI Dependencies**: Completely removed ncurses/curses dependencies
- **Size Filtering**: Skip files larger than specified size (supports K/M/G/T suffixes)
- **JSON Output**: Clean, structured logging for automated processing
- **Custom File Paths**: Configurable signature and session file paths with optional session disable
- **Backward Compatible**: Maintains existing `/cmd` parameter functionality

## NCurses Dependency Removal
- Maintains full compatibility with existing PhotoRec core functionality

## Structured Logging Framework
- Developed JSON-based logging system for integration with monitoring and analysis tools
- Implemented standardized event logging for session lifecycle, partition discovery, recovery progress, and file operations
- Replaced terminal control sequences with machine-readable output format

## Session and Signature Management
- Added `/sig FILE` parameter for custom signature file path specification
- Implemented `/sess FILE` parameter for custom session file location control
- Introduced `/nosess` flag to disable session file support and resume capability

## File Size Filtering Enhancement
- Implemented configurable maximum file size constraints via `/maxsize` parameter
- Added in-flight file size checking for early termination during recovery process instead of waiting for complete file reconstruction

# Build Instructions

```bash
sudo apt install -y build-essential autotools-dev autoconf libtool pkg-config zlib1g-dev libext2fs-dev libntfs-3g-dev libjpeg-dev libewf-dev

autoreconf -fiv

./configure --without-ncurses --enable-silent-rules

make -j10

ldd src/photorec | grep ncurses  # Should return empty
```

## Parameter Reference

| Parameter | Description | Example |
|-----------|-------------|---------|
| `/maxsize SIZE` | Maximum file size to recover | `/maxsize 100M`, `/maxsize 2G` |
| `/log` | Create log file | `/log` |
| `/logname FILE` | Specify log filename | `/logname recovery.log` |
| `/logjson FILE` | Enable JSON logging to specified file | `/logjson recovery.json` |
| `/d DIR` | Recovery directory | `/d recovered_files` |
| `/cmd DEVICE ACTION` | Device and action | `/cmd /dev/sda1 search` |
| `/debug` | Add debug information | `/debug` |
| `/sig FILE` | Custom signature file path | `/sig /path/to/signatures.txt` |
| `/sess FILE` | Custom session file path | `/sess /path/to/session.ses` |
| `/nosess` | Disable session file support (no resume capability) | `/nosess` |

## Size Format Support

The `/maxsize` parameter supports the following suffixes:
- `K` or `k`: Kilobytes (1024 bytes)
- `M` or `m`: Megabytes (1024² bytes)
- `G` or `g`: Gigabytes (1024³ bytes)
- `T` or `t`: Terabytes (1024⁴ bytes)
- No suffix: Bytes

## JSON Logging Format

The modified PhotoRec outputs structured JSON logs with the following message types:

<details>
<summary><strong>Session Start</strong></summary>

```json
{
  "type": "session_start",
  "timestamp": "2025-09-19T10:30:00Z",
  "photorec_version": "7.3-WIP",
  "recovery_directory": "/path/to/recovery",
  "max_file_size": 104857600,
  "paranoid_mode": true,
  "keep_corrupted_files": false
}
```
</details>

<details>
<summary><strong>Recovery Progress</strong></summary>

```json
{
  "type": "recovery_progress",
  "timestamp": "2025-09-19T10:30:05Z",
  "pass": 1,
  "current_sector": 1690894,
  "total_sectors": 8391537,
  "elapsed_time": "0h00m05s",
  "estimated_completion": "0h00m19s",
  "files_found": {
    "txt": 347,
    "cab": 5,
    "exe": 4,
    "bmp": 1,
    "total": 357
  }
}
```
</details>

<details>
<summary><strong>File Recovered</strong></summary>

```json
{
  "type": "file_recovered",
  "timestamp": "2025-09-19T10:30:06Z",
  "filename": "f0001234.jpg",
  "filetype": "jpg",
  "filesize": 2048576,
  "disk_offset": 123456789
}
```
</details>

<details>
<summary><strong>Partition Info</strong></summary>

```json
{
  "type": "partition_info",
  "timestamp": "2025-09-19T10:30:01Z",
  "disk": "/dev/sda1",
  "size_gb": 465.8
}
```
</details>

<details>
<summary><strong>Disk Info</strong></summary>

```json
{
  "type": "disk_info",
  "timestamp": "2025-09-19T10:30:01Z",
  "device": "/dev/sda",
  "size_gb": 465.8,
  "sector_size": 512
}
```
</details>

<details>
<summary><strong>Options Info</strong></summary>

```json
{
  "type": "options_info",
  "timestamp": "2025-09-19T10:30:01Z",
  "paranoid": true,
  "brute_force": false,
  "keep_corrupted": false,
  "ext2_mode": false,
  "expert_mode": false,
  "low_memory": false
}
```
</details>

<details>
<summary><strong>Signatures Info</strong></summary>

```json
{
  "type": "signatures_info",
  "timestamp": "2025-09-19T10:30:01Z",
  "enabled_signatures": 487
}
```
</details>

<details>
<summary><strong>Pass Info</strong></summary>

```json
{
  "type": "pass_info",
  "timestamp": "2025-09-19T10:30:02Z",
  "pass": 1,
  "blocksize": 512
}
```
</details>

<details>
<summary><strong>Session End</strong></summary>

```json
{
  "type": "session_end",
  "timestamp": "2025-09-19T10:35:00Z",
  "final_statistics": {
    "total_files": 1205,
    "txt": 875,
    "jpg": 156,
    "png": 98,
    "pdf": 76
  }
}
```
</details>

## Important Notes

- **Non-Interactive Only**: This version requires the `/cmd` parameter and will not start any interactive interface
- **No Menu System**: All file type selection and options must be specified via command line
- **JSON Output**: When using `/log`, the output format is structured JSON instead of the original text format
- **Size Filtering**: Files exceeding the `/maxsize` limit are automatically skipped during recovery

Below original readme:

**TestDisk** checks the partition and boot sectors of your disks.
It is very useful in recovering lost partitions.

It works with:

- DOS/Windows FAT12, FAT16 and FAT32
- NTFS (Windows NT/2K/XP)
- Linux Ext2 and Ext3
- BeFS (BeOS)
- BSD disklabel (FreeBSD/OpenBSD/NetBSD)
- CramFS (Compressed File System)
- HFS and HFS+, Hierarchical File System
- JFS, IBM's Journaled File System
- Linux Raid
- Linux Swap (versions 1 and 2)
- LVM and LVM2, Linux Logical Volume Manager
- Netware NSS
- ReiserFS 3.5, 3.6 and 4
- Sun Solaris i386 disklabel
- UFS and UFS2 (Sun/BSD/...)
- XFS, SGI's Journaled File System

**PhotoRec** is file data recovery software designed to recover
lost pictures from digital camera memory or even hard disks.
It has been extended to search also for non audio/video headers.

The whole list of file formats recovered by PhotoRec contains
[more than 480 file extensions](https://www.cgsecurity.org/wiki/File_Formats_Recovered_By_PhotoRec) (about 300 file families).

TestDisk and PhotoRec run on:

- DOS, Win 9x
- Windows
- Linux
- FreeBSD, NetBSD, OpenBSD
- SunOS

Both are licensed under the GNU General Public License.

See the [latest documentation](https://github.com/cgsecurity/testdisk_documentation)
To build from source, read [INSTALL](INSTALL).

Christophe GRENIER
[grenier@cgsecurity.org](mailto:grenier@cgsecurity.org)
[https://www.cgsecurity.org/](https://www.cgsecurity.org/)

[![Build Status](https://travis-ci.org/cgsecurity/testdisk.svg?branch=master)](https://travis-ci.org/cgsecurity/testdisk)
