This document describes the localization protocol used in Retro-go


# C files
translation.h contains the original messages and the corresponding translations ex :
````c
{
    .msg = "Yes",
    .fr = "Oui",
    .es = "Si"
},
````

If you want to add your own language :
First, this is what the struct definition should looks like in rg_localization.h :
````c
typedef struct {
    const char *msg;   // Original message in english
    const char *fr;    // FR Translated message

    const char *es;    // to add spanish translation
} Translation;
````

You should also update the enum accordingly (in rg_localization.h):
````c
enum languages
{
    RG_LANG_EN,
    RG_LANG_FR,

    RG_LANG_ES, // to add spanish translation

    RG_LANG_MAX
};
````


# Python tool
`rg_locate_str.py` is a simple python tool that locate every string preceded by `_("` pattern in each file of Retro-go project
Then the tool compare theses strings to the ones in `translations.h` and put the missing ones in a .txt file called missing_translation.txt