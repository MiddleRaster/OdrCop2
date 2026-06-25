#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

#ifdef TEST_MATRIX_DOCS
Legend:
•	✓ = meaningful, distinct test case
•	— = not applicable
•	(covered) = redundant with another test

Internal-linkage entity |	Data member type |	Base class |	Function parameter |	Function return |	Variable type |	Template type arg |	NTTP arg |	Alias/typedef member |
Class/struct/union      |           ✓        |      ✓      |            ✓          |            ✓       |       ✓         |	        ✓	      |     —    |  	    ✓            |  
Enum	                |           ✓        |      —      |        	✓          |        	✓       |   	✓         |         ✓         |	    ✓    |      	✓            |
Function                |       	—        |  	—      |        	—          |        	—       |   	—         |         —         | 	✓    |	        —            |
Object/variable         |       	—        |  	—      |        	—          |        	—       |   	✓         |     	—         | 	✓    |      	—            |
Class template          |       	✓        |  	✓      |        	✓          |        	✓       |   	✓         |     	✓         | 	—    |      	✓            |
Variable template       |       	—        |  	—      |        	—          |        	—       |   	✓         |     	—         | 	✓    |      	—            |
That gives 24 distinct cells.

Additional ODR-significant positions
These are worth separate tests because they are part of declaration identity:
Usage position            |	     Class	     |     Enum	   |         Function       |	Variable    |
Static data member type   |       	✓        |  	✓      |        	—           |   	—       |
Default function argument |	        ✓        |  	✓      |        	✓           |   	✓       |
Requires-clause/constraint|     	✓        |  	✓      |         	✓           |   	✓       |
Friend declaration        |  	    ✓        |  	✓      |        	✓           |   	✓       |
Deduction guide           |     	✓        |  	✓      |        	—           |   	—       |

Things I would
not
make separate rows
Candidate row                           |         	Reason                                                          |
Typedef in anonymous namespace	        |   Same underlying type identity.                                          |
Alias in anonymous namespace	        |   Same underlying type identity.                                          |
Trailing return type	                |   Same as return type.                                                    |
Using-declaration                       |	Usually reduces to one of the above cases.                              |
Lambda closure type                     |	Better tested as its own category, not as an anonymous-namespace type.  |
Anonymous namespace namespace itself    |	Not an entity type.                                                     |

#endif