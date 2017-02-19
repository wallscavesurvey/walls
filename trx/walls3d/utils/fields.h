// This may look like C code, but it is really -*- C++ -*-

//<copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
//</copyright>

//<file>
//
// Name:       fields.h
//
// Purpose:    Generic dynamic field of data
//
// Created:     1 Feb 92    Gerald Pani
//
// Modified:   21 Jul 93    Gerald Pani
//
//
//
// Description:
//
// Macros for creating fields of abstract datatypes. The array is 
// dynamic in storage allocation.
//
//</file>

#ifndef hg_utils_fields_h
#define hg_utils_fields_h

#include <iostream>
#include "../generic.h"
#include "types.h"

//<class>
//
// Name:       Field
//
// Purpose:    Macro for dynamic fields of type Data
//
//
//
// Public Interface:
//
// - Field()
//   Default constructor. Sets up empty Field with length 0.
//
// - Field( int c, Data* i, int s = 0)
//   Constructor. Sets up Field with length s (or default c) from
//   datafield i within c elements. The STORAGE ALLOCATED by datafield
//   i is NOW CONTROLLED by the FIELD. 
//
// - Field( int s)
//   Constructor. Sets up empty Field with length s.
//
// - Field( const Field& f)
//   Constructor. Sets up Field with data from f.
//
// - Field( const Field& f, int begin, int count)
//   Constructor. Sets up Field with count elements of f beginning at
//   position begin. 
//
// - Field( const Data& d)
//   Constructor. Sets up Field with length 1 and first element d.
//
// - Field( const Data& d1, const Data& d2)
//   Constructor. Sets up Field with length 2 and elements d1 and d2.
//
// - ~Field()
//   Destructor. Deletes ALL storage used by the field itself and
//   calls the destructor of all elements.
//
// - void init( int c, Data* i, int s = 0)
//   Free old data and reinitialize the field. The STORAGE ALLOCATED
//   by datafield i is NOW CONTROLLED by the FIELD.
//
// - void free()
//   Deletes ALL storage used by the field itself and calls the
//   destructor of all elements. 
//
// - boolean insert( int pos, const Data& data)
//   Inserts data into the field at position pos. If pos is not valid,
//   return false.
//
// - void append( const Data& data)
//   Appends data at the end of the field.
//
// - boolean remove( int pos)
//   Remove data from position pos. If pos is not valid, return false. 
//
// - int count() const
//   Returns the number of elements.
//
// - const Data* data() const
//   Returns a pointer to the internal data array.
//
// - const Data& operator []( int i) const
//   Returns a reference to the i-th element.
//
// - Field& operator =( const Field& f)
//   Free any old data if f not equal this and copy the data of f.
//   Returns a reference to the result (this).
//
// - Field& operator +=( const Field& f)
//   Append f to this returning this.
//
// - boolean operator ==( const Field& f)
//   True, if both fields are equal. Otherwise false.
//
// - boolean operator <( const Field& f)
//   True, if this is less than f (like string comparison).
//
// - Field& getpart( const Field& a, int b, int c)
//   Free any old data and reinitialize the field with the c elements
//   of a, beginning at position b. Return a reference to this.
//
// - friend ostream& operator <<( ostream& s, const Field& f)
//   Writes the field f to the ostream s by calling the operator <<
//   for each element. Elements are separated by space and after the
//   last item a newline is printed.
//   
// 
// Protected Interface:
//
// - void enlarge()
//   Enlarges the allocated storage.
//
// - void init( const Field& f, int begin, int count)
//   Reinitialize the field with count elements of f beginning at begin.
//
//
// Description:
//
// To create an field of class Data use Fieldsdeclare(FieldName, Data)
// in your header file and Fieldsimplement(FieldName, Data) in your
// corresponding C++-file.
//
// The class Data must have:
// * a default constructor
// * a destructor
// * an operator =
// * an operator <
// * an operator <<
// 
//</class>

