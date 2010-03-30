/* openclose.c: open and close files for TeX, Metafont, and BibTeX.

   Written 1995, 96 Karl Berry.  Public domain.  */

#include "config.h"
#include "lib.h"
#include <kpathsea/c-pathch.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/variable.h>
#include <kpathsea/absolute.h>

/* The globals we use to communicate.  */
extern string nameoffile;
extern unsigned namelength;
/* For "file:line:error style error messages. */
extern string fullnameoffile;
/* For the filename recorder. */
extern boolean recorder_enabled;
/* For the output-dir option. */
extern string output_directory;

/* Define some variables. */
string fullnameoffile;       /* Defaults to NULL.  */
static string recorder_name; /* Defaults to NULL.  */
static FILE *recorder_file;  /* Defaults to NULL.  */
boolean recorder_enabled;    /* Defaults to false. */
string output_directory;     /* Defaults to NULL.  */

/* For TeX and MetaPost.  See below.  Always defined so we don't have to
   #ifdef, and thus this file can be compiled once and go in lib.a.  */
int tfmtemp;
int ocptemp;
int texinputtype;

/* Helpers for the filename recorder... */
/* Start the recorder */
static void
recorder_start(void)
{
    /* Alas, while we'd like to use mkstemp it is not portable,
       and doing the autoconfiscation (and providing fallbacks) is more
       than we want to cope with.  So we have to be content with using a
       default name.  Throw in the pid so at least parallel builds might
       work (Debian bug 575731).  */
    string cwd;
    pid_t pid = getpid();
    char pid_str[MAX_INT_LENGTH];
    sprintf (pid_str, "%ld", (long) pid);
    
    recorder_name = (string)xmalloc(strlen(kpse_program_name)+5);
    strcpy(recorder_name, kpse_program_name);
    strcat(recorder_name, pid_str);
    strcat(recorder_name, ".fls");
    
    /* If an output directory was specified, use it instead of cwd.  */
    if (output_directory) {
      string temp = concat3(output_directory, DIR_SEP_STRING, recorder_name);
      free(recorder_name);
      recorder_name = temp;
    }
    
    recorder_file = xfopen(recorder_name, FOPEN_W_MODE);
    
    cwd = xgetcwd();
    fprintf(recorder_file, "PWD %s\n", cwd);
    free(cwd);
}

/* Change the name of the recorder file. */
void
recorder_change_filename (string new_name)
{
   if (!recorder_file)
     return;
   rename(recorder_name, new_name);
   free(recorder_name);
   recorder_name = xstrdup(new_name);
}

/* helper for recorder_record_* */
static void
recorder_record_name (const_string prefix, const_string name)
{
    if (recorder_enabled) {
        if (!recorder_file)
            recorder_start();
        fprintf(recorder_file, "%s %s\n", prefix, name);
        fflush(recorder_file);
    }
}

/* record an input file name */
void
recorder_record_input (const_string name)
{
    recorder_record_name ("INPUT", name);
}

/* record an output file name */
void
recorder_record_output (const_string name)
{
    recorder_record_name ("OUTPUT", name);
}

/* Open an input file F, using the kpathsea format FILEFMT and passing
   FOPEN_MODE to fopen.  The filename is in `nameoffile+1'.  We return
   whether or not the open succeeded.  If it did, `nameoffile' is set to
   the full filename opened, and `namelength' to its length.  */

