/* ------------------------------------------------------------------------- */
/*  Makes us able to change the version at a glance       VERSION.C          */
/* ------------------------------------------------------------------------- */

#define VERSION "3.11.02d"

char version[] = VERSION;
#ifdef ALPHA_TEST
char testsite[] = "Alpha Test Site";
#else
#  ifdef BETA_TEST
char testsite[] = "Beta Test Site";
#  else
char testsite[] = "";
#  endif
#endif

char cmpDate[] = __DATE__;
char cmpTime[] = __TIME__;

char *welcome[] = {    /* 10 LINES MAX LENGTH!!!!!! */
    "Come not between the dragon and his wrath.",
    "",
    "--William Shakespeare",
    0
};

char *copyright[] = {   /* 2 LINES ONLY!!!! */
    "DragCit Copyright (c) Peter Torkelson 1989, 1990",
    "Created using Turbo C, Copyright (c) Borland 1987, 1988",
    0
};

                        /* CRC's for the copyright array above */
unsigned int welcomecrcs[] = { 0xC97B, 0x0, 0x3F21 };

unsigned int copycrcs[] = { 0xA290, 0xD8F0 };

