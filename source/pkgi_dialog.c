#include "pkgi_dialog.h"
#include "pkgi.h"
#include "pkgi_style.h"
#include "pkgi_utils.h"

#include <ppu-types.h>
#include <sysutil/msg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iconv.h>

#include <sys/timer.h> // ADICIONADO: Inclui sys_timer_usleep
#include <sys/process.h> // ADICIONADO: Inclui sysUtilCheckCallback

// ===========================================================
// Conversor UTF-8 → Latin-1 (ISO-8859-1) para textos no PS3
// ===========================================================
static const char* pkgi_utf8_to_latin1(const char* input)
{
    static char output[512];
    memset(output, 0, sizeof(output));

    iconv_t cd = iconv_open("ISO-8859-1", "UTF-8");
    if (cd == (iconv_t)-1)
        return input; // fallback

    char* inbuf = (char*)input;
    char* outbuf = output;
    size_t inbytesleft = strlen(input);
    size_t outbytesleft = sizeof(output) - 1;

    if (iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft) == (size_t)-1)
    {
        iconv_close(cd);
        return input; // fallback em caso de erro
    }

    *outbuf = '\0';
    iconv_close(cd);
    return output;
}

// ===========================================================
// Estrutura do diálogo
// ===========================================================
typedef struct {
    char title[256];
    char text[512];
    int result;
    int active;
} Dialog;

static Dialog dialog;

static void msg_dialog_event(msgButton button, void* userdata)
{
    (void)userdata;
    if (button == MSG_DIALOG_BTN_YES)
        dialog.result = 1;
    else if (button == MSG_DIALOG_BTN_NO)
        dialog.result = 0;
    else
        dialog.result = -1;
    dialog.active = 0;
}

// ===========================================================
// Funções públicas
// ===========================================================
void pkgi_dialog_init(void)
{
    memset(&dialog, 0, sizeof(dialog));
}

void pkgi_dialog_message(const char* title, const char* text)
{
    strncpy(dialog.title, pkgi_utf8_to_latin1(title), sizeof(dialog.title) - 1);
    strncpy(dialog.text, pkgi_utf8_to_latin1(text), sizeof(dialog.text) - 1);

    dialog.active = 1;
    msgDialogOpen2(
        MSG_DIALOG_NORMAL,
        pkgi_utf8_to_latin1(dialog.text),
        msg_dialog_event,
        NULL,
        NULL
    );
}

int pkgi_dialog_confirm(const char* title, const char* text)
{
    strncpy(dialog.title, pkgi_utf8_to_latin1(title), sizeof(dialog.title) - 1);
    strncpy(dialog.text, pkgi_utf8_to_latin1(text), sizeof(dialog.text) - 1);

    dialog.active = 1;
    dialog.result = -1;

    msgDialogOpen2(
        MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO,
        pkgi_utf8_to_latin1(dialog.text),
        msg_dialog_event,
        NULL,
        NULL
    );

    while (dialog.active)
    {
        sysUtilCheckCallback();
        sys_timer_usleep(10000);
    }

    return dialog.result;
}

void pkgi_dialog_error(const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // CORREÇÃO: Usar a string original "ERROR" para que o Gettext a traduza
    pkgi_dialog_message("ERROR", buffer);
}

void pkgi_dialog_ok(const char* text)
{
    // CORREÇÃO: Usar a string original "Content" (ou similar) para que o Gettext a traduza
    pkgi_dialog_message("Content", text);
}

int pkgi_dialog_is_active(void)
{
    return dialog.active;
}
