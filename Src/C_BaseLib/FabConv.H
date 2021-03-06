
#ifndef BL_FABCONV_H
#define BL_FABCONV_H

#include <iosfwd>

#include <Array.H>
#include <BLassert.H>
#include <REAL.H>

//
// A Descriptor of the Long Integer type
//
// This class is meant to hold all information needed to completely
// describe the "long" type on a machine.  To describe a "long" both
// the number of bytes in the long and their ordering, relative to canonical
// ordering 1 .. sizeof(long), needs to be specified.
//
// This allows us to write out "long"s in the native format on a machine,
// and then by also saving the IntDescriptor, we can read them back in on
// another machine and have enough information to construct the exact same
// "long" values.
//

class IntDescriptor
{

public:
    //
    // An enumeration describing the two ordering of a "long"
    // that we currently support: NormalOrder and ReverseOrder.
    // Other ordering may be added as BoxLib is ported to run on
    // machines with non-standard "long" orderings.
    //
    enum Ordering { NormalOrder = 1, ReverseOrder = 2 };
    //
    // The default constructor.  Does not build a proper IntDescriptor.
    // This should only be used when you need to build a generic
    // IntDescriptor in order to be able to read in a specific
    // one from an istream.
    //
    IntDescriptor ();
    //
    // Construct a specific IntDescriptor.
    //
    IntDescriptor (long     nb,
                   Ordering ordering = NormalOrder);
    //
    // Returns the ordering of the "long".
    //
    Ordering order () const;
    //
    // Returns the number of bytes in a "long".
    //
    int numBytes () const;
    //
    // The equality operator.
    //
    bool operator== (const IntDescriptor& id) const;
    //
    // The inequality operator.
    //
    bool operator!= (const IntDescriptor& id) const;

private:

    long     numbytes;
    Ordering ord;
};

//
// Write out an IntDescriptor to an ostream in ASCII.
//
std::ostream& operator<< (std::ostream& os, const IntDescriptor& id);
//
// Read in an IntDescriptor from an istream.
//
std::istream& operator>> (std::istream& is, IntDescriptor& id);

/*
  A Descriptor of the Real Type

  This class is meant to hold all information needed to completely
  describe the "Real" floating-point type on a machine.  By "Real" here we
  mean either the "float" or "double" type that this version of BoxLib
  was built with, which corresponds to whether BL_USE_FLOAT or
  BL_USE_DOUBLE was used to build the version of the library.
  
  To describe a "Real" type two arrays are needed: one detailing the ordering
  of the bytes in the Real, relative to the canonical ordering
  1 .. sizeof(Real) and the other detailing the format of the floating-point
  number.

  The array detailing the format of a floating-point number is an eight-element
  array of longs containing the following information:

          format[0] = number of bits per number
          format[1] = number of bits in exponent
          format[2] = number of bits in mantissa
          format[3] = start bit of sign
          format[4] = start bit of exponent
          format[5] = start bit of mantissa
          format[6] = high order mantissa bit (CRAY needs this)
          format[7] = bias of exponent

  This allows us to write out "Real"s in the native format on a machine,
  and then by also saving the IntDescriptor, we can read them back in on
  another machine and have enough information to construct the exact same
  "Real" values, provided the Reals have the same size on the two machines.
*/

class RealDescriptor
{
public:
    //
    // The default constructor.  Does not build a proper
    // RealDescriptor. This should only be used when you need to
    // build a generic RealDescriptor in order to be able to read
    // in a specific one from an istream.
    //
    RealDescriptor ();
    //
    // Construct a specific RealDescriptor, passing in the format
    // of the Real, the order of the Real, and the length of the
    // array detailing the ordering.
    //
    RealDescriptor (const long* format,
                    const int*  order,
                    int         order_length);
    //
    // Returns the format array as a const long*
    //
    const long* format () const;
    //
    // Returns const Array<long> reference to the format array.
    //
    const Array<long>& formatarray () const;
    //
    // Returns the order array as a const int*
    //
    const int* order () const;
    //
    // Returns const Array<int> reference to the order array.
    //
    const Array<int>& orderarray () const;
    //
    // Returns the number of bytes in the Real.
    //
    int numBytes () const;
    //
    // The equality operator.
    //
    bool operator== (const RealDescriptor& rd) const;
    //
    // The inequality operator.
    //
    bool operator != (const RealDescriptor& rd) const;
    //
    // Set to always fix denormals when converting to native format.
    //
    static void SetFixDenormals ();
    //
    // Returns a copy of this RealDescriptor on the heap.
    // The user is responsible for deletion.
    //
    RealDescriptor* clone () const;
    //
    // Returns a RealDescriptor on the heap detailing requested
    // floating-point type.  Here format, precision, and ordering
    // correspond to the enumerations in FABio.  This is here to
    // support reading "old" FABs.  Do NOT use it in new code.
    //
    static RealDescriptor* newRealDescriptor (int         format,
                                              int         precision,
                                              const char* systype,
                                              int         ordering);
    //
    // Convert nitems in RealDescriptor format to native Real format.
    // The out array is assumed to be large enough to hold the
    // resulting output.
    //
    static void convertToNativeFormat (Real*                 out,
                                       long                  nitems,
                                       void*                 in,
                                       const RealDescriptor& id);
    //
    // Read nitems from istream in ReadDescriptor format and
    // convert them to the native Real format.  The out array is
    // assumed to be large enough to hold the resulting output.
    //
    static void convertToNativeFormat (Real*                 out,
                                       long                  nitems,
                                       std::istream&         is,
                                       const RealDescriptor& id);
    //
    // Convert nitems Reals in native format to RealDescriptor format
    // and write them to the ostream.
    //
    static void convertFromNativeFormat (std::ostream&         os,
                                         long                  nitems,
                                         const Real*           in,
                                         const RealDescriptor& od);
    //
    // Convert nitems Reals in native format to RealDescriptor format.
    // The out array is assumed to be large enough to hold the
    // resulting output.
    //
    static void convertFromNativeFormat (void*                 out,
                                         long                  nitems,
                                         Real*                 in,
                                         const RealDescriptor& od);
private:

    Array<long> fr;
    Array<int>  ord;
    static bool bAlwaysFixDenormals;
};

//
// Write out an RealDescriptor to an ostream in ASCII.
//
std::ostream& operator<< (std::ostream& os, const RealDescriptor& id);
//
// Read in a RealDescriptor from an istream.
//
std::istream& operator>> (std::istream& is, RealDescriptor& id);

#endif /*BL_FABCONV_H*/
