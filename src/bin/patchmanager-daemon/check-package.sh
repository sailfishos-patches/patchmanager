#!/bin/sh
if [ -e "$1" ]; then
  echo "Error: A package name must be provided!"
  exit -1
fi

rpm -q "$1" > /dev/null 2>&1
if [ ! $? = 0 ]; then
  echo "Error: Invalid package name!"
  exit -1
fi


# 
# Some constants
# 

# Paths / Files
BACKUP_PREFIX=/opt/patchmanager/backups
TMP_FILE=/tmp/package-tmp
ORIGINAL_FAILED_TMP_FILE=/tmp/original_failed.md5sums
BACKUP_FAILED_TMP_FILE=/tmp/backup_failed.md5sums
ORIGINAL_FAILED_FILE=/tmp/original_failed
BACKUP_FAILED_FILE=/tmp/backup_failed
LOG_FILE_PREFIX=/var/lib/patchmanager/check-package
LOG_ORIGINAL_FILE="$LOG_FILE_PREFIX-$1-original.log"
LOG_BACKUP_FILE="$LOG_FILE_PREFIX-$1-backup.log"


#
# Helper functions that do all the needed heavy work
#

do_check_failure() {
  rm -f $TMP_FILE
  rm -f $ORIGINAL_FAILED_FILE
  rm -f $BACKUP_FAILED_FILE
  rm -f $ORIGINAL_FAILED_TMP_FILE
  rm -f $BACKUP_FAILED_TMP_FILE
  exit 1
}

do_check_success() {
  rm -f $TMP_FILE
  rm -f $ORIGINAL_FAILED_FILE
  rm -f $BACKUP_FAILED_FILE
  rm -f $ORIGINAL_FAILED_TMP_FILE
  rm -f $BACKUP_FAILED_TMP_FILE
  exit 0
}

rm -f $LOG_FILE
rm -f $TMP_FILE
rm -f $ORIGINAL_FAILED_TMP_FILE
rm -f $BACKUP_FAILED_TMP_FILE
rm -f $ORIGINAL_FAILED_FILE
rm -f $BACKUP_FAILED_FILE
rm -f "$LOG_ORIGINAL_FILE"
rm -f "$LOG_BACKUP_FILE"

mkdir -p /var/lib/patchmanager
cd /

# Verify original files in package
rpm -q --queryformat '[%{FILEMD5S}  %{FILENAMES}\n]' "$1" | grep \\.qml > $TMP_FILE
sha256sum -c $TMP_FILE 2> /dev/null | grep -v OK > $ORIGINAL_FAILED_TMP_FILE
while read i; do
  file="$(echo "$i" | awk -F: '{print $1}')"
  echo "$file" >> $ORIGINAL_FAILED_FILE
done < $ORIGINAL_FAILED_TMP_FILE

# Verify backup files in package
rpm -q --queryformat "[%{FILEMD5S}  $BACKUP_PREFIX%{FILENAMES}\n]" "$1" | grep \\.qml > $TMP_FILE
sha256sum -c $TMP_FILE 2> /dev/null | grep -v OK > $BACKUP_FAILED_TMP_FILE
while read i; do
  file="$(echo "$i" | awk -F: '{print $1}')"
  echo "$file" >> $BACKUP_FAILED_FILE
done < $BACKUP_FAILED_TMP_FILE

# If the backup is up-to-date, it's ok
if [ ! -f $BACKUP_FAILED_FILE ]; then
  echo "Backup available."
  do_check_success
fi

# If the backup is not up-to-date, and original files are 
# ok, we create the backup files

if [ ! -f $ORIGINAL_FAILED_FILE ]; then
  rpm -ql "$1" | grep \\.qml > $TMP_FILE
  while read i; do
    backup_file="$BACKUP_PREFIX/$i"
    rm -f "$backup_file"
    backup_dir="$(dirname "$backup_file")"
    mkdir -p "$backup_dir"
    cp "$i" "$backup_file"
  done < $TMP_FILE
  echo "Backup done."
  do_check_success
fi

while read i; do
  file="$(echo "$i" | awk -F: '{print $1}')"
  echo "$file" >> $LOG_ORIGINAL_FILE
done < $ORIGINAL_FAILED_TMP_FILE

while read i; do
  file="$(echo "$i" | awk -F: '{print $1}')"
  echo "$file" >> $LOG_BACKUP_FILE
done < $BACKUP_FAILED_TMP_FILE


echo "Warning: Both backup and installed files are corrupted!"
do_check_failure
