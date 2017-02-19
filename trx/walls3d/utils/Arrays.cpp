// This may look like C code, but it is really -*- C++ -*-

// <copyright>
// 
// Copyright (c) 1993
// Institute for Information Processing and Computer Supported New Media (IICM),
// Graz University of Technology, Austria.
// 
// </copyright>

//<file>
//
// Name:       arrays.h
//
// Purpose:    Generic dynamic and sorted arrays of data
//
// Created:     1 Feb 92    Gerald Pani
//
// Modified:   20 Jul 93    Gerald Pani
//
//
//
// Description:
//
// Macros for creating arrays of abstract datatypes. The array is
// always sorted and dynamic in storage allocation.
//
//</file>

#ifndef hg_utils_arrays_h
#define hg_utils_arrays_h

#include "../stdafx.h"

#include "types.h"

#include <iostream>
#include <fstream>
using namespace std;
#include "../generic.h"
#include "../hyperg/objconst.h"


//<class>
//
// Name:       Array
//
// Purpose:    Macro for dynamic and sorted arrays of type Data
//
//
//
// Public Interface:
//
// - Array()
//   Default constructor. Sets up empty Array with length 0.
//
// - Array( int c, Data* i, int s = 0)
//   Constructor. Sets up Array with length s (or default c) from
//   datafield i within c elements. The constructor sorts the array.
//   The STORAGE ALLOCATED by datafield i is NOW CONTROLLED by the ARRAY.
//
// - Array( int s)
//   Constructor. Sets up empty Array with length s.
//
// - Array( const Array& arr)
//   Constructor. Sets up Array with data from arr.
//
// - Array( const Array& arr, int begin, int count)
//   Constructor. Sets up Array with count elements of arr beginning
//   at position begin.
//
// - ~Array()
//   Destructor. Deletes ALL storage used by the array itself and
//   calls the destructor of all elements.
//
// - void init( int c, Data* i, int s = 0)
//   Free old data and reinitialize the array. The function sorts the
//   array. The STORAGE ALLOCATED by datafield i is NOW CONTROLLED by
//   the ARRAY.
//
// - void free()
//   Deletes ALL storage used by the array itself and
//   calls the destructor of all elements.
//
// - boolean insert(const Data& data)
//   Inserts data into the array. If data is already inserted, the
//   function does nothing and return false.
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
// - Array& operator =( const Array& arr)
//   Free any old data if arr not equal this and copy the data of arr.
//   Returns a reference to the result (this).
//
// - Array& operator +=( const Array& a)
//   Union of this array with a returning this.
//
// - Array& operator *=( const Array& a)
//   Intersection of this array with a returning this.
//
// - Array& operator -=( const Array& a)
//   Remove all elements from this array, which are members of a too.
//   Returns a reference to the result (this).
//
// - boolean operator ==( const Array& a)
//   True, if both arrays are equal. Otherwise false.
//
// - boolean remove( const Data& data)
//   Remove data from this array. Returns true, if data was a member.
//
// - boolean element( const Data& data) const
//   True, if data is a member.
//
// - boolean position( const Data& data, int& i) const
//   If data is a member, assign the position (index) of it to i and
//   return true.
//
// - Array& getpart( const Array& a, int b, int c)
//   Free any old data and reinitialize the array with the c elements
//   of a, beginning at position b. Return a reference to this.
//
// - friend ostream& operator <<( ostream& s, const Array& arr)
//   Writes the array arr to the ostream s by calling the operator <<
//   for each element. Elements are separated by space and after the
//   last item a newline is printed.
// 
// Protected Interface:
//
// - void enlarge()
//   Enlarges the allocated storage.
//
// - void remove( int i)
//   Removes the element at position i.
//
// - void init( const Array& arr, int begin, int count)
//   Reinitialize the array with count elements of arr beginning at begin.
//
// - void sort()
//   Sorts the array.
//
//
// Description:
//
// The key of each array element is unique. Insert and sort garantee
// this. The array is always sorted. Binary search is used to find an
// element. 
//
// To create an array of class Data use Arraysdeclare(ArrayName, Data)
// in your header file and Arraysimplement(ArrayName, Data) in your
// corresponding C++-file.
//
// The class Data must have:
// * a default constructor
// * a destructor
// * an operator =
// * an operator ==
// * an operator <
// * an operator !=
// * an operator <<
// 
//</class>

//#define Arraysdeclare(Array, Data)                       

