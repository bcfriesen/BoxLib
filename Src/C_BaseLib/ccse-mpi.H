
#ifndef BL_CCSE_MPP
#define BL_CCSE_MPP

//
// This header encapsulates the MPI header.
//
// By placing <mpi.h> here we make it easier to recompile with or w/o MPI.
//

#ifdef BL_USE_MPI
#include <winstd.H>

#include <mpi.h>

namespace ParallelDescriptor
{
    template <class T>
    struct Mpi_typemap
    {
	static MPI_Datatype type();
    };
    //
    // Template functions to return MPI_Datatype depending on argument type.
    //
    template <> MPI_Datatype Mpi_typemap<char>::type();
    template <> MPI_Datatype Mpi_typemap<short>::type();
    template <> MPI_Datatype Mpi_typemap<int>::type();
    template <> MPI_Datatype Mpi_typemap<long>::type();
    template <> MPI_Datatype Mpi_typemap<unsigned char>::type();
    template <> MPI_Datatype Mpi_typemap<unsigned short>::type();
    template <> MPI_Datatype Mpi_typemap<unsigned int>::type();
    template <> MPI_Datatype Mpi_typemap<unsigned long>::type();
    template <> MPI_Datatype Mpi_typemap<float>::type();
    template <> MPI_Datatype Mpi_typemap<double>::type();
}


#else

typedef int MPI_Op;
typedef int MPI_Comm;
typedef int MPI_Group;
typedef int MPI_Datatype;
struct MPI_Status {};
typedef int MPI_Request;
const int MPI_DATATYPE_NULL = 0;
const int MPI_REQUEST_NULL  = 0;
const int MPI_COMM_WORLD    = 0;
const int MPI_COMM_NULL     = 0;
const int MPI_GROUP_NULL    = 0;
const int MPI_MAX_PROCESSOR_NAME = 64;

namespace ParallelDescriptor
{
    template <class T>
    struct Mpi_typemap
    {
	static MPI_Datatype type();
    };
}

#endif /*BL_USE_MPI*/

#endif /*BL_CCSE_MPP*/
