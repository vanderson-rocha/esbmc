// Fig. 9.10: circle2.h
// Circle2 class contains x-y coordinate pair and radius.
#ifndef CIRCLE2_H
#define CIRCLE2_H

#include "point.h"  // Point class definition

class Circle2 : public Point {

public:

   // default constructor
   Circle2( int = 0, int = 0, double = 0.0 );  

   void setRadius( double );   // set radius
   double getRadius() const;   // return radius

   double getDiameter() const;       // return diameter
   double getCircumference() const;  // return circumference
   double getArea() const;           // return area

   void print() const;         // output Circle2 object

private: 
   double radius;  // Circle2's radius

}; // end class Circle2

#endif 

/**************************************************************************
 * (C) Copyright 1992-2003 by Deitel & Associates, Inc. and Prentice      *
 * Hall. All Rights Reserved.                                             *
 *                                                                        *
 * DISCLAIMER: The authors and publisher of this book have used their     *
 * best efforts in preparing the book. These efforts include the          *
 * development, research, and testing of the theories and programs        *
 * to determine their effectiveness. The authors and publisher make       *
 * no warranty of any kind, expressed or implied, with regard to these    *
 * programs or to the documentation contained in these books. The authors *
 * and publisher shall not be liable in any event for incidental or       *
 * consequential damages in connection with, or arising out of, the       *
 * furnishing, performance, or use of these programs.                     *
 *************************************************************************/