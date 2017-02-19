#if defined(_MSC_VER) & defined(_DEBUG)
#include <pcutil/debug.h>
#define new DEBUG_NEW
#endif


#include "lzw.h"

#include <stdio.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>
#include <strstrea.h>

//      Implementation:

//  Algorithmus von Lempel, Ziv und Welch

//  Prinzip: Der Input wird mit Eintraegen in einem Kompressionswoerterbuch
//     verglichen. Der ausgegebene Code ist ein Pointer in das Woerterbuch.
//     Das Worterbuch besteht am Anfang nur aus den 256 Zeichen, danach
//     wird jeweils das beste (=laengste) Vergleichsergebnis mit dem naechsten Zeichen
//     im Woerterbuch eingetragen. Implementation des Worterbuchs mit einem
//     trie. Bei der Dekodierung wird das Woerterbuch erneut aufgebaut, muss also
//     nicht uebertragen werden. Wenn das Woerterbuch voll ist, gibt es drei
//     Methoden, darauf zu reagieren:
//  freeze: nach dem Aufbau des Woerterbuchs wird dieses nicht mehr veraendert
//  flush: wenn nach dem Aufbau des Woerterbuchs die Kompressionsrate zu sinken beginnt,
//      wird es geloescht und von neuem erzeugt
//  swap: nach dem vollstaendigen Aufbau eines Woerterbuchs wird sofort mit dem Aufbau eines
//      neuen begonnen; sobald das neue fertig ist, ersetzt es das alte



// typedef long int code_int;
typedef int code_int;
typedef long int count_int;


//     Deklarationen

// Hilfsfunktionen

#ifndef max
inline long max(long, long );
#endif

#ifndef min
inline long min(long, long );
#endif

int log2(int);


// Klassen instream, outstream und deren Ableitungen dienen der Abstraktion
//  von iostreams und nicht-nullterminierten Strings
//  notwendig, da stringstreams sich nur auf null-terminierte Strings beziehen,
//  aber andrerseits eine volle Ableitung von iostream und streambuf zu aufwendig
//  und noch dazu wenig portabel waere
//  nur die zur Implementation unbedingt noetigen Methoden werden implementiert
//  die Semanik entspricht exakt der der C++ ios-Klassen

class instream
// abstrakte Klasse analog zu istream
{
    public:
    virtual instream& get(unsigned char&) = 0;
    virtual operator int() = 0;
    virtual int eof() = 0;
    virtual instream& read(unsigned char* pch,int nCount) = 0;
    virtual int gcount() = 0;
    long in_count;  // Anzahl der ueberhaupt bereits eingelesenen Zeichen
    int gct;        //  Anzahl der bei der letzten Operation eingelesenen Zeichen
    instream();
    virtual ~instream(){};
};


class outstream
// abstrakte Klasse analog zu ostream
{
    public:
    virtual outstream& operator<<(unsigned char c) = 0;
    virtual operator int() = 0;
    virtual outstream& write(unsigned char* pch,int nCount) = 0;
    long bytes_out; // Anzahl der ueberhaupt bereits ausgegebenen Zeichen
    int gct;         //  Anzahl der bei der letzten Operation eingelesenen Zeichen
    outstream();
    virtual ~outstream(){};
};

class instream_stream : public instream
// Implementation von instream fuer istreams
{
    public:
    instream& get(unsigned char&);
    operator int();
    int eof();
    instream& read(unsigned char* pch,int nCount);
    int gcount();
    instream_stream(istream& i, long ibuflen);
    ~instream_stream();
    private:
    istream& in;
    unsigned char* buf;  //  Puffer fuer Input-Datei
    long buflen;         //   Laenge des Puffers
    unsigned char* bufpos;   //   Position im Puffer
};

class instream_string : public instream
//Implementation von instream fuer Strings
{
    public:
    instream& get(unsigned char&);
    operator int();
    int eof();
    instream& read(unsigned char* pch,int nCount);
    int gcount();
    instream_string(unsigned char* i, long il);
            // il muss die Laenge des Strings enthalten
    private:
    unsigned char* in;  // Input-String
    long ilen;      //  dessen Laenge
};


class outstream_stream : public outstream
//Implementation von outstream fuer ostreams
{
    public:
    outstream& operator<<(unsigned char c);
    operator int();
    outstream& write(unsigned char* pch,int nCount);
    outstream_stream(ostream& o, long obuflen);
    ~outstream_stream();
    private:
    ostream& out;
    unsigned char* buf; // Puffer fuer Output-Datei
    long buflen;        //  Laenge des Puffers
    unsigned char* bufpos;  //  Position im Puffer
};

class outstream_string : public outstream
//Implementation von outstream fuer Strings
{
    public:
    outstream& operator<<(unsigned char c);
    operator int();
    outstream& write(unsigned char* pch,int nCount);
    outstream_string(unsigned char* o, long ol);
        // ol muss die Laenge von o enthalten
    long size() const { return buflen_;}
    unsigned char* buf() const { return buf_;}
    private:
    void enlarge();

    unsigned char* buf_;
    unsigned char* out_;  // output-String
    long buflen_;        //  dessen Laenge
};


//  Anmerkung: im folgenden sind die Rueckgabewerte von Funktionen immer
//    Fehlerindikatoren ( 0 = o.k., 1 = Fehler), falls nicht anders festgelegt


// Buffer-Klassen dienen der šbersetzung von E/A-Bytes in Codes
//     (Codes sind i.a. laenger als 8 bits und ausserdem nicht aligned)

class buffer
{
    public:
        operator int(void);
         // zur Zustandueberpruefung, 1 = o.k.
    protected:
    buffer(unsigned short int num_bits);
    virtual ~buffer(void);
    code_int max_code(int bits);
         // berechnet max. Code bei bits bits
    int n_bits;
         // derzeitige Anzahl von bits/code
    int maxbits;
         // max. bits/code ((de)compress-Aufruf)
    code_int maxcode;
         // derzeitiger max. code (abh. von n_bits)
    code_int maxmaxcode;
         // darf niemals erzeugt werden
    static int init_bits;
         // anfaengliche Anzahl von bits/Code
    int offset;
         // Position des naechsten Codes im Inputbuffer (in bits)
    unsigned char* buf;
         // Inputpuffer
    static unsigned char lmask[9];
    static unsigned char rmask[9];
         // Masken zur Extraktion der Codes aus dem Buffer
};