boolean
open_input (FILE **f_ptr, int filefmt, const_string fopen_mode)
{
    string fname = NULL;
#ifdef FUNNY_CORE_DUMP
    /* This only applies if a preloaded TeX/Metafont is being made;
       it allows automatic creation of the core dump (typing ^\ loses
       since that requires manual intervention).  */
    if ((filefmt == kpse_tex_format || filefmt == kpse_mf_format
         || filefmt == kpse_mp_format)
        && STREQ (nameoffile + 1, "HackyInputFileNameForCoreDump.tex"))
        funny_core_dump ();
#endif

    /* We havent found anything yet. */
    *f_ptr = NULL;
    if (fullnameoffile)
        free(fullnameoffile);
    fullnameoffile = NULL;
    
    /* Look in -output-directory first, if the filename is not
       absolute.  This is because .aux and other such files will get
       written to the output directory, and we have to be able to read
       them from there.  We only look for the name as-is.  */
    if (output_directory && kpse_absolute_p (nameoffile+1, false)) {
        fname = concat3 (output_directory, DIR_SEP_STRING, nameoffile + 1);
        *f_ptr = fopen (fname, fopen_mode);
        if (*f_ptr) {
            free (nameoffile);
            namelength = strlen (fname);
            nameoffile = (string) xmalloc (namelength + 2);
            strcpy (nameoffile + 1, fname);
            fullnameoffile = fname;
        } else {
            free (fname);
        }
    }

    /* No file means do the normal search. */
    if (*f_ptr == NULL) {
        /* A negative FILEFMT means don't use a path.  */
        if (filefmt < 0) {
            /* no_file_path, for BibTeX .aux files and MetaPost things.  */
            *f_ptr = fopen(nameoffile + 1, fopen_mode);
            /* FIXME... fullnameoffile = xstrdup(nameoffile + 1); */
        } else {
            /* The only exception to `must_exist' being true is \openin, for
               which we set `tex_input_type' to 0 in the change file.  */
            /* According to the pdfTeX people, pounding the disk for .vf files
               is overkill as well.  A more general solution would be nice. */
            boolean must_exist = (filefmt != kpse_tex_format || texinputtype)
                    && (filefmt != kpse_vf_format);
            fname = kpse_find_file (nameoffile + 1,
                                    (kpse_file_format_type)filefmt,
                                    must_exist);
            if (fname) {
                fullnameoffile = xstrdup(fname);
                /* If we found the file in the current directory, don't leave
                   the `./' at the beginning of `nameoffile', since it looks
                   dumb when `tex foo' says `(./foo.tex ... )'.  On the other
                   hand, if the user said `tex ./foo', and that's what we
                   opened, then keep it -- the user specified it, so we
                   shouldn't remove it.  */
                if (fname[0] == '.' && IS_DIR_SEP (fname[1])
                    && (nameoffile[1] != '.' || !IS_DIR_SEP (nameoffile[2])))
                {
                    unsigned i = 0;
                    while (fname[i + 2] != 0) {
                        fname[i] = fname[i + 2];
                        i++;
                    }
                    fname[i] = 0;
                }

                /* kpse_find_file always returns a new string. */
                free (nameoffile);
                namelength = strlen (fname);
                nameoffile = (string)xmalloc (namelength + 2);
                strcpy (nameoffile + 1, fname);
                free (fname);

                /* This fopen is not allowed to fail. */
                *f_ptr = xfopen (nameoffile + 1, fopen_mode);
            }
        }
    }

    if (*f_ptr) {
        recorder_record_input (nameoffile + 1);

        /* If we just opened a TFM file, we have to read the first
           byte, to pretend we're Pascal.  See tex.ch and mp.ch.
           Ditto for the ocp/ofm Omega file formats.  */
        if (filefmt == kpse_tfm_format) {
            tfmtemp = getc (*f_ptr);
            /* We intentionally do not check for EOF here, i.e., an
               empty TFM file.  TeX will see the 255 byte and complain
               about a bad TFM file, which is what we want.  */
        } else if (filefmt == kpse_ocp_format) {
            ocptemp = getc (*f_ptr);
        } else if (filefmt == kpse_ofm_format) {
            tfmtemp = getc (*f_ptr);
        }
    }            

    return *f_ptr != NULL;
}

/* Open an output file F either in the current directory or in
   $TEXMFOUTPUT/F, if the environment variable `TEXMFOUTPUT' exists.
   (Actually, this also applies to the BibTeX and MetaPost output files,
   but `TEXMFMPBIBOUTPUT' was just too long.)  The filename is in the
   global `nameoffile' + 1.  We return whether or not the open
   succeeded.  If it did, `nameoffile' is reset to the name opened if
   necessary, and `namelength' to its length.  */

boolean
open_output (FILE **f_ptr, const_string fopen_mode)
{
    string fname;
    boolean absolute = kpse_absolute_p(nameoffile+1, false);

    /* If we have an explicit output directory, use it. */
    if (output_directory && !absolute) {
        fname = concat3(output_directory, DIR_SEP_STRING, nameoffile + 1);
    } else {
        fname = nameoffile + 1;
    }

    /* Is the filename openable as given?  */
    *f_ptr = fopen (fname, fopen_mode);

    if (!*f_ptr) {
        /* Can't open as given.  Try the envvar.  */
        string texmfoutput = kpse_var_value("TEXMFOUTPUT");

        if (texmfoutput && *texmfoutput && !absolute) {
            if (fname != nameoffile + 1)
                free(fname);
            fname = concat3(texmfoutput, DIR_SEP_STRING, nameoffile+1);
            *f_ptr = fopen(fname, fopen_mode);
        }
    }
    /* If this succeeded, change nameoffile accordingly.  */
    if (*f_ptr) {
        if (fname != nameoffile + 1) {
            free (nameoffile);
            namelength = strlen (fname);
            nameoffile = (string)xmalloc (namelength + 2);
            strcpy (nameoffile + 1, fname);
        }
        recorder_record_output (fname);
    }
    if (fname != nameoffile +1)
        free(fname);
    return *f_ptr != NULL;
}

/* Close F.  */

void
close_file (FILE *f)
{
  /* If F is null, just return.  bad_pool might close a file that has
     never been opened.  */
  if (!f)
    return;
    
  if (fclose (f) == EOF) {
    /* It's not always nameoffile, we might have opened something else
       in the meantime.  And it's not easy to extract the filenames out
       of the pool array.  So just punt on the filename.  Sigh.  This
       probably doesn't need to be a fatal error.  */
    perror ("fclose");
  }
}
