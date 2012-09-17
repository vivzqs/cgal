
/*!
\ingroup PkgCircularKernel2Concepts
\cgalconcept

\refines ::Kernel::IsVertical_2 
*/

class CircularKernel::IsVertical_2 {
public:

/// \name Operations 
/// An object `fo` of this type must provide: 
/// @{

/*! 
For a line arc. 
*/ 
bool operator() 
(const CircularKernel::Line_arc_2 & l); 

/*! 
For a circular arc, always returns `false`. 
*/ 
bool operator() 
(const CircularKernel::Circular_arc_2 & c); 

/// @}

}; /* end CircularKernel::IsVertical_2 */