inline code_int buffer::max_code(int bits)
// erzeugt den mit bits bits maximal erzeugbaren Code
{
    return ((code_int) 1 << (bits)) - 1;
}


class output_buffer : public buffer
{
    public:
    output_buffer(unsigned short int num_bits, outstream& o);
    int output(code_int code);
         // Ausgabe eines Codes an den Puffer
    int clear_buf(void);
         // gibt Puffer aus und loescht ihn
    int cleared(void);
         // clear_buf + zuruecksetzen in Anfangszustand
    int eof(void);
         // Ende der Ausgabe
    int size_increase(code_int f);
         // ueberprueft ob naechsthoehere Codegroesse
         // noetig ist, und trifft allenfalls noetige Massnahmen
        int setmax(code_int free_ent);
         // setzt Zustand fuer max. Codegroesse
    private:
    outstream& out;
};

inline int output_buffer::output(code_int code)
// schreibt code in den Puffer und gibt diesen gegebenenfalls aus
{
    register int r_off = offset;
    register int bits = n_bits;
    register unsigned char* bp = buf;


  // erste bits
    bp += (r_off >> 3);
    r_off &= 7;
    *bp = (*bp & rmask[r_off]) | (code << r_off) & lmask[r_off];
    bp++;

  // Mittelteil
    bits -= (8 - r_off);
    code >>= 8 - r_off;
    if ( bits >= 8 )
    {
       *bp++ = code;
       code >>= 8;
       bits -= 8;
    }
  // Letzte bits
    if(bits) *bp = code;

    offset += n_bits;
    if ( offset == (n_bits << 3) )    // Puffer voll
    {
    out.write(buf,n_bits);
    if (!out)
    {
       cerr << "lzw E014: problem with output stream" << endl;
       return 1;
    }
    offset = 0;
    }
    return 0;
}


class input_buffer : public buffer
{
    public:
    input_buffer(unsigned short int num_bits, instream& i);
    code_int getcode(void);
         // gibt den naechsten Code aus oder -1 bei eof
    int clear_buf();
         // liest einen Puffer voll ein,
         // ergibt -1 bei eof
    int cleared(void);
         // zuruecksetzen in den Anfangszustand + Einlesen eines neuen Puffers
         // ergibt -1 bei eof
    int size_increase(code_int f);
         // ueberprueft ob naechsthoehere Codegroesse
         // noetig ist, und trifft allenfalls noetige Massnahmen
    private:
    instream& in;
    int size;
         // eine Position des Buffers an der ein Bit des letzten Codes steht
         // dient zur Markierung des Pufferendes, schneller zu berechnen als das letzte bit
};


// Dictionary-Klassen dienen dem Aufbau der Woerterbuecher

class dict
// Klasse fuer Gemeinsamkeiten der Woerterbuecher
{
    public:
    code_int free_ent;
        // Index des ersten freien Eintrags
    static int clear;
        // Output Code: signalisiert Reinitialisierung des dict
    static unsigned long HashTabGroesse(unsigned short bits);
        // berechnet die Groesse einer Hashtabelle
    protected:
    dict(unsigned short int bits);
    int maxbits;
        // max. # bits/code
    code_int maxmaxcode;
        // darf niemals erzeugt werden
    code_int hsize;
        // Groesse der Hashtabelle
    static int first;
        // Index des ersten Eintrags
    int st;
        // 1 falls Hilfsspeicher von aussen uebergeben wurde
};


class dict_dec;

class dict_enc : public dict
// Woerterbuch fuer den Kodierungsvorgang
{
    public:
    dict_enc(unsigned short int bits, char* store);
    ~dict_enc(void);
    void cl_block(void);
        // zuruecksetzen in Anfangszustand
    void cl_hash(void);
        // loeschen der Hashtabelle
    int search(unsigned char c,code_int& ent);
        // sucht c mit vorhergehendem Code ent im dict und traegt ihn evtl. ein
        //  returns: -1: neuer Eintrag eingefuegt
        //        0: alles okay
        //        1: Fehler
        //       -2: kein Platz mehr
    int check_ratio(long in_bytes, long out_bytes);
        // berechnet die Kompressionsrate fuer flush-Verfahren
        //  ergibt 1 falls Rate verschlechtert, 0 sonst
    operator int(void);
        // zur Zustandsueberpruefung, 1 = o.k.
    friend void insert_enc_dec(dict_dec*, dict_enc*);
    friend int insert_dec_enc(dict_enc*, dict_dec*, code_int&);
    private:
    count_int* htab;
        // Hashtabelle, enthaelt Praefix + letztes Zeichen
    unsigned short* codetab;
        // Codetabelle, enthaelt die Codes (= Indizes im Woerterbuch)
    int hshift;
        // Schranke fuer die Hashcodeberechnung
    long int ratio;
        // Kompressionsrate  (flush)
    static int check_gap;
        // šberpruefungsintervall der Kompressionsrate (flush)
    count_int checkpoint;
        // naechster šberpruefungspunkt  (flush)
};


class dict_dec : public dict
{
    public:
    dict_dec(unsigned short int bits,outstream&, char* store);
    ~dict_dec(void);
    void clear_dict(void);
        // zuruecksetzen in Anfangszustand
    code_out(code_int code, code_int oldcode, int& finchar);
        // Ausgabe der dem Code entsprechenden Zeichen
    
        int new_entry(code_int oldcode,int finchar,input_buffer&);
        // Eintragung des Codes in das Dekodierungswoerterbuch
    operator int(void);
        // zur Zustandueberpruefung, 1 = o.k.
    friend void insert_enc_dec(dict_dec*, dict_enc*);
    friend int insert_dec_enc(dict_enc*, dict_dec*,code_int&);
    unsigned char* stacktop;
        // Anfang des Ausgabestacks
    private:
    outstream& out;
    unsigned char* suffixtab;
        // Tabelle der Suffixe
    unsigned short int* prefixtab;
        // Tabelle der Praefixe
    unsigned char* stack;
        // Ausgabestack
};