#define Fieldsdeclare(Field, Data)                                            \
class Field {                                                                 \
  public:                                                                     \
     Field();                                                                 \
     Field( const Data&);                                                     \
     Field( const Data&, const Data&);                                        \
     Field( int c, Data* i, int s = 0);                                       \
     Field( int s);                                                           \
     Field( const Field&);                                                    \
     Field( const Field&, int, int);                                          \
     ~Field();                                                                \
     void               init( int c, Data* i, int s = 0);                     \
     void               free();                                               \
     boolean            insert( int pos, const Data&);                        \
     void               append( const Data&);                                 \
     boolean            remove( int pos);                                     \
     int                count() const;                                        \
     const Data*        data() const;                                         \
     const Data&        operator []( int) const;                              \
     Field&     operator =( const Field&);                                    \
     Field&     operator +=( const Field&);                                   \
     boolean            operator ==( const Field&) const;                     \
     boolean            operator <( const Field&) const;                      \
     Field&     getpart( const Field&, int, int);                             \
                                                                              \
     friend ostream& operator << ( ostream& s, const Field&);                 \
                                                                              \
  protected:                                                                  \
     void       enlarge();                                                    \
     void       init( const Field&, int, int);                                \
                                                                              \
     int        count_;                                                       \
     int        size_;                                                        \
     Data*      data_;                                                        \
};                                                                            \
                                                                              \
inline const Data& Field::operator []( int i) const {                         \
     return data_[i];                                                         \
}                                                                             \
                                                                              \
inline int Field::count() const {                                             \
     return count_;                                                           \
}                                                                             \
                                                                              \
inline const Data* Field::data() const {                                      \
     return data_;                                                            \
}                                                                             \
                                                                              \
inline Field::Field() : count_( 0), data_( nil), size_( 0) {                  \
}                                                                             \
                                                                              \
inline Field::Field( const Data& d) : count_( 1), data_( new Data[1]), size_( 1) {  \
     data_[0] = d;                                                            \
}                                                                             \
                                                                              \
inline Field::Field( const Data& d1, const Data& d2) : count_( 2), data_( new Data[2]), size_( 2) {  \
     data_[0] = d1;                                                           \
     data_[1] = d2;                                                           \
}                                                                             \
                                                                              \
inline Field::Field( const Field& f) : count_( 0), data_( nil), size_( 0) {   \
     operator=(f);                                                            \
}                                                                             \
                                                                              \
inline Field::Field( int c, Data* i, int s) : count_( c), data_( i) {         \
     if (s <= c)                                                              \
          size_ = count_;                                                     \
     else                                                                     \
          size_ = s;                                                          \
}                                                                             \
                                                                              \
inline Field::Field( int s) : count_( 0), size_( s) {                         \
    data_ = s ? new Data[s] : nil;                                            \
}

#define Fieldsimplement(Field, Data)                                          \
boolean Field::insert( int pos, const Data& data) {                           \
     if (pos < 0 || pos > count_)                                             \
          return false;                                                       \
     if (count_ == size_)                                                     \
          enlarge();                                                          \
     for (int i = count_; i > pos; i--) {                                     \
		  data_[i] = data_[i-1];											  \
	 }																		  \
     data_[pos] = data;                                                       \
     count_++;                                                                \
     return true;                                                             \
}                                                                             \
                                                                              \
void Field::append( const Data& data) {                                       \
     if (count_ == size_)                                                     \
          enlarge();                                                          \
     data_[count_++] = data;                                                  \
}                                                                             \
                                                                              \
boolean Field::remove( int pos) {                                             \
     if (pos < 0 || pos >= count_)                                            \
          return false;                                                       \
     count_--;                                                                \
     for (int i = pos; i < count_; i++) {                                     \
          data_[i] = data_[i+1];                                              \
     }                                                                        \
     return true;                                                             \
}                                                                             \
                                                                              \
void Field::free() {                                                          \
     count_ = size_ = 0;                                                      \
     if (!data_)                                                              \
          return;                                                             \
     delete [] data_;                                                         \
     data_ = nil;                                                             \
}                                                                             \
                                                                              \
