**Faizan-rm 🗑️**

A safe rm alternative with trash, restore, and auto-cleanup

faizan-rm is a Linux command-line tool that works like rm, but safer.
Instead of permanently deleting files and folders, it moves them to a user-local trash, allowing you to list, restore, or permanently delete them later.

✨ **Features**

✅ Safe deletion (no immediate permanent loss)

♻️ Restore deleted files & folders

📋 List trash contents with timestamps

🧹 Auto-cleanup old trash files (default: 90 days)

🔥 Force delete & permanent delete support

🗂️ Metadata stored using JSON

⚡ Background cleanup using async threads

📂 Trash Structure
~/.local/share/Faizan_rm/
├── files/   # Deleted files & folders
└── info/    # JSON metadata for each item

🔧 **Installation**
1️⃣ Build
g++ -std=c++17 faizan_rm.cpp -o faizan-rm

2️⃣ Install binary
chmod +x faizan-rm
mv faizan-rm ~/.local/bin/


Make sure ~/.local/bin is in your PATH.

3️⃣ Install man page
mkdir -p ~/.local/share/man/man1
cp faizan-rm.1 ~/.local/share/man/man1/
mandb ~/.local/share/man


Test:

man faizan-rm

🚀 **Usage**
Delete files safely
faizan-rm file.txt

Delete folders
faizan-rm -r myfolder


Force delete (no confirmation):

faizan-rm -rf myfolder

Delete multiple files
faizan-rm file1 file2 dir1

📋 List Trash
faizan-rm -list


Filter by folder:

faizan-rm myfolder -list

♻️ Restore Files

Restore interactively:

faizan-rm -Restore


Restore by folder:

faizan-rm myfolder -Restore


You can restore:

Single ID → 3

Multiple IDs → 1,4,7

Range → 2-6

🔥 Permanent Deletion

Delete selected trash items permanently:

faizan-rm -delete


Empty entire trash:

faizan-rm -empty


⚠️ These actions cannot be undone.

🧹 Auto Cleanup

Set cleanup days:

faizan-rm -cleandays 30


Default: 90 days

Cleanup runs asynchronously in the background

Config file:

~/.local/share/Faizan_rm/info/Faizan_rm_cleanupdays.json

📄 **Help**
faizan-rm -h


or

man faizan-rm

⚠️ **Notes**

Existing files are never overwritten during restore

Missing parent directories are recreated automatically

This does not integrate with GNOME/KDE trash (gio trash)

Designed for Linux systems

👤 **Author**

**Faizan Momin**