//  Funktionen zur Uebersetzung der Worterbuecher ineinander (swap)

void insert_enc_dec(dict_dec* to, dict_enc* from);

int insert_dec_enc(dict_enc* to, dict_dec* from, code_int& ent);

int lzw::decompress(unsigned char* in, long ilen, unsigned char*& out, long& olen, long& size,
             unsigned short int bits, method meth, char* store)
{
  // Ueberpruefung der Argumente, gegebenenfalls Speicherallozierung

    if (!in || ilen < 0)
    {
    cerr << "lzw E001: kein Input" << endl;
    return 1;
    };

    if (!out) {
     if (size > 0)
          out = new unsigned char[size];
     else {
          out = new unsigned char[ilen];
          size = ilen;
     }
    }
    else {
     // out
     if (size <= 0) {
          delete out;
          out = new unsigned char[ilen];
          size = ilen;
     }
    }

  // Umwandlung der Eingabe in Streams

    instream_string i(in,ilen);
    outstream_string o(out,size);

  // Implementationsaufruf

    register int r = decompress(i,o,bits,meth,store);

  // null-terminiert falls Platz vorhanden
    out = o.buf();
    olen = o.bytes_out;
    size = o.size();
    if (out && olen < size) out[olen] = '\0';
    return r;
}

int lzw::compress(unsigned char* in, long& ilen, unsigned char*& out, long& olen,
           unsigned short int bits, method meth, char* store)
{
  // Ueberpruefung der Argumente, gegebenenfalls Speicherallozierung

    if (!in)
    {
    cerr << "lzw E004: kein Input" << endl;
    return 1;
    };
    if (ilen < 0 ) ilen = strlen((char*)in);
    if (out == NULL)
    {
    out = new unsigned char[ilen+1];
    olen = ilen;
    }
    else if (olen < 0) olen = strlen((char*)out);
    if (!out)
    {
    cerr << "lzw E005: zu wenig Speicher" << endl;
    return 1;
    };

  // Umwandlung der Eingabe in Streams

    instream_string i(in,ilen);
    outstream_string o(out,olen);

  // Implementationsaufruf

    register int r = compress(i,o,bits,meth,store);

    out = o.buf();
    ilen = i.in_count;
    olen = o.bytes_out;
    return r;
}

int lzw::decompress(istream& in, long& ilen, long ibuflen, ostream& out, long& olen, long obuflen,
             unsigned short int bits, method meth, char* store)
{
  // Uberpruefung der Argumente

    if (!in)
    {
    cerr << "lzw E018: Problem mit Eingabestrom" << endl;
    return 1;
    }
    if (!out)
    {
    cerr << "lzw E018: Problem mit Ausgabestrom" << endl;
    return 1;
    }

  // Umwandlung der Eingabe in Streams

    instream_stream i(in,ibuflen);
    outstream_stream o(out,obuflen);

  // Implementationsaufruf

    register int r = decompress(i,o,bits,meth,store);

    ilen = i.in_count;
    olen = o.bytes_out;
    return r;
}

int lzw::compress(istream& in, long& ilen, long ibuflen, ostream& out, long& olen, long obuflen,
           unsigned short int bits, method meth, char* store)
{
  // Uberpruefung der Argumente

    if (!in)
    {
    cerr << "lzw E018: Problem mit Eingabestrom" << endl;
    return 1;
    }
    if (!out)
    {
    cerr << "lzw E018: Problem mit Ausgabestrom" << endl;
    return 1;
    }

  // Umwandlung der Eingabe in Streams

    instream_stream i(in,ibuflen);
    outstream_stream o(out,obuflen);

  // Implementationsaufruf

    register int r = compress(i,o,bits,meth,store);

    ilen = i.in_count;
    olen = o.bytes_out;
    return r;
}

int lzw::compress(const char* in, long& ilen, char*& out, long& olen,
           unsigned short int bits, method meth, char* store)
{
    return compress((unsigned char*)in,ilen,(unsigned char*&)out,olen,bits,meth,store);
}

int lzw::decompress(char* in, long ilen, char*& out, long& olen, long& size,
             unsigned short int bits, method meth, char* store)
{
    return decompress((unsigned char*)in,ilen,(unsigned char*&)out,olen,size,bits,meth,store);
}



int lzw::compress(unsigned char* in, long& ilen, ostream& out, long& olen, long obuflen,
                  unsigned short bits, method meth, char* store)
{
  // Ueberpruefung der Argumente, gegebenenfalls Speicherallozierung

    if (!out)
    {
    cerr << "lzw E018: Problem mit Ausgabestrom" << endl;
    return 1;
    }

    if (!in)
    {
    cerr << "lzw E004: kein Input" << endl;
    return 1;
    }
    if (ilen < 0 ) ilen = strlen((char*)in);

  // Umwandlung der Eingabe in Streams

    instream_string i(in,ilen);
    outstream_stream o(out,obuflen);

  // Implementationsaufruf

    register int r = compress(i,o,bits,meth,store);

    olen = o.bytes_out;
    ilen = i.in_count;
    return r;

}

int lzw::compress(char* in, long& ilen, ostream& out, long& olen, long obuflen,
                  unsigned short bits, method meth, char* store)
{
    return compress((unsigned char*) in, ilen, out, olen, obuflen, bits, meth, store);
}

int lzw::compress(istream& in, long& ilen, long ibuflen, unsigned char*& out, long& olen,
                  unsigned short bits, method meth, char* store)
{
  // Ueberpruefung der Argumente, gegebenenfalls Speicherallozierung

    if (out == NULL)
    {
    out = new unsigned char[olen];
    }
    else if (olen < 0) olen = strlen((char*)out);
    if (!out)
    {
    cerr << "lzw E005: zu wenig Speicher" << endl;
    return 1;
    }
    if (!in)
    {
    cerr << "lzw E018: Problem mit Eingabestrom" << endl;
    return 1;
    }

  // Umwandlung der Eingabe in Streams

    instream_stream i(in,ibuflen);
    outstream_string o(out,olen);

  // Implementationsaufruf

    register int r = compress(i,o,bits,meth,store);

    ilen = i.in_count;
    olen = o.bytes_out;
    return r;

}