#define Data RString
class Array {                                    
  public:                                    
     Array();                                    
     Array( int c, Data* i, int s = 0);                      
     Array( int s);                              
     Array( const Array&);                           
     Array( const Array&, int, int);                         
     ~Array();                                   
     void init( int c, Data* i, int s = 0);                  
     void free();                                
     boolean insert( const Data&);                       
     int count() const;                              
     const Data* data() const;                           
     const Data& operator []( int) const;                    
     Array& operator =( const Array&);                       
     Array& operator +=( const Array&);                          
     Array& operator *=( const Array&);                      
     Array& operator -=( const Array&);                      
     boolean operator ==( const Array&);                     
     boolean remove( const Data&);                       
     boolean element( const Data&) const;                    
     boolean position( const Data& , int& ) const;               
     Array& getpart( const Array&, int, int);                    
                                         
     friend ostream& operator <<( ostream&, const Array&);           
                                         
  protected:                                     
     void enlarge();                                 
     void remove( int);                              
     void init( const Array&, int, int);                     
     void sort();                                
                                         
     int count_;                                 
     int size_;                                  
     Data* data_;                                
};                                       
                                         
inline const Data& Array::operator []( int i) const {                
     return data_[i];                                
}                                        
                                         
inline int Array::count() const {                        
     return count_;                              
}                                        
                                         
inline const Data* Array::data() const {                     
     return data_;                               
}                                        
                                         
inline Array::Array() : count_( 0), data_( nil), size_( 0) {             
}                                        
                                         
inline Array::Array( const Array& arr) : count_( 0), data_( nil), size_( 0) { 
     operator=( arr);                                
}                                        
                                         
inline Array::Array( int c, Data* i, int s) : count_( c), data_( i) {        
     if (s <= c)                                 
      size_ = count_;                            
     else                                    
      size_ = s;                                 
     sort();                                     
}                                        
                                         
inline Array::Array( int s) : count_( 0) , /*data_( s ? new Data[s] : nil),*/ size_( s) {
    data_ = s ? new Data[s] : nil;      // for the sake of MS-VC compatibility ...
}

#define Arraysimplement(Array, Data)                         
boolean Array::insert( const Data& data) {                   
     int pos;                                    
     if (!position( data, pos)) {                        
      if (count_ == size_)                           
           enlarge();                            
      for (int i = count_; i > pos;i--)                  
           data_[i] = data_[i-1];                        
      data_[pos] = data;                             
      count_++;                              
      return true;                               
     }                                       
     return false;                               
}                                        
                                         
boolean Array::remove( const Data& data) {                   
     int pos;                                    
     if (position( data, pos)) {                         
      remove( pos);                              
      return true;                               
     }                                       
     return false;                               
}                                        
                                         
void Array::remove( int pos) {                           
     count_--;                                   
     for (int i = pos; i < count_; i++) {                    
      data_[i] = data_[i+1];                         
     }                                       
}                                        
                                         
boolean Array::position( const Data& data, int& pos) const {             
     int low = 0, high = count_;                         
     for (;;) {                                  
      if (low == high) {                             
           pos = low;                            
           return false;                             
      }                                  
      int mid = (low + high)/2;                      
      if (data == data_[mid]) {                      
           pos = mid;                            
           return true;                          
      }                                  
      if (data < data_[mid])                         
           high = mid;                           
      else                                   
           low = mid + 1;                            
     }                                       
}                                        
                                         
void Array::free() {                                 
     count_ = size_ = 0;                             
     if (!data_)                                 
      return;                                
     delete [] data_;                                
     data_ = nil;                                
}                                        
                                         
void Array::init( int c, Data* i, int s) {                   
     free();                                     
     count_ = c;                                 
     if (s < count_)                                 
      size_ = count_;                            
     else                                    
      size_ = s;                                 
     data_ = size_ ? i : nil;                            
     sort();                                     
}                                        
                                         
Array::~Array() {                                
     free();                                     
}                                        
                                         
boolean Array::element( const Data& data) const {                
     int pos;                                    
     if (position( data, pos))                           
      return true;                               
     return false;                               
}                                        
                                         
void Array::enlarge() {                              
     Data* olddata = data_;                          
     if (size_ == 0)                                 
      size_ = 1;                                 
     else                                    
      size_ *= 2;                                
     data_ = new Data [size_];                           
     Data* p = olddata, * q = data_;                         
     for(int i = count_; i; i--)                         
      *q++ = *p++;                               
     delete [] olddata;                              
}                                        
                                         