void Field::init( int c, Data* i, int s) {                                    \
     free();                                                                  \
     count_ = c;                                                              \
     if (s < count_)                                                          \
          size_ = count_;                                                     \
     else                                                                     \
          size_ = s;                                                          \
     data_ = size_ ? i : nil;                                                 \
}                                                                             \
                                                                              \
Field::~Field() {                                                             \
     free();                                                                  \
}                                                                             \
                                                                              \
void Field::enlarge() {                                                       \
     Data* olddata = data_;                                                   \
     if (size_ == 0)                                                          \
          size_ = 1;                                                          \
     else                                                                     \
          size_ *= 2;                                                         \
     data_ = new Data [size_];                                                \
     Data* p = olddata, * q = data_;                                          \
     for(int i = count_; i; i--)                                              \
          *q++ = *p++;                                                        \
     delete [] olddata;                                                       \
}                                                                             \
                                                                              \
Field& Field::operator =( const Field& f) {                                   \
     free();                                                                  \
     if (!f.count())                                                          \
          return *this;                                                       \
     data_ = new Data [f.count()];                                            \
     if (!data_)                                                              \
          return *this;                                                       \
     count_ = size_ = f.count();                                              \
     for (int i = 0; i < count_; i++ ) {                                      \
          data_[i] = f[i];                                                    \
     }                                                                        \
     return *this;                                                            \
}                                                                             \
                                                                              \
Field& Field::getpart( const Field& f, int begin, int count) {                \
     free();                                                                  \
     init( f, begin, count);                                                  \
     return *this;                                                            \
}                                                                             \
                                                                              \
Field::Field( const Field& f, int begin, int count)							  \
 : count_( 0), data_( nil), size_( 0) {										  \
     init( f, begin, count);                                                  \
}                                                                             \
                                                                              \
void Field::init( const Field& f, int begin, int count) {                     \
     if (begin < 0)                                                           \
          begin = 0;                                                          \
     if (begin >= f.count())                                                  \
          return;                                                             \
     if (count <= 0)                                                          \
          return;                                                             \
     if (begin + count > f.count())                                           \
          count = f.count() - begin;                                          \
     data_ = new Data[count];                                                 \
     size_ = count_ = count;                                                  \
     for (int i = 0; i < count_; i++) {                                       \
          data_[i] = f[begin + i];                                            \
     }                                                                        \
}                                                                             \
                                                                              \
Field& Field::operator +=( const Field& f) {                                  \
     int count = count_ + f.count();                                          \
     if (!count)                                                              \
          return *this;                                                       \
     if (!count_)                                                             \
          return operator=( f);                                               \
                                                                              \
     Data* ndata = new Data [count];										  \
     int i = 0, k = 0 ;														  \
     for (; i < count_; ) {													  \
          ndata[k++] = data_[i++];                                            \
     }                                                                        \
     for (i = 0; i < f.count(); ) {                                           \
          ndata[k++] = f[i++];                                                \
     }                                                                        \
     init( count, ndata, count);                                              \
     return *this;                                                            \
}                                                                             \
                                                                              \
boolean Field::operator ==( const Field& data) const {                        \
     if (count() != data.count())                                             \
          return false;                                                       \
     for (int i = 0; i < count(); i++) {                                      \
          if (data_[i] != data[i])                                            \
               return false;                                                  \
     }                                                                        \
     return true;                                                             \
}                                                                             \
                                                                              \
boolean Field::operator <( const Field& data) const {                         \
     int count = (count_ < data.count()) ? count_ : data.count();             \
     for (int i = 0; i < count; i++) {                                        \
          if (data_[i] < data[i])                                             \
               return true;                                                   \
          else if (data[i] < data_[i])                                        \
               return false;                                                  \
     }                                                                        \
     return ((count_ < data.count()) ? true : false);                         \
}                                                                             \
                                                                              \
ostream& operator << ( ostream& s, const Field& f) {                          \
     for (int i = 0; i < f.count(); i++)                                      \
          s << f[i] << ' ';                                                   \
     s << endl;                                                               \
     return s;                                                                \
}

#endif