int lzw::compress(istream& in, long& ilen, long ibuflen, char*& out, long& olen,
                  unsigned short bits, method meth, char* store)
{
    return compress(in, ilen, ibuflen, (unsigned char*&) out, olen, bits, meth, store);
}


int lzw::decompress(unsigned char* in, long& ilen, ostream& out, long& olen, long obuflen,
                  unsigned short bits, method meth, char* store)
{
  // Ueberpruefung der Argumente, gegebenenfalls Speicherallozierung

    if (!in || ilen < 0)
    {
    cerr << "lzw E001: kein Input" << endl;
    return 1;
    }
    if (!out)
    {
    cerr << "lzw E018: Problem mit Ausgabestrom" << endl;
    return 1;
    }

  // Umwandlung der Eingabe in Streams

    instream_string i(in,ilen);
    outstream_stream o(out,obuflen);

  // Implementationsaufruf

    register int r = decompress(i,o,bits,meth,store);

    olen = o.bytes_out;
    ilen = i.in_count;
    return r;

}

int lzw::decompress(char* in, long& ilen, ostream& out, long& olen, long obuflen,
                  unsigned short bits, method meth, char* store)
{
    return decompress((unsigned char*) in, ilen, out, olen, obuflen, bits, meth, store);
}

int lzw::decompress(istream& in, long& ilen, long ibuflen, unsigned char*& out, long& olen,
                  unsigned short bits, method meth, char* store)
{
  // Ueberpruefung der Argumente, gegebenenfalls Speicherallozierung

    if (olen > 0 && out == NULL) out = new unsigned char[olen+1];
    else
    {
    out = new unsigned char[2*ilen+1];   // testen wenn zu kurz!!
    olen = 2*ilen;
    }
    if (out != NULL && olen < 0) olen = strlen((char*)out);
    if (!out)
    {
    cerr << "lzw E002: zu wenig Speicher" << endl;
    return 1;
    }
    if (olen < 1)
    {
    cerr << "lzw E003: Output-String zu kurz" << endl;
    return 1;
    }
    if (!in)
    {
    cerr << "lzw E018: Problem mit Eingabestrom" << endl;
    return 1;
    }

  // Umwandlung der Eingabe in Streams

    instream_stream i(in,ibuflen);
    outstream_string o(out,olen);

  // Implementationsaufruf

    register int r = decompress(i,o,bits,meth,store);

  // null-terminiert falls Platz vorhanden

    if (out && o.bytes_out < olen) out[o.bytes_out] = '\0';
    olen = o.bytes_out;
    ilen = i.in_count;
    return r;

}

int lzw::decompress(istream& in, long& ilen, long ibuflen, char*& out, long& olen,
                  unsigned short bits, method meth, char* store)
{
    return decompress(in, ilen, ibuflen, (unsigned char*&) out, olen, bits, meth, store);
}



int lzw::compress(instream& in, outstream& out, unsigned short int bits, method meth, char* store)
{
  // Argumentueberpruefungen

    if (!in || !out)
    {
     cerr << "lzw E006: fehlerhafter Input" << endl;
     return 1;
    }

  // lokale Variablen

    register int i;
    register int j = 0;
      // Variablen fuer Rueckgabewerte der search-methoden

    register int swapping = 0;
      // swap: 1 wenn zweites Woerterbuch bereits initialisiert worden ist

    unsigned char c;
    in.get(c);
    if (!in)
    {
         cerr << "lzw E007: fehlerhafter Input" << endl;
         return 1;
    }

      // Eingabezeichen

    code_int ent = (code_int) c;
    code_int ent2;
      // Praefixe der beiden Worterbuecher

    register dict_enc* dict1, * dict2, * hpt;
    dict1 = new dict_enc(bits,store);
    if (meth == swap_method) dict2 = new dict_enc(bits,NULL);
    else dict2 = NULL;
      // Woerterbuecher

    output_buffer outbuf(bits,out);
      // Outputpuffer

    if (!dict1 || !outbuf || (meth == swap_method && !dict2))
    {
        cerr << "lzw E008: nicht genug Hauptspeicher" << endl;
        delete dict1;
            delete dict2;
            return 1;
    }

  // Hauptschleife: lesen, nachschlagen + bearbeiten
  //      Ende bei eof oder Fehler

    while (in.get(c)  && in.gcount() > 0)
    {

  // Suche des Zeichens in den Woerterbuechern

    i = dict1->search(c,ent);
    if (swapping) j = dict2->search(c,ent2);

  //  Behandlung der Suchergebnisse

    switch (i)
    {
        case  1 : return 1;
        // Fehler
        case  0 : break;
        // ist bereits im Woerterbuch
        case -1 :
        // nicht im Woerterbuch: wurde neu eingetragen
              if (outbuf.output ( (code_int) ent))
                      {
                           delete dict1;
                           delete dict2;
                           return 1;  
                      }     
                      if (outbuf.size_increase(dict1->free_ent-1))
            {
                           delete dict1;
                           delete dict2;
                           return 1;
            }
              ent = (code_int) c;
              break;
        case -2 :
        // kein Platz mehr
              if (outbuf.output ( (code_int) ent)) 
            {
                           delete dict1;
                           delete dict2;
                           return 1; 
            }
              if (outbuf.size_increase(dict1->free_ent))
            {
                           delete dict1;
                           delete dict2;
                           return 1;
            }
              ent = (code_int) c;
              if (meth == flush_method)
              {
             if (dict1->check_ratio(in.in_count,out.bytes_out))
             {
               // Loeschen des Woerterbuches falls Kompressionsrate zu gering

                if (outbuf.output((code_int) dict1->clear))
                {
                               delete dict1;
                               delete dict2;
                               return 1;
                            }
                dict1->cl_block();
                if (outbuf.cleared())
                {
                               delete dict1;
                               delete dict2;
                               return 1;
                            }
             }
              }
              else if (meth == swap_method && !swapping)
               {
                // Initialisieren des swap-Woerterbuches

                   if (outbuf.output((code_int) dict1->clear))
                   {
                                  delete dict1;
                                  delete dict2;
                                  return 1;
                   }
                   swapping = 1;
                   ent2 = (code_int) c;
               }
    }
    if (swapping) switch (j)
         // Ergebnisse der Suche im swap-Woerterbuch
              {
                  case  1 : return 1;
                // Fehler
                  case  0 : break;
                // ist vorhanden
                  case -1 :
                // wurde eingetragen
                    ent2 = (code_int) c;
                    break;
                  case -2 :
                // kein Platz mehr -> Vertauschung der Woerterbuecher
                    if (outbuf.output((code_int) ent))
                    {
                                            delete dict1;
                                            delete dict2;
                                            return 1;
                    }
                    if (outbuf.output((code_int) dict1->clear))
                        {
                                            delete dict1;
                                            delete dict2;
                                            return 1;
                    }  
                    if (outbuf.clear_buf())
                        {
                                            delete dict1;
                                            delete dict2;
                                            return 1;
                        }
                    if (outbuf.setmax(dict2->free_ent))
                        {
                                            delete dict1;
                                            delete dict2;
                                        }          
                    hpt = dict1;
                    dict1 = dict2;
                    dict2 = hpt;
                    dict2->cl_block();
                    in.get(c);
                    ent = (code_int) c;
                    ent2 = (code_int) c;
              }
    }

 // Abschluss
    if (outbuf.output(ent))
    {
       delete dict1;
       delete dict2;
       return 1;
    }
    if (outbuf.eof())
    {
       delete dict1;
       delete dict2;
       return 1;
    }
    delete dict1;
    delete dict2;
    return 0;
}



int lzw::decompress(instream& in, outstream& out, unsigned short int bits, method meth, char* store)
{

    register int swapping = 0;
      //  swap: 1 = zweites Woerterbuch ist initialisiert

    int finchar;
      //  erster char am stack d.h. erster char des letzten Codes
    register code_int code;
      //  code
    register code_int oldcode;
      //  letzter code
    code_int ent;
      //  swap: Praefix fuer Kodierungswoerterbuch

    register dict_dec* dict1;
      //  Dekodierungswoerterbuch
    register dict_enc* dict2;
      // swap: Kodierungswoerterbuch
    dict1 = new dict_dec(bits,out,store);
    if (meth == swap_method) dict2 = new dict_enc(bits,NULL);
    else dict2 = NULL;

    input_buffer inbuf(bits,in);

    if (!dict1 || !inbuf || (meth == swap_method && !dict2))
    {
        cerr << "lzw E009: nicht genug Hauptspeicher" << endl;
        delete dict1;
            delete dict2;
            return 1;
    }

    oldcode = inbuf.getcode();
    finchar = oldcode;
    if(oldcode == -1)   // eof
    {
    delete dict1;
    delete dict2;
    return 0;
    }
    out << (unsigned char)finchar;  // erster Code hat 8 bits
    if(!out)
      {
    cerr << "lzw E010: problem with output stream" << endl;
    delete dict1;
        delete dict2;
        return 1;
      }

  // Hauptschleife, Ende bei eof oder Fehler

    while ((code = inbuf.getcode()) > -1)
    {

    if (code == dict1->clear)
    {
       // Loeschen bzw. Reinitialisieren der Woerterbuecher

        if (swapping)
        {
        insert_enc_dec(dict1,dict2);
          //  bis hierher aufgebautes Kodierungswoerterbuch wird in ein
          //    Dekodierungswoerterbuch umgewandelt
        if(inbuf.clear_buf()) break;
        dict2->cl_block();
        ent = -1;
        oldcode = -1;
        finchar = -1;
        }
        else if (meth == swap_method)
        {
           // erstes Initialisieren des swap-Woerterbuchs

        swapping = 1;
        ent = -1;
        }
        else
        {
        dict1->clear_dict();
        if(inbuf.cleared()) break;
        }
    }
    else
    {
        if (dict1->code_out(code,oldcode,finchar))
              {
                 delete dict1;
                 delete dict2;
                 return 1;
          }
         // Code nachschlagen und Zeichen ausgeben
        if (dict1->new_entry(oldcode,finchar,inbuf)) break;
         // Code in Dekodierungswoerterbuch einfuegen
        if (swapping) if (insert_dec_enc(dict2,dict1,ent))
          {
                 delete dict1;
                 delete dict2;
                 return 1;
          }
         // Kodierungsworterbuch aufbauen
        oldcode = code;
    }
    }

    if (!out)
       {
    cerr << "lzw E011: trouble with output stream" << endl;
    delete dict1;
        delete dict2;
        return 1;
       }
    delete dict1;
    delete dict2;
    return 0;
}

char* lzw::temp_store_comp(unsigned short bits)
// alloziert Hilfsspeicher fuer compress
{
    register unsigned long hsize = dict::HashTabGroesse(bits);
    if (hsize ==  0) return NULL;
     else return new char[hsize*(sizeof(count_int)+sizeof(unsigned short))];
}

char* lzw::temp_store_decomp(unsigned short bits)
// alloziert Hilfsspeicher fuer decompress
{
    register unsigned long hsize = dict::HashTabGroesse(bits);
    if (hsize ==  0) return NULL;
     else return new char[hsize*(2*sizeof(unsigned char)+sizeof(unsigned short))];
}

char* lzw::temp_store(unsigned short bits)
// alloziert Hilfsspeicher fuer compress und decompress
{
    register unsigned long hsize = dict::HashTabGroesse(bits);
    if (hsize ==  0) return NULL;
     else return new char[max(hsize*(sizeof(count_int)+sizeof(unsigned short)),
                  hsize*(2*sizeof(unsigned char)+sizeof(unsigned short)))];
}



int buffer::init_bits = 9;
unsigned char buffer::lmask[9] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};
unsigned char buffer::rmask[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};

buffer::buffer(unsigned short int num_bits)
  : maxcode(max_code(n_bits = init_bits)),
    maxbits(num_bits),
    maxmaxcode((code_int) 1 << num_bits),
    offset(0),
    buf(new unsigned char[num_bits])
{
}

buffer::~buffer()
{
    delete[] buf;
}


buffer::operator int()
{
    if (!buf) return 0;
     else return 1;
}

output_buffer::output_buffer(unsigned short int num_bits, outstream& o)
  : buffer(num_bits),
    out(o)
{
}



