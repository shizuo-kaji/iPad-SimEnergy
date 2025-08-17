//
//  iPad-SimEnergy-Bridging-Header.h
//  iPad-SimEnergy
//
//  Swift bridging header to expose Objective-C classes and C/C++ functionality
//

#ifndef iPad_SimEnergy_Bridging_Header_h
#define iPad_SimEnergy_Bridging_Header_h

// Import Foundation and UIKit for basic functionality
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <CoreFoundation/CoreFoundation.h>

// Import Accelerate framework for LAPACK support
#import <Accelerate/Accelerate.h>

// Import custom Objective-C classes
#import "ImageMesh.h"

// Define constants used in mathematical computations
// N should be defined based on the maximum number of vertices
#ifndef N
#define N 2000  // Adjust based on maximum mesh resolution needs
#endif

// C++ Eigen includes (for mixed C++/Swift compatibility)
#ifdef __cplusplus
#include "../third-party/eigen/Eigen/Sparse"
#include "../third-party/eigen/Eigen/Dense"
#endif

#endif /* iPad_SimEnergy_Bridging_Header_h */