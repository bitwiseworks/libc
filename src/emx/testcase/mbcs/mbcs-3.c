#include <stdio.h>
#include <unidef.h>
#include <locale.h>
#include <wchar.h>

static int test( wint_t wc, const char *name, int expected )
{
    int rc = wcwidth( wc );
    const UNICTYPE *type = UniQueryCharType( (UniChar)wc );

    printf("%s: wcwidth( 0x%04X ) = %d(%d) %s\n",
           rc == expected ? "PASSED" : "FAILED",
           (unsigned)wc, rc, expected, name );

    if( type == NULL )
    {
        printf("  UniQueryCharType returned NULL\n");
        return 1;
    }

    printf("  UNICTYPE: itype=0x%04X bidi=0x%02X charset=0x%02X "
           "extend=0x%04X codepage=0x%04X\n",
           (unsigned)type->itype, (unsigned)type->bidi,
           (unsigned)type->charset, (unsigned)type->extend,
           (unsigned)type->codepage );
    printf("  flags: PRINT=%d CNTRL=%d NONSPACING=%d DIACRITIC=%d "
           "VOWELMARK=%d HALFWIDTH=%d FULLWIDTH=%d IDEOGRAPH=%d "
           "HIRAGANA=%d KATAKANA=%d\n",
           !!(type->itype & CT_PRINT), !!(type->itype & CT_CNTRL),
           !!(type->extend & C3_NONSPACING), !!(type->extend & C3_DIACRITIC),
           !!(type->extend & C3_VOWELMARK), !!(type->extend & C3_HALFWIDTH),
           !!(type->extend & C3_FULLWIDTH), !!(type->extend & C3_IDEOGRAPH),
           !!(type->extend & C3_HIRAGANA), !!(type->extend & C3_KATAKANA) );

    return rc != expected;
}

static int test_string( const wchar_t *s, size_t n, const char *name, int expected )
{
    int rc = wcswidth( s, n );

    printf("%s: wcswidth( %s, %lu ) = %d(%d)\n",
           rc == expected ? "PASSED" : "FAILED", name,
           (unsigned long)n, rc, expected );
    return rc != expected;
}

int main( void )
{
    static const struct
    {
        wint_t wc;
        const char *name;
        int expected;
    } cases[] = {
        { 0x0000, "NUL", 0 },
        { 0x0020, "SPACE", 1 },
        { 0x0041, "LATIN CAPITAL LETTER A", 1 },
        { 0x00E9, "LATIN SMALL LETTER E WITH ACUTE", 1 },
        { 0x00AD, "SOFT HYPHEN", 1 },
        { 0x00A1, "INVERTED EXCLAMATION MARK (AMBIGUOUS WIDTH)", 1 },
        { 0x0009, "CHARACTER TABULATION", -1 },
        { 0x000A, "LINE FEED", -1 },
        { 0x007F, "DELETE", -1 },
        { 0x0085, "NEXT LINE", -1 },
        { 0x0301, "COMBINING ACUTE ACCENT", 0 },
        { 0x05B0, "HEBREW POINT SHEVA", 0 },
        { 0x200B, "ZERO WIDTH SPACE", 0 },
        { 0x200C, "ZERO WIDTH NON-JOINER", 0 },
        { 0x200D, "ZERO WIDTH JOINER", 0 },
        { 0xFEFF, "ZERO WIDTH NO-BREAK SPACE", 0 },
        { 0xFE0F, "VARIATION SELECTOR-16", 0 },
        { 0x1160, "HANGUL JUNGSEONG FILLER", 0 },
        { 0x11A8, "HANGUL JONGSEONG KIYEOK", 0 },
        { 0xD7B0, "HANGUL JUNGSEONG O-YEO", 0 },
        { 0xD7CB, "HANGUL JONGSEONG NIEUN-RIEUL", 0 },
        { 0x302A, "IDEOGRAPHIC LEVEL TONE MARK (WIDE BUT NONSPACING)", 0 },
        { 0x02C7, "CARON (SPACING MODIFIER)", 1 },
        { 0x036F, "COMBINING LATIN SMALL LETTER X", 0 },
        { 0x0374, "GREEK NUMERAL SIGN", 1 },
        { 0xFF1A, "FULLWIDTH COLON", 2 },
        { 0x4E00, "CJK UNIFIED IDEOGRAPH-4E00", 2 },
        { 0xAC00, "HANGUL SYLLABLE GA", 2 },
        { 0x3042, "HIRAGANA LETTER A", 2 },
        { 0x30A2, "KATAKANA LETTER A", 2 },
        { 0xFF71, "HALFWIDTH KATAKANA LETTER A", 1 }
    };
    size_t i;
    int failed = 0;
    const char *locale;

    printf("Testing if wcwidth() works reasonably in UTF-8 locales...\n");
    locale = setlocale( LC_ALL, "fr_FR.UTF-8" );
    if( locale == NULL )
    {
        fprintf( stderr, "FAILED: cannot set fr_FR.UTF-8 locale\n" );
        return 1;
    }
    printf("Locale: %s\n", locale );

    for( i = 0; i < sizeof( cases ) / sizeof( cases[ 0 ] ); i++ )
        failed += test( cases[ i ].wc, cases[ i ].name, cases[ i ].expected );

    failed += test_string( L"", 1, "empty", 0 );
    failed += test_string( L"A\x0301", 2, "A + combining acute accent", 1 );
    failed += test_string( L"A\x200B\xFF1A", 3, "A + zero width space + fullwidth colon", 3 );
    failed += test_string( L"A\x200B\xFF1A", 2, "same string limited to first two characters", 1 );
    failed += test_string( L"A\n", 2, "A + line feed", -1 );
    failed += test_string( L"\x200B", 1, "zero width space", 0 );
    failed += test_string( L"\xFF1A", 1, "fullwidth colon", 2 );
    failed += test_string( L"\x1100\x1161\x11A8", 3, "Hangul leading + medial + final Jamo", 2 );
    failed += test_string( L"A", 0, "zero length limit", 0 );

    locale = setlocale( LC_ALL, "C" );
    if( locale == NULL )
    {
        fprintf( stderr, "FAILED: cannot set C locale\n" );
        return 1;
    }
    printf("Locale: %s (non-ASCII characters must remain nonprintable)\n", locale );
    failed += test( 0x0000, "NUL", 0 );
    failed += test( 0x0041, "LATIN CAPITAL LETTER A", 1 );
    failed += test( 0x000A, "LINE FEED", -1 );
    failed += test( 0x00E9, "LATIN SMALL LETTER E WITH ACUTE", -1 );
    failed += test( 0x0301, "COMBINING ACUTE ACCENT", -1 );
    failed += test( 0xFE0F, "VARIATION SELECTOR-16", -1 );
    failed += test( 0xFF1A, "FULLWIDTH COLON", -1 );
    failed += test_string( L"A\x0301", 2, "A + combining acute accent in C locale", -1 );

    printf("Failures: %d\n", failed );
    return !!failed;
}
