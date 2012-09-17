
/*!
\ingroup PkgCircularKernel2Concepts
\cgalconcept

\refines `Kernel::HasOnBoundedSide_2` 

\sa `CircularKernel::BoundedSide_2`
\sa `CircularKernel::HasOnUnboundedSide_2`

*/

class CircularKernel::HasOnBoundedSide_2 {
public:

/// \name Operations 
/// An object `fo` of this type must provide:
/// @{

/*! 

*/ 
bool 
operator() 
(const CircularKernel::Circle_2& s, 
const CircularKernel::Circular_arc_point_2& p); 

/// @}

}; /* end CircularKernel::HasOnBoundedSide_2 */

