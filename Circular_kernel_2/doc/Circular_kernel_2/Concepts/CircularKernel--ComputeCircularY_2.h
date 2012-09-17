
/*!
\ingroup PkgCircularKernel2Concepts
\cgalconcept

A model `fo` of this type must provide: 

*/

class CircularKernel::ComputeCircularY_2 {
public:

/// \name See Also 
/// @{

/*! 
Computes the \f$ y\f$-coordinate of the point. 
*/ 
CircularKernel::Root_of_2 
operator()(const CircularKernel::Circular_arc_point_2 &p); 

/// @}

}; /* end CircularKernel::ComputeCircularY_2 */

