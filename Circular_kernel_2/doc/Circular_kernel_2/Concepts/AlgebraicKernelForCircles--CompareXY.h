
/*!
\ingroup PkgCircularKernel2Concepts
\cgalconcept

\sa `AlgebraicKernelForCircles::CompareX`
\sa `AlgebraicKernelForCircles::CompareY`
\sa `CircularKernel::CompareXY_2`

*/

class AlgebraicKernelForCircles::CompareXY {
public:

/// \name Operations
/// A model `fo` of this type must provide:
/// @{

/*! 
Compares two `Root_for_circles_2_2` lexicographically. 
*/ 
template < class OutputIterator > 
CGAL::Comparison_result 
operator()(const AlgebraicKernelForCircles::Root_for_circles_2_2 & r1, 
const AlgebraicKernelForCircles::Root_for_circles_2_2 & r2); 

/// @}

}; /* end AlgebraicKernelForCircles::CompareXY */

