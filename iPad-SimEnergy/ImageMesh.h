//
//  ImageMesh.h
//
//  Copyright (c) 2013 S. Kaji
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.
//

#import <Foundation/Foundation.h>
#import <GLKit/GLKit.h>

@interface ImageMesh : NSObject

// mesh division
@property GLint verticalDivisions,horizontalDivisions;
@property int indexArrsize;
// vertex indices for triangle strip
@property int *vertexIndices;

// OpenGL
@property GLfloat *verticesArr, *textureCoordsArr;
@property GLKTextureInfo *texture;

// image size
@property float image_width,image_height;

// number of vertices
@property int numVertices;

// radius for a point ( used for touch recognision )
@property float radius;

// current vertex coordinates
@property GLfloat *x, *y;
// initial vertex coordinates
@property GLfloat *ix, *iy;
// vertex index of triangles
@property int *triangles;
// number of triangles
@property int numTriangles;

// to keep track of touches
@property int *selected; // list of the indices of the selected vertices
@property int numSelected;

- (void)dealloc;

// init
- (ImageMesh*)initWithUIImage:(UIImage*)uiImage VerticalDivisions:(GLuint)verticalDivisions HorizontalDivisions:(GLuint)horizotalDivisions;

- (void)deform;
- (void)initialize;

@end
