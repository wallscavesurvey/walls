//----------------------------------------------------------
// Title     : RTF parsing and converting
// Programmer: Alexander V. Dolgov (2003)
// Copyright owned by: Ireneusz Zielinski, ireksoftware.com
// ul. Prszowicka 8a, 31-228 Krakow, Poland
// If you want to use this code in your application you have to buy it.
// Please visit http://www.ireksoftware.com for more info.
// If you have already bought this software you're free to use this source code in
// your application (free or comerrcial), you can modify this code.
// IT'S FORBIDDEN to sell this source code or release it to public.
// You can sell this product in compiled (binnary form).
// 
// Interface header
//----------------------------------------------------------

#if !defined(RTF_INTERFACE)
  #define RTF_INTERFACE

  #pragma warning(disable:4663) // STL compiling: C++ language change
  #pragma warning(disable:4511) // STL compiling: copy constructor could not be generated
  #pragma warning(disable:4512) // STL compiling: assignment operator could not be generated
  #pragma warning(disable:4786) // STL compiling: identifier was truncated in the debug information
  #pragma warning(disable:4018) // STL compiling: signed/unsigned mismatch

  #include <string>
  #include <sstream>
  #include <istream>
  #include <vector>

  namespace rtf
  {

  //#define SUPPRESS_REMINDER

  // Used for VisualStudio output logging on compile stage
  // To suppres logging define SUPPRESS_REMINDER
  // e.g. #pragma REMINDER( Hello, world! )
  #if !defined(SUPPRESS_REMINDER) && defined(_DEBUG)
    #define TXT0( X ) #X
    #define TXT1( X ) TXT0( X )
    #define REMINDER(X) message(__FILE__"("TXT1(__LINE__)") : "TXT1(X))
  #else
    #define REMINDER(X)
  #endif

  // Used for check speedup
  #define ISDIGIT(ch) ((ch)=='0' || (ch)=='1' || (ch)=='2' || (ch)=='3' || (ch)=='4' || \
                       (ch)=='5' || (ch)=='6' || (ch)=='7' || (ch)=='8' || (ch)=='9')

  // Global variable mostly used as return value
  extern const std::string EMPTY_STRING;

  //----------------------------------------------------------
  // RTF word
  // Hold parsed RTF word and its hierarchy
  // Line of development: hold separately RTF command and 
  //  its optional parameter
  //----------------------------------------------------------
  class Word {

    friend class Parser;

    public:
      Word( const char* sCommand );
      virtual ~Word();
    public:
      const std::string&  GetCommand() const;
      const Word*         GetNext() const;
      const Word*         GetChild() const;
    private:
      static void         Clean( Word* pWord );

    private:
      std::string m_sCommand;
	    Word*       m_pNext;
	    Word*       m_pChild;
  };

  //----------------------------------------------------------
  // RTF data parsing
  // Parsing RTF data to tree of RTF words
  // Line of development: separate RTF command and its optional 
  //  parameter on parsing stage
  //----------------------------------------------------------
  class Parser {

    // size of data block reading from the input stream 
    // (used for speedup stream reading)
    #define READ_BUF_LEN  50000
    // maximal length of one reading token
    #define WORD_BUFF_LEN 100

    public:
      Parser();
      virtual ~Parser(){}

    public:
      // Input:  STL stream with RTF data
      // Output: pointer to top word (needs to destroy after use)
      Word* ParseStream( std::istream& iRtfDataStream );

    private:
      void    Cleanup();
      // parse one-level words
      Word*   Parse( std::istream& iRtfDataStream );
      // parse current word
      size_t  ReadWord( std::istream& iRtfDataStream );
      // read current stream (or ungot) char
      char    ReadChar( std::istream& iRtfDataStream );

    private:
      std::vector<char> m_vecUngot;                   // stack of ungot chars
      char              m_szInputStr[WORD_BUFF_LEN];  // current parsed string
      char              m_sReadBuf[READ_BUF_LEN];     // stream reading is buffered by choosen size 
                                                      //  (size can be optimized by application)
      char              m_cLastReturnedChar;          // 
      int               m_nReadBufEnd;                // count of chars realy readen
      int               m_nReadBufIndex;              // current buffer position
  };

  //----------------------------------------------------------
  // RTF data converting parameters
  // Used to prevent interface changing including parameter 
  //  list expanding
  // Line of development: add member(s) to return in it from 
  //  comberter(s) RTF embedded image data
  //----------------------------------------------------------
  struct Params {
    bool bInline;

    Params( bool _bInline=false )
      : bInline(_bInline)
    {
    }
  };

  //----------------------------------------------------------
  // RTF data converting interface
  // Input RTF data, converting parameters and returns 
  //  converted data in string.
  //----------------------------------------------------------
  class Converter {

    public:
      virtual ~Converter() {}

    public:
      virtual const std::string& Convert( const char* szRtfData, Params& params );
      virtual const std::string& Convert( std::istream& iRtfDataStream, Params& params ) = 0;

    // members for successor use
    public:
      void OutputString( const char* szString );
      void OutputFormatString(const char* szFormat, ... );
      void ClearOutput();
      const std::string& GetOutput() const;
    private:
      std::string m_sOutput;
  };

  //----------------------------------------------------------
  // Returns successor of rtf::Convertor which can 
  //  convert RTF to HTML.
  // line of development: new written convertors must 
  //  declare its fabric methods in there (and implement in 
  //  own cpp-file).
  //----------------------------------------------------------
  extern Converter* CreateHtmlConverter();

  }

#endif