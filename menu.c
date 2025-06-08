#include    <sys/utsname.h>
#include    <sys/wait.h>
#include    <dirent.h>
#include    <fcntl.h>
#include    <ncurses.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <unistd.h>

#include    "lib.h"
#include    "ll.h"
#include    "vector.h"

static struct vector vec_desktop_entries = { 0 };

struct desktop_entry {

    char *key, *value;
    struct desktop_entry *next;

};

static int cols = 0;
static int lines = 0;

static WINDOW *title = 0;
static WINDOW *menu = 0;
static WINDOW *status = 0;

static void cleanup (void) {

    if (title) {
        delwin (title);
    }
    
    if (menu) {
        delwin (menu);
    }
    
    if (status) {
        delwin (status);
    }
    
    endwin ();

}

static int str_ends_with (const char *s, const char *suffix) {

    unsigned long slen = strlen (s);
    unsigned long suffix_len = strlen (suffix);
    
    return suffix_len <= slen && !strcmp (s + slen - suffix_len, suffix);

}

static int is_wsl (void) {

    struct utsname buf;
    
    if (uname (&buf) != 0) {
        return 0;
    }
    
    if (strcmp (buf.sysname, "Linux") != 0) {
        return 0;
    }
    
    if (str_ends_with (buf.release, "microsoft-standard-WSL2")) {
        return 1;
    }
    
    if (str_ends_with (buf.release, "-Microsoft")) {
        return 2;
    }
    
    return 0;

}


static struct desktop_entry *new_desktop_entry (char *key, char *value) {

    struct desktop_entry *de = xmalloc (sizeof (*de));
    
    de->key = key;
    de->value = value;
    
    return de;

}

static void parse_file (const char *filename) {

    struct desktop_entry head = { 0 }, *de = &head;
    FILE *fp;
    
    char *line, *line_end;
    void *load_line_internal_data = NULL;
    
    char *trimmed_line, *delimiter;
    char *key, *value;
    
    int no_add = 1;
    
    if (!(fp = fopen (filename, "r"))) {
        return;
    }
    
    load_line_internal_data = load_line_create_internal_data ();
    
    while (!load_line (&line, &line_end, fp, &load_line_internal_data)) {
    
        trimmed_line = trim_whitespace (line);
        
        if (trimmed_line[0] == '[') {
        
            no_add = (strcmp (trimmed_line, "[Desktop Entry]") != 0);
            continue;
        
        }
        
        if (no_add) {
            continue;
        }
        
        if ((delimiter = strchr (trimmed_line, '='))) {
        
            *delimiter = '\0';
            
            key = xstrdup (trim_whitespace (trimmed_line));
            value = xstrdup (trim_whitespace (delimiter + 1));
            
            de = de->next = new_desktop_entry (key, value);
        
        }
    
    }
    
    load_line_destroy_internal_data (load_line_internal_data);
    fclose (fp);
    
    if (!head.next) {
        return;
    }
    
    no_add = 0;
    
    for (de = head.next; de; de = de->next) {
    
        if (strcmp (de->key, "NoDisplay") == 0) {
        
            if (strcmp (de->value, "true") == 0) {
                no_add = 1;
            } else if (strcmp (de->value, "fales") == 0) {
                no_add = 0;
            }
        
        }
    
    }
    
    if (no_add) {
    
        struct desktop_entry *next_de;
        
        for (de = head.next; de; de = next_de) {
        
            next_de = de->next;
            
            free (de->value);
            free (de->key);
            
            free (de);
        
        }
        
        return;
    
    }
    
    vec_push (&vec_desktop_entries, head.next);

}

static void get_entries (const char *root) {

    struct dirent *de;
    DIR *dir;
    
    char *temp;
    
    if (!(dir = opendir (root))) {
        return;
    }
    
    while ((de = readdir (dir))) {
    
        if (de->d_name[0] == '.') {
            continue;
        }
        
        temp = xmalloc (strlen (root) + 1 + strlen (de->d_name) + 1);
        sprintf (temp, "%s/%s", root, de->d_name);
        
        parse_file (temp);
        free (temp);
    
    }
    
    closedir (dir);

}

static void draw_menu (int lines, int cols, int normal, int selected) {

    if (normal == 0) {
    
        struct desktop_entry *e;
        unsigned long skip = 0, i, j;
        
        if (selected >= lines - 2) {
        
            for (i = selected; i > lines - 2; i--) {
                skip++;
            }
            
            selected = lines - 2;
        
        }
        
        wclear (menu);
        
        for (i = skip; j = 0, i < vec_desktop_entries.length && j < lines - 2; i++, j++) {
        
            for (e = vec_desktop_entries.data[i]; e; e = e->next) {
            
                if (strcmp (e->key, "Name") == 0) {
                
                    if (strlen (e->value) >= cols - 3) {
                        mvwprintw (menu, (i - skip) + 1, 3, "%.*s...", cols - 6 - 3, e->value);
                    } else {
                        mvwprintw (menu, (i - skip) + 1, 3, "%s", e->value);
                    }
                
                }
            
            }
        
        }
    
    }
    
    if (normal > 0 && normal <= vec_desktop_entries.length) {
        mvwchgat (menu, normal, 0, cols, A_NORMAL, 1, NULL);
    }
    
    if (selected > 0 && selected <= vec_desktop_entries.length) {
        mvwchgat (menu, selected, 0, cols, A_STANDOUT, 1, NULL);
    }
    
    wnoutrefresh (menu);

}