int output_buffer::clear_buf()
{
    if ( offset > 0 )
    {
       out.write(buf, n_bits);
       if (!out)
      {
          cerr << "lzw E015: problem with output stream" << endl;
          return 1;
      }
       offset = 0;
    }
    return 0;
}

int output_buffer::cleared()
{
    if (clear_buf()) return 1;
    maxcode = max_code (n_bits = init_bits);
    return 0;
}

int output_buffer::eof()
{
    if ( offset > 0 )
    out.write( buf, (offset + 7) / 8);
    if(!out)
    {
       cerr << "lzw E016: trouble with output stream" << endl;
       return 1;
    }
    return 0;
}

int output_buffer::size_increase(code_int free_ent)
// falls Code zu gross geworden ist, muss er verlaengert werden
{
    register int ret = 0;
    if (free_ent > maxcode)
    {
    if (clear_buf())
      {
            ret = 1;
            goto raus;
      }
    n_bits++;
    if ( n_bits == maxbits )
        maxcode = maxmaxcode;
    else
        maxcode = max_code(n_bits);
    }
  raus:   // aus Optimierungsgruenden
    return ret;
}

int output_buffer::setmax(code_int free_ent)
// setzt maximale Codegroesse
{
    n_bits = log2(free_ent);
    if (n_bits < 0) return 1;
    if (n_bits == maxbits)  maxcode = maxmaxcode;
    else maxcode = max_code(n_bits);
    return 0;
}


input_buffer::input_buffer(unsigned short int num_bits, instream& i)
  : buffer(num_bits),
    in(i),
    size(0)
{
}

int input_buffer::clear_buf()
{
    if (!in || in.eof()) return -1;
    else
    {
    in.read(buf,n_bits);
    size = in.gcount();
    if (size == 0) return -1;    // eof
    }
    offset = 0;
    size = (size << 3) - (n_bits - 1);
      // size zeigt auf ein bit des letzten Codes
    return 0;
}

int input_buffer::cleared()
{
    maxcode = max_code (n_bits = init_bits);
    return clear_buf();
}

int input_buffer::size_increase(code_int free_ent)
// Anpassungen, falls Code groesser geworden ist
{
    register int ret = 0;
    if ( free_ent > maxcode && free_ent < maxmaxcode)
    {
    n_bits++;
    if ( n_bits == maxbits )
        maxcode = maxmaxcode;
    else
        maxcode = max_code(n_bits);
    if (clear_buf()){ret = -1; goto raus;}
    }

  raus:
    return ret;
}

code_int input_buffer::getcode()
// extrahiert einen Code aus dem Puffer und liest gegebenenfalls einen
//  neuen Puffer ein, ergibt -1 falls eof
{
    register code_int code = -1;
    register int r_off, bits;
    register unsigned char *bp = buf;

    if (offset >= size && clear_buf()) goto raus;

    r_off = offset;
    bits = n_bits;

  // erstes Byte
    bp += (r_off >> 3);
    r_off &= 7;

  // erster Teil (low order bits)
    code = (*bp++ >> r_off);
  // Mittelteil
    bits -= (8 - r_off);
    r_off = 8 - r_off;
    if ( bits >= 8 )
    {
       code |= *bp++ << r_off;
       r_off += 8;
       bits -= 8;
    }
  // letzter Teil  (high order bits)
    code |= (*bp & rmask[bits]) << r_off;

    offset += n_bits;

  raus:
    return code;
}





// Hilfsfunktionen


#ifndef max
inline long max(long a, long b) { return (a > b) ? a : b; }
#endif

#ifndef min
inline long min(long a, long b) { return (a < b) ? a : b; }
#endif

int log2(int x)
{
    if (x <= 0)
    {
    cerr << "lzw E017: Logarithmus einer nichtpositiven Zahl!" << endl;
    return -13;
    }
    register int r = -1;
    do
    {
    x >>= 1;
    r++;
    }
    while(x);
    return r;
}


// instream, outstream und Ableitungen


instream::instream()
  : in_count(0),
    gct(0)
{
}

instream_stream::instream_stream(istream& i, long ibuflen)
  : in(i),
    buf(new unsigned char[ibuflen]),
    bufpos(buf),
    buflen(ibuflen)
{
    in.read(buf,buflen);
}

inline instream_stream::operator int()
{
    if (in.bad() || !buf) return 0;
     else return 1;
}

inline instream& instream_stream::get(unsigned char& c)
{
    if (bufpos < buf + buflen)
    {
    c = *bufpos++;
    gct = 1;
    in_count++;
    }
    else
    {
    if (in && !in.eof()) 
        {
          in.read(buf,buflen);
      buflen = in.gcount();
      bufpos = buf;
      c = *bufpos++;
      if (buflen > 0) 
          {
           gct = 1;
       in_count++;
          } else gct = 0;  
    }
    else  gct = 0;
    }
    return *this;
}

inline int instream_stream::eof()
{
    return (in.eof() && bufpos == buf + buflen);
}

inline instream& instream_stream::read(unsigned char* pch, int nCount)
{
    unsigned int buf1 = (buf + buflen) - bufpos;
          // Anzahl der freien Stellen im Puffer
    if (buf1 >= nCount)
    // noch Platz
    {
    memcpy(pch,bufpos,nCount);
    gct = nCount;
    bufpos += nCount;
    }
    else
    {
    unsigned int buf2 = nCount - buf1;
    if (buf1 > 0) memcpy(pch,bufpos,buf1);
    if (buf2 <= buflen)
    {
       if (in)
           {
               in.read(buf, buflen);
           buflen = in.gcount();
            }
            else buflen = 0; 
        buf2 = min(buf2, buflen);
        if (buf2 > 0) memcpy(pch + buf1, buf, buf2);
        bufpos = buf + buf2;
        gct = buf1 + buf2;
    }
    else
    {
        if(in) 
            {
             in.read(pch+buf1,buf2);
         buf2 = in.gcount();
        }
            else buf2 = 0;
        if (in) 
            {
             in.read(buf,buflen);
         buflen = in.gcount();
        }else buflen = 0;
        bufpos = buf;
        gct = buf1 + buf2;
    }
    }
    in_count += gct;
    return *this;
}

inline int instream_stream::gcount()
{
    return gct;
}

