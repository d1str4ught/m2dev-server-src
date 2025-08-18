#include "stdafx.h"

int is_hangul(const BYTE* str)
{
    if (str[0] >= 0xb0 && str[0] <= 0xc8 && str[1] >= 0xa1 && str[1] <= 0xfe)
        return 1;

    return 0;
}

int check_han(const char* str)
{
    int i, code;

    if (!str || !*str)
        return 0;

    for (i = 0; str[i]; i += 2)
    {
        if (isnhspace(str[i]))
            return 0;

        if (isalpha(str[i]) || isdigit(str[i]))
            continue;

        code = str[i];
        code += 256;

        if (code < 176 || code > 200)
            return 0;
    }

    return 1;
}

const char* first_han(const BYTE* str)
{
    unsigned char high, low;
    int len, i;
    char* p = "그외";

    static const char* wansung[] =
    {
        "가","가","나","다","다",
        "라","마","바","바","사",
        "사","아","자","자","차",
        "카","타","파","하",""
    };

    static const char* johab[] =
    {
        "\x88" "a", "\x8C" "a", "\x90" "a", "\x94" "a", "\x98" "a",
        "\x9C" "a", "\xA0" "a", "\xA4" "a", "\xA8" "a", "\xAC" "a",
        "\xB0" "a", "\xB4" "a", "\xB8" "a", "\xBC" "a", "\xC0" "a",
        "\xC4" "a", "\xC8" "a", "\xCC" "a", "\xD0" "a", ""
    };

    len = strlen((const char*)str);

    if (len < 2)
        return p;

    high = str[0];
    low = str[1];

    if (!is_hangul(str))
    {
        return p;
    }

    high = (KStbl[(high - 0xb0) * 94 + low - 0xa1] >> 8) & 0x7c;

    for (i = 0; johab[i][0]; i++)
    {
        low = (johab[i][0] & 0x7f);

        if (low == high)
            return (wansung[i]);
    }

    return (p);
}

int under_han(const void* orig)
{
    const BYTE* str = (const BYTE*)orig;
    BYTE high, low;
    int len;

    len = strlen((const char*)str);

    if (len < 2)
        return 0;

    if (str[len - 1] == ')')
    {
        while (str[len] != '(')
            len--;
    }

    high = str[len - 2];
    low = str[len - 1];

    if (!is_hangul(&str[len - 2]))
        return 0;

    high = KStbl[(high - 0xb0) * 94 + low - 0xa1] & 0x1f;

    if (high < 2)
        return 0;

    if (high > 28)
        return 0;

    return 1;
}