int main (int argc, char **argv) {

    static int selected = 1, normal = 0;
    static int menu_lines = 0, menu_cols = 0;
    
    char *root = "share/applications", *home = getenv ("HOME"), *temp;
    int i;
    
    atexit (cleanup);
    
    temp = xmalloc (5 + strlen (root) + 1);
    sprintf (temp, "/usr/%s", root);
    
    get_entries (temp);
    free (temp);
    
    temp = xmalloc (1 + 3 + 1 + 5 + 1 + strlen (root) + 1);
    sprintf (temp, "/usr/local/%s", root);
    
    get_entries (temp);
    free (temp);
    
    temp = xmalloc (strlen (home) + 1 + 6 + 1 + strlen (root) + 1);
    sprintf (temp, "%s/.local/%s", home, root);
    
    get_entries (temp);
    free (temp);
    
    if (vec_desktop_entries.length == 0) {
    
        printf ("No application launchers found\n");
        return EXIT_SUCCESS;
    
    }
    
_restart:
    
    initscr ();
    cbreak ();
    raw ();
    noecho ();
    
    curs_set (0);
    getmaxyx (stdscr, lines, cols);
    
    if (cols & 1) {
        cols = (cols / 2) * 2;
    }
    /*
    if (!(title = newwin (1, cols, 0, 0))) {
        return EXIT_FAILURE;
    }
    
    wattron (title, A_REVERSE);
    */
    menu_lines = (lines - 2 /* title and status */) - 6;
    menu_cols = cols / 2;
    
    if (!(menu = newwin (menu_lines, menu_cols, 4, (cols / 2) - (menu_cols / 2)))) {
        return EXIT_FAILURE;
    }
    
    keypad (menu, true);
    /*
    if (!(status = newwin (1, cols, lines - 1, 0))) {
        return EXIT_FAILURE;
    }
    
    wattron (status, A_REVERSE);
    
    for (i = 0; i < cols; i++) {
    
        wprintw (title, " ");
        wprintw (status, " ");
    
    }
    */
    draw_menu (menu_lines, menu_cols, normal, selected);
    box (menu, 0, 0);
    
    /*wnoutrefresh (title);
    wnoutrefresh (status);*/
    
    doupdate ();
    
    for (;;) {
    
        struct desktop_entry *de;
        int retcode;
        
        char *exec = 0;
        int exec_terminal = 0;
        
        int ch = wgetch (menu);
        
        if (ch == 'q') {
            break;
        }
        
        switch (ch) {
        
            case '\n':
            
                for (de = vec_desktop_entries.data[selected - 1]; de; de = de->next) {
                
                    if (strcmp (de->key, "Exec") == 0) {
                    
                        if (exec) {
                            free (exec);
                        }
                        
                        exec = xstrdup (de->value);
                    
                    } else if (strcmp (de->key, "Terminal") == 0) {
                        exec_terminal = (strcmp (de->value, "true") == 0);
                    }
                
                }
                
                if (exec) {
                
                    char *p;
                    pid_t pid;
                    
                    if ((p = strchr (exec, ' '))) {
                        *p = '\0';
                    }
                    
                    if ((pid = fork ()) < 0) {
                    
                        free (exec);
                        break;
                    
                    }
                    
                    if (pid == 0) {     /* Child */
                    
                        int dev_null = open ("/dev/null", O_WRONLY);
                        
                        if (dev_null == -1) {
                            exit (EXIT_FAILURE);
                        }
                        
                        dup2 (dev_null, STDOUT_FILENO);
                        dup2 (dev_null, STDERR_FILENO);
                        
                        close (dev_null);
                        
                        if (exec_terminal) {
                        
                            if (is_wsl ()) {
                            
                                execlp ("cmd.exe", "cmd.exe", "/C", "start", "wsl", "-e", exec, NULL);
                                exit (EXIT_FAILURE);
                            
                            } else {
                            
                                execlp ("x-terminal-emulator", "x-terminal-emulator", "-e", exec, NULL);
                                exit (EXIT_FAILURE);
                            
                            }
                        
                        } else {
                        
                            execlp (exec, exec, NULL);
                            exit (EXIT_FAILURE);
                        
                        }
                    
                    }
                    
                    free (exec);
                
                }
                
                break;
            
            case KEY_DOWN:
            
                if (selected < vec_desktop_entries.length) {
                
                    normal = selected;
                    
                    if (++selected >= menu_lines - 2) {
                        normal = 0;
                    }
                    
                    draw_menu (menu_lines, menu_cols, normal, selected);
                    box (menu, 0, 0);
                    
                    doupdate ();
                
                }
                
                break;
            
            case KEY_UP:
            
                if (selected > 1) {
                
                    normal = selected;
                    
                    if (--selected >= menu_lines - 2) {
                        normal = 0;
                    }
                    
                    draw_menu (menu_lines, menu_cols, normal, selected);
                    box (menu, 0, 0);
                    
                    doupdate ();
                
                }
                
                break;
            
            default:
            
                break;
        
        }
    
    }
    
    return EXIT_SUCCESS;

}