instream_stream::~instream_stream()
{
    delete[] buf;
}

instream_string::instream_string(unsigned char* i, long il)
  : in(i),
    ilen(il)
{
}

inline instream_string::operator int()
{
    if (in && ilen >= 0) return 1;
     else return 0;
}

inline instream& instream_string::get(unsigned char& c)
{
    if (in && ilen > 0)
    {
    c = *in++;
    gct = 1;
    ilen--;
    in_count++;
    }
    else
    {
    gct = 0;
    ilen--;
    }
    return *this;
}

inline int instream_string::eof()
{
    return ilen < 0;
}

inline instream& instream_string::read(unsigned char* pch, int nCount)
{
    register int i;
    gct = 0;
    if (in && pch)
    for(i = 0; (i < nCount) && (ilen > 0); i++)
    {
    *pch++ = *in++;
    ilen--;
    gct++;
    }
    if (i < nCount && ilen == 0) ilen--;
    in_count += gct;
    return *this;
}

inline int instream_string::gcount()
{
    return gct;
}

outstream::outstream()
  : bytes_out(0),
    gct(0)
{
}

outstream_stream::outstream_stream(ostream& o, long obuflen)
  : out(o),
    buf(new unsigned char[obuflen]),
    bufpos(buf),
    buflen(obuflen)
{
}

inline outstream_stream::operator int()
{
    if (buf && (void*)out) return 1;
     else return 0;
}

inline outstream& outstream_stream::operator<<(unsigned char c)
{
    if (bufpos < buf + buflen) *bufpos++ = c;
    else
    {
    out.write(buf,buflen);
    bufpos = buf;
    *bufpos++ = c;
    }
    bytes_out++;

    gct = 1;
    return *this;
}

inline outstream& outstream_stream::write(unsigned char* pch, int nCount)
{
    unsigned int buf1 = (buf + buflen) - bufpos;

    if (buf1 >= nCount)
    {
    memcpy(bufpos,pch,nCount);
    bufpos += nCount;
    }
    else
    {
    unsigned int buf2 = nCount - buf1;

    if (buf1 > 0) memcpy(bufpos,pch,buf1);
    if (buf2 <= buflen)
    {
        out.write(buf, buflen);
        memcpy(buf, pch + buf1, buf2);
        bufpos = buf + buf2;
    }
    else
    {
        out.write(buf,buflen);
        out.write(pch+buf1,buf2);
        bufpos = buf;
    }
    }
    gct = nCount;
    bytes_out += gct;
    return *this;
}

outstream_stream::~outstream_stream()
{
    unsigned int n = bufpos - buf;

    if (n > 0) out.write(buf,n);
    delete[] buf;
}

outstream_string::outstream_string(unsigned char* o, long ol)
  : buf_(o),
    buflen_(ol - 1),        // for terminating null
    out_(o)
{
     if (!buf_ || buflen_ < 1) {
      delete buf_;
      buf_ = new unsigned char[2];
      out_ = buf_;
      buflen_ = 1;
     }
}

inline outstream_string::operator int()
{
    if (out_ && bytes_out <= buflen_) return 1;
     else return 0;
}

inline outstream& outstream_string::operator<<(unsigned char c)
{
     if (bytes_out == buflen_)
      enlarge();

     if (out_ && bytes_out < buflen_)
     {
      *out_++ = c;
      gct = 1;
      bytes_out++;
     }
     else gct = 0;
     return *this;
}

inline outstream& outstream_string::write(unsigned char* pch, int nCount)
{
     while (bytes_out + nCount > buflen_)
      enlarge();
     register int i;
     for(i = 0; i < nCount && out_ && pch && bytes_out < buflen_; i++)
     {
      *out_++ = *pch++;
      bytes_out++;
     }
     gct = i;
     return *this;
}

void outstream_string::enlarge() {
     unsigned char* newbuf = new unsigned char[ buflen_*2 + 1];
     ::memcpy( newbuf, buf_, buflen_);
     out_ = newbuf + (out_ - buf_);
     delete buf_;
     buf_ = newbuf;
     buflen_ = buflen_*2;
}

// Dictionaries

int dict::first = 257;
int dict::clear = 256;


dict::dict(unsigned short int bits)
  : free_ent(first),
    maxbits(bits),
    maxmaxcode((code_int)1 << bits),
    st(0),
    hsize((code_int) HashTabGroesse(bits))
{
}

unsigned long dict::HashTabGroesse(unsigned short bits)
{
    unsigned long hs = 0;
    if (bits == 16) hs = 69001;
     else if (bits == 15) hs = 35023;
      else if (bits == 14) hs = 18013;
       else if (bits == 13) hs = 9001;
    else if (bits <= 12) hs = 5003;
     else
     {
        cerr << "lzw E013: dictionary size not supported" << endl;
     }
    return hs;
}

int dict_enc::check_gap = 10000;

dict_enc::dict_enc(unsigned short int bits, char* store)
  : dict(bits)
{
    htab = NULL;
    codetab = NULL;

  if (hsize > 0)
  {
    ratio = 0;
    checkpoint = 10000;

    hshift = 0;
    for (long f = (long) hsize;  f < 65536L; f *= 2L )
    hshift++;
    hshift = 8 - hshift;

    if (store)
    {
    htab = (count_int*) store;
    codetab = (unsigned short*) (htab+hsize);
    st = 1;
    }
    else
    {
    htab = new count_int[hsize];
    codetab = new unsigned short[hsize];
    }
    if (htab) cl_hash();
  }
}

dict_enc::~dict_enc()
{
    if (!st)
    {
    delete[] htab;
    delete[] codetab;
    }
}

dict_enc::operator int()
{
    if (!htab || !codetab) return 0;
    else return 1;
}


int dict_enc::check_ratio(long in_count, long bytes_out)
// ueberprueft Kompressionsrate (multiplikationsfrei)
{
   register int ret = 0;
   register long int rat;

   if(bytes_out <= 0 || in_count <= 0) goto raus;
   if (in_count >= checkpoint)
   {
    checkpoint = in_count + check_gap;
    if(in_count > 0x007fffff)
    {
    rat = bytes_out >> 8;
    if(rat == 0)
    {
        rat = 0x7fffffff;
    }
    else
    {
        rat = in_count / rat;
    }
    }
    else
    {
    rat = (in_count << 8) / bytes_out;
    }

    if ( rat > ratio )
    {
    ratio = rat;
    }
    else  ret = 1;
   }

 raus:
   return ret;
}

