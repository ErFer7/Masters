#pragma once

/*
  Defines
*/

namespace Anagram {

#define anagram_DICTWORDS 2279
#define anagram_MASK_BITS 32 /* number of bits in a Quad */
#define anagram_MAX_QUADS 2  /* controls largest phrase */
#define anagram_MAXCAND 100  /* candidates */
#define anagram_MAXSOL 51    /* words in the solution */
#define anagram_ALPHABET 26  /* letters in the alphabet */

#define anagram_OneStep(i)                                                 \
    if ((aqNext[i] = pqMask[i] - pw->aqMask[i]) & anagram_aqMainSign[i]) { \
        ppwStart++;                                                        \
        continue;                                                          \
    }

static const int ANAGRAM_HEAP_SIZE = 21000;

/*
  Type definitions
*/

typedef unsigned int anagram_Quad; /* for building our bit mask */

/* A Word remembers the information about a candidate word. */
typedef struct {
    char *pchWord;                          /* the word itself */
    anagram_Quad aqMask[anagram_MAX_QUADS]; /* the word's mask */
    unsigned cchLength;                     /* letters in the word */
    char padding[4];
} anagram_Word;
typedef anagram_Word *anagram_PWord;
typedef anagram_Word **anagram_PPWord;

/* A Letter remembers information about each letter in the phrase to
   be anagrammed. */
typedef struct {
    unsigned uFrequency; /* how many times it appears */
    unsigned uShift;     /* how to mask */
    unsigned uBits;      /* the bit mask itself */
    unsigned iq;         /* which Quad to inspect? */
} anagram_Letter;
typedef anagram_Letter *anagram_PLetter;

class Anagram {
   public:
    Anagram();

    ~Anagram() { delete[] anagram_simulated_heap; }

    int run();

   private:
    int anagram_ch2i(int ch);
    int anagram_CompareFrequency(char *pch1, char *pch2);
    void anagram_AddWords(void);
    void anagram_BuildMask(char const *pchPhrase);
    void anagram_BuildWord(char *pchWord);
    void anagram_DumpWords(void);
    void anagram_FindAnagram(anagram_Quad *pqMask, anagram_PPWord ppwStart, int iLetter);
    anagram_PWord anagram_NewWord(void);
    anagram_PWord anagram_NextWord(void);
    void anagram_ReadDict(void);
    void anagram_Reset(void);
    void anagram_SortCandidates(void);

    void anagram_swapi(char *ii, char *ij, unsigned long es) {
        char *i, *j, c;

        i = (char *)ii;
        j = (char *)ij;
        do {
            c = *i;
            *i++ = *j;
            *j++ = c;
            es -= sizeof(char);
        } while (es != 0);
    }

    char *anagram_pivot(char *a, unsigned long n, unsigned long es) {
        unsigned long j;
        char *pi, *pj, *pk;

        j = n / 6 * es;
        pi = a + j; /* 1/6 */
        j += j;
        pj = pi + j; /* 1/2 */
        pk = pj + j; /* 5/6 */
        if (anagram_CompareFrequency(pi, pj) < 0) {
            if (anagram_CompareFrequency(pi, pk) < 0) {
                if (anagram_CompareFrequency(pj, pk) < 0) return pj;
                return pk;
            }
            return pi;
        }
        if (anagram_CompareFrequency(pj, pk) < 0) {
            if (anagram_CompareFrequency(pi, pk) < 0) return pi;
            return pk;
        }
        return pj;
    }

    void anagram_qsorts(char *a, unsigned long n, unsigned long es) {
        unsigned long j;
        char *pi, *pj, *pn;
        volatile unsigned int _ = 0;  // Was flowfactdummy
        while (n > 1) {
            if (n > 10)
                pi = anagram_pivot(a, n, es);
            else
                pi = a + (n >> 1) * es;

            anagram_swapi(a, pi, es);
            pi = a;
            pn = a + n * es;
            pj = pn;
            while (1) {
                /* wcc note: this assignment expression was added to avoid assignment of
                   multiple loop bound annotations to same loop (cf. Ticket #0002323). */
                _++;
                do {
                    pi += es;
                } while (pi < pn && anagram_CompareFrequency(pi, a) < 0);
                do {
                    pj -= es;
                } while (pj > a && anagram_CompareFrequency(pj, a) > 0);
                if (pj < pi) break;
                anagram_swapi(pi, pj, es);
            }
            anagram_swapi(a, pj, es);
            j = (unsigned long)(pj - a) / es;
            n = n - j - 1;
            if (j >= n) {
                anagram_qsorts(a, j, es);
                a += (j + 1) * es;
            } else {
                anagram_qsorts(a + (j + 1) * es, n, es);
                n = j;
            }
        }
    }

    void anagram_qsort(void *va, unsigned long n, unsigned long es) {
        anagram_qsorts((char *)va, n, es);
        ;
    }

    void *anagram_malloc(unsigned int numberOfBytes) {
        void *currentPos = (void *)&anagram_simulated_heap[anagram_freeHeapPos];
        /* Get a 4-byte address for alignment purposes */
        // anagram_freeHeapPos += ( ( numberOfBytes + 4 ) & ( unsigned int )0xfffffffc );
        unsigned int rem = (numberOfBytes & (unsigned int)0x3);
        unsigned int adjustment = rem ? 4 - rem : 0;
        anagram_freeHeapPos += numberOfBytes + adjustment;
        return currentPos;
    }

    void anagram_bzero(char *p, unsigned long len) {
        unsigned long i;

        for (i = 0; i < len; ++i) {
            *p++ = '\0';
        }
    }

    inline void reset() {
        anagram_freeHeapPos = 0;
        anagram_ReadDict();
    }

   private:
    static const char *anagram_achPhrase[3];
    static const char *anagram_dictionary[anagram_DICTWORDS];

    /* candidates we've found so far */
    anagram_PWord anagram_apwCand[anagram_MAXCAND];
    /* how many of them? */
    unsigned anagram_cpwCand;

    /* statistics on the current phrase */
    anagram_Letter anagram_alPhrase[anagram_ALPHABET];

    /* number of letters in phrase */
    int anagram_cchPhraseLength;

    /* the bit field for the full phrase */
    anagram_Quad anagram_aqMainMask[anagram_MAX_QUADS];
    /* where the sign bits are */
    anagram_Quad anagram_aqMainSign[anagram_MAX_QUADS];

    inline static const int anagram_cchMinLength = 3;

    /* auGlobalFrequency counts the number of times each letter appears,
       summed over all candidate words. This is used to decide which letter
       to attack first. */
    unsigned anagram_auGlobalFrequency[anagram_ALPHABET];
    int anagram_achByFrequency[anagram_ALPHABET]; /* for sorting */

    /* the dictionary is read here */
    char *anagram_pchDictionary;

    /* the answers */
    anagram_PWord anagram_apwSol[anagram_MAXSOL];
    int anagram_cpwLast;

    /* buffer to write an answer */
    char anagram_buffer[30];

    char *anagram_simulated_heap;
    unsigned int anagram_freeHeapPos;
};
}  // namespace Anagram