Array& Array::operator =( const Array& arr) {                    
     if (data_ == arr.data_)                             
          return *this;                              
     free();                                     
     if (!arr.count())                               
      return *this;                              
     data_ = new Data [arr.count()];                         
     if (!data_)                                 
      return *this;                              
     count_ = size_ = arr.count();                       
     for (int i = 0; i < count_; i++ ) {                     
      data_[i] = arr[i];                             
     }                                       
     return *this;                               
}                                        
                                         
Array& Array::getpart( const Array& arr, int begin, int count) {         
     free();                                     
     init( arr, begin, count);                           
     return *this;                               
}                                        
                                         
Array::Array( const Array& arr, int begin, int count) : count_( 0), data_( nil), size_( 0) {
     init( arr, begin, count);                           
}                                        
                                         
void Array::init( const Array& arr, int begin, int count) {          
     if (begin < 0)                              
      begin = 0;                                 
     if (begin >= arr.count())                           
      return;                                
     if (count <= 0)                                 
      return;                                
     if (begin + count > arr.count())                        
      count = arr.count() - begin;                       
     data_ = new Data[count];                            
     size_ = count_ = count;                             
     for (int i = 0; i < count_; i++) {                      
      data_[i] = arr[begin + i];                         
     }                                       
}                                        
                                         
Array& Array::operator +=( const Array& arr)
{                   
     int count = count_ + arr.count();                       
     if (!count)                                 
      return *this;                              
     if (!count_)                                
      return operator=( arr);                        
                                         
     Data* ndata = new Data [count];

	 int i=0,j=0,k=0;

     for (; i < count_ && j < arr.count(); ) {        
      if (data_[i] == arr[j]) {                      
           ndata[k++] = data_[i++];                      
           j++;                              
      }                                  
      else if (data_[i] < arr[j]) {                      
           ndata[k++] = data_[i++];                      
      }                                  
      else {                                 
           ndata[k++] = arr[j++];                        
      }                                  
     }

     if (i == count_) {                              
      for ( ; j < arr.count(); )                         
           ndata[k++] = arr[j++];                        
     }                                       
     else {                                  
      for ( ; i < count_; )                          
           ndata[k++] = data_[i++];                      
     }

     init(k, ndata, count);                          
     return *this;                               
}                                        
                                         
Array& Array::operator *=( const Array& arr) {                   
     if (!count_)                                
      return *this;

	 int i=0,j=0,k=0;

     for (; i < count_ && j < arr.count(); ) {        
      if (data_[i] == arr[j]) {                      
           data_[k++] = data_[i++];                      
           j++;                              
      }                                  
      else if (data_[i] < arr[j]) {                      
           i++;                              
      }                                  
      else {                                 
           j++;                              
      }                                  
     }                                       
     count_ = k;                                 
     return *this;                               
}                                        
                                         
Array& Array::operator -=( const Array& arr) {                   
     if (!count_)                                
      return *this;

	 int i=0,j=0,k=0;

     for (; i < count_ && j < arr.count(); ) {        
      if (data_[i] == arr[j]) {                      
           i++;                              
           j++;                              
      }                                  
      else if (data_[i] < arr[j]) {                      
           data_[k++] = data_[i++];                      
      }                                  
      else {                                 
           j++;                              
      }                                  
     }

     if (j == arr.count()) {                             
      for ( ; i < count_; )                          
           data_[k++] = data_[i++];                      
     }                                       
     count_ = k;                                 
     return *this;                               
}                                        
                                         
boolean Array::operator ==( const Array& data) {                 
     if (count() != data.count())                        
      return false;                              
     for (int i = 0; i < count(); i++) {                     
      if (data_[i] != data[i])                       
           return false;                             
     }                                       
     return true;                                
}                                        
                                         
void Array::sort() {                                 
     boolean sort;                               
     do {                                    
      sort = true;                               
      for (int i = 0, j = 1; j < count(); ) {                
           if (data_[j] < data_[i]) {                        
            if (data_[i] == data_[j]) {                  
             remove(j);                      
             continue;                       
            }                                
            else {                           
             Data tmp = data_[i];                    
             data_[i] = data_[j];                    
             data_[j] = tmp;                     
             sort = false;                       
            }                                
           }                                 
           i++; j++;                             
      }                                  
     } while (!sort);                                
}                                        
                                         
ostream& operator << ( ostream& s, const Array& arr) {               
     for (int i = 0; i < arr.count(); i++)                   
      s << arr[i] << ' ';                            
     s << endl;                                  
     return s;                                   
}

#endif