void dict_enc::cl_block()
// Reinitialisierung
{
    ratio = 0;
    cl_hash ();
    free_ent = first;
}

void dict_enc::cl_hash()
// loescht Hashtabelle
{
    register count_int* hp;
    register count_int* ende = htab + hsize;
    for(hp = htab; hp < ende; hp++) *hp = -1;
}

int dict_enc::search(unsigned char c, code_int& ent)
// sucht Code mit Zeichen c und Praefix ent
//   returns: -1: neuer Eintrag eingefuegt
//         0: alles okay
//         1: Fehler
//        -2: kein Platz mehr
{
    register code_int hashval;
    register long fcode;
    register code_int disp;

    fcode = (long) (((long) c << maxbits) + ent);  // gesuchtes Wort in der Hashtabelle
    hashval = (((code_int)c << hshift) ^ ent);   // Hashfunktion: xor

    if ( htab[hashval] == fcode )   // gleich gefunden
    {
    ent = codetab[hashval];
    return 0;
    }
    else
    if ( (long)htab[hashval] >= 0 )    // Eintrag leer
    {
    disp = hsize - hashval;        // zweites Hashing nach G.Knott (vgl. Knuth)
    if ( hashval == 0 ) disp = 1;
    do
    {
        if ( (hashval -= disp) < 0 ) hashval += hsize;
        if ( htab[hashval] == fcode )   // gefunden
        {
        ent = codetab[hashval];
        return 0;
        }
    } while ((long) htab[hashval] > 0);  // solange Eintrag nicht leer
    }

  // nicht vorhanden

    if ( free_ent < maxmaxcode )    // neuer Eintrag
    {
    codetab[hashval] = free_ent;
    htab[hashval] = fcode;
    free_ent++;
    return -1;
    }
    else return -2;    // Woerterbuch voll
}


dict_dec::dict_dec(unsigned short int bits, outstream& o, char* store)
  : dict(bits),
    out(o)
{
    suffixtab = NULL;
    prefixtab = NULL;
  if (hsize > 0)
  {
    if (store)
    {
    suffixtab = (unsigned char*) store;
    stack =  suffixtab+hsize;
    prefixtab = (unsigned short*) (suffixtab + 2*hsize);
    st = 1;
    }
    else
    {
    suffixtab = new unsigned char[hsize];
    prefixtab = new unsigned short[hsize];
    stack = new unsigned char[hsize];
    }
    stacktop = stack;

  // Initialisierung der ersten 256 Zeichen
    for (register int c = 255; c >= 0; c-- )
    {
    prefixtab[c] = 0;
    suffixtab[c] = (unsigned char) c;
    }

  }
}

dict_dec::~dict_dec()
{
    if(!st)
    {
    delete[] prefixtab;
    delete[] suffixtab;
    delete[] stack;
    }
}


dict_dec::operator int()
{
    if (!prefixtab || !suffixtab || !stack) return 0;
    else return 1;
}

void dict_dec::clear_dict()
// Reinitialsierung
{
    for (register int c = 255; c >= 0; c-- )
    {
    prefixtab[c] = 0;
    }
    free_ent = first - 1;
}


int dict_dec::new_entry(code_int oldcode, int finchar, input_buffer& inbuf)
{
    register int ret = 0;
    if (free_ent < maxmaxcode && oldcode > -1)
    {
    prefixtab[free_ent] = (unsigned short) oldcode;
    suffixtab[free_ent] = finchar;
    free_ent++;
    if(inbuf.size_increase(free_ent)) ret = -1;
    }
    return ret;
}

int dict_dec::code_out(code_int code, code_int oldcode, int& finchar)
{
    register unsigned char* stackp = stack;

  // Spezialfall: KwKwK
    if ( oldcode > -1 && code >= free_ent )
    {
    *stackp++ = finchar;
    code = oldcode;
    }

  // Outputzeichen in umgekehrter Reihenfolge -> Stack
    while ( code >= 256 )
    {
    *stackp++ = suffixtab[code];
    code = prefixtab[code];
    }
    *stackp++ = finchar = suffixtab[code];
    stacktop = stackp;

  // Ausgabe von Stack
    do
    {
       out << *--stackp;
    }
    while ( stackp > stack);
    if (!out)
    {
    cerr << "lzw E012: problem with output stream" << endl;
    return 1;
    }
    return 0;
}


void insert_enc_dec(dict_dec* to, dict_enc* from)
// Umwandlung eines Kodierungs- in ein Dekodierungswoerterbuch
{
  // Initialisierung des Dekodierungswoerterbuchs
    register int c;
    for ( c = 255; c >= 0; c-- )
    {
    to->prefixtab[c] = 0;
    }
    to->free_ent = 0;

  // Einfuegen
    for (long i = 0; i < from->hsize; i++)
    {
    if (from->htab[i] >= 0)
    {
        unsigned short pos = from->codetab[i];
        unsigned char suff = from->htab[i] >> from->maxbits;
        to->suffixtab[pos] = suff;
        to->prefixtab[pos] = from->htab[i] - ((unsigned long)suff << from->maxbits);
        if (pos > to->free_ent) to->free_ent = pos;
    }
    }
    to->free_ent++;
}




int insert_dec_enc(dict_enc* to, dict_dec* from, code_int& ent)
// Umwandeln eines Dekodierungs- in ein Kodierungswoerterbuch
{
    register unsigned char* stp = from->stacktop;
    register int i;

  // Zeichenfolge wird vom stack gelesen, sonst wie in compress eingefuegt

    do
    {
    if (ent == -1) ent = *--stp;
    else
    {
    i = to->search(*--stp,ent);
    switch (i)
    {
        case  1 : return 1;
        case  0 : break;
        case -1 :
              ent = (code_int) *stp;
              break;
        case -2 :
              ent = (code_int) *stp;
              break;
    }
    }
    }
    while (stp > from->stack);
    return 0;
}
