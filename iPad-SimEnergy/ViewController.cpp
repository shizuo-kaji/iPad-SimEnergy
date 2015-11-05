//
//  ViewController.m
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

#import "ViewController.h"
#import <Accelerate/Accelerate.h>

#include "Eigen/Sparse"
#include "Eigen/Dense"
#include <vector>

using namespace Eigen;

typedef SparseMatrix<float> SpMat;
typedef SparseLU<SpMat, COLAMDOrdering<int>> SpSolver;
typedef Triplet<float> T;


#define MAX_TOUCHES 7
// the numbers of horizontal and vertical grids
#define HDIV 10
#define VDIV 10
#define NUMV 2*(HDIV+1)*(VDIV+1)
#define DEFAULTIMAGE @"Default.png"

@interface ViewController ()
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;
- (void)setupGL;
- (void)tearDownGL;
@end

@implementation ViewController
@synthesize effect;

// for LAPACK
__CLPK_integer N=NUMV, NRHS=1, LDA=NUMV, IPIV[NUMV], LDB=NUMV, INFO;
float A[NUMV*NUMV], mat[NUMV*NUMV];
float vec[NUMV];
// for Eigen
SpSolver solver;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    
    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    [EAGLContext setCurrentContext:self.context];
    
    // enable multi-touch tracking
    self.view.multipleTouchEnabled = YES;
    // prepare dictionary for touch tracking
    touchedPts = CFDictionaryCreateMutable(NULL, MAX_TOUCHES, &kCFTypeDictionaryKeyCallBacks,NULL);
    // load default image
    UIImage *pImage = [ UIImage imageNamed:DEFAULTIMAGE ];
    mainImage = [[ImageMesh alloc] initWithUIImage:pImage VerticalDivisions:VDIV HorizontalDivisions:HDIV];
    [self loadTexture:pImage];
    
    [self setupGL];
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil)) {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context) {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (void)loadTexture:(UIImage *)pImage{
    NSError *error;
    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
                             [NSNumber numberWithBool:YES],
                             GLKTextureLoaderOriginBottomLeft,
                             nil];
    UIImage *image = [UIImage imageWithData:UIImagePNGRepresentation(pImage)];
    mainImage.texture = [GLKTextureLoader textureWithCGImage:image.CGImage options:options error:&error];
    if (error)
        NSLog(@"Error loading texture from image: %@",error);
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    self.effect = [[GLKBaseEffect alloc] init];
    [self setupScreen];
}

- (void)setupScreen{
    float gl_height, gl_width, ratio;
    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    if (UIInterfaceOrientationIsLandscape(orientation)) {
        screen.height = [UIScreen mainScreen].bounds.size.width;
        screen.width = [UIScreen mainScreen].bounds.size.height;
    }else{
        screen.height = [UIScreen mainScreen].bounds.size.height;
        screen.width = [UIScreen mainScreen].bounds.size.width;
    }
    if (screen.width*mainImage.image_height<screen.height*mainImage.image_width) {
        ratio = mainImage.image_width/screen.width;
        gl_width = mainImage.image_width;
        gl_height = screen.height*ratio;
    }else{
        ratio = mainImage.image_height/screen.height;
        gl_height = mainImage.image_height;
        gl_width = screen.width*ratio;
    }
    ratio_height = gl_height / screen.height;
    ratio_width = gl_width / screen.width;
    // compute touch radius for each vertex
    float r = mainImage.image_width/(float)mainImage.horizontalDivisions;
    mainImage.radius = r*r;
    
    GLKMatrix4 projectionMatrix = GLKMatrix4MakeOrtho(-gl_width/2.0, gl_width/2.0, -gl_height/2.0, gl_height/2.0, -1024, 1024);
    self.effect.transform.projectionMatrix = projectionMatrix;
}

- (void)tearDownGL
{
    GLuint name = mainImage.texture.name;
    glDeleteTextures(1, &name);
    [EAGLContext setCurrentContext:self.context];
    self.effect = nil;    
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    [self renderImage];
}

- (void)renderImage{
    self.effect.texture2d0.name = mainImage.texture.name;
    self.effect.texture2d0.enabled = YES;
    
    glClearColor(0, 104.0/255.0, 55.0/255.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    
    [self.effect prepareToDraw];
    
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glEnableVertexAttribArray(GLKVertexAttribTexCoord0);
    
    glVertexAttribPointer(GLKVertexAttribPosition, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, mainImage.verticesArr);
    glVertexAttribPointer(GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, mainImage.textureCoordsArr);
    
    for (int i=0; i<mainImage.verticalDivisions; i++) {
        glDrawArrays(GL_TRIANGLE_STRIP, i*(mainImage.horizontalDivisions*2+2), mainImage.horizontalDivisions*2+2);
    }
}

/**
 *  Touch event tracking
 */
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event {
    if ([touches count] > 0){
        for (UITouch *touch in touches) {
            int *point = (int *)CFDictionaryGetValue(touchedPts, (__bridge void*)touch);
            // touched location
            CGPoint p = [touch locationInView:self.view];
            p.x = (p.x - screen.width/2.0)*ratio_width;
            p.y = (screen.height/2.0 - p.y)*ratio_height;
            // nearest point will be considered to be touched ?
            int closest_vertex = 0;
            float min_dist = mainImage.radius;
            // !! the lower left corner is kept unselectable due to a technical reason
            for(int i=1;i<mainImage.numVertices;i++){
                float dist = (p.x-mainImage.x[i])*(p.x-mainImage.x[i])+(p.y-mainImage.y[i])*(p.y-mainImage.y[i]);
                if(dist<min_dist){
                    min_dist = dist;
                    closest_vertex = i;
                }
            }
            if(min_dist<mainImage.radius){
                if (point == NULL) {
                    point = (int *)malloc(sizeof(*point));
                    CFDictionarySetValue(touchedPts, (__bridge void*)touch, point);
                }
                *point = closest_vertex;
                mainImage.selected[mainImage.numSelected] = closest_vertex;
                mainImage.numSelected++;
            }
        }
        [self formEnergy];
    }
}
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    if(mainImage.numSelected==0) return;
    for (UITouch *touch in touches) {
        CGPoint p = [touch locationInView:self.view];
        p.x = (p.x - screen.width/2.0)*ratio_width;
        p.y = (screen.height/2.0 - p.y)*ratio_height;
        int *point = (int *)CFDictionaryGetValue(touchedPts, (__bridge void*)touch);
        if (point != NULL){
            mainImage.x[*point] = p.x;
            mainImage.y[*point] = p.y;
        }
    }
//    [self solve_vertices_LAPACK];
    [self solve_vertices_Eigen];
    [mainImage deform];
}

- (void)solve_vertices_LAPACK{
    // clear constraint vector
    for(int i=0;i<N;i++){
        vec[i]=0;
    }
    if(mainImage.numSelected==1){
        int index=mainImage.selected[mainImage.numSelected-1];
        mainImage.x[0] = mainImage.ix[0] + mainImage.x[index]-mainImage.ix[index];
        mainImage.y[0] = mainImage.iy[0] + mainImage.y[index]-mainImage.iy[index];
        vec[0] = mainImage.x[0];
        vec[mainImage.numVertices] = mainImage.y[0];
    }
    for(int k=0;k<mainImage.numSelected;k++){
        int i=mainImage.selected[k];
        int j=i+mainImage.numVertices;
        vec[i] = mainImage.x[i];
        vec[j] = mainImage.y[i];
    }
    // LAPACK destroys original matrix so mat should be duplicated
    memcpy(A, mat, sizeof(float) * N * N);
    sgesv_(&N, &NRHS, A, &LDA, IPIV, vec, &LDB, &INFO);
    // could be optimised by re-using the factrisation (the matrix is invariant during a drag)
    //    NSLog(@"LAPACK: %ld", INFO);
    for(int i=0;i<mainImage.numVertices;i++){
        mainImage.x[i] = vec[i];
        mainImage.y[i] = vec[i+mainImage.numVertices];
    }
}

- (void)solve_vertices_Eigen{
    VectorXf V = VectorXf::Zero(NUMV);
    if(mainImage.numSelected==1){
        int index=mainImage.selected[mainImage.numSelected-1];
        mainImage.x[0] = mainImage.ix[0] + mainImage.x[index]-mainImage.ix[index];
        mainImage.y[0] = mainImage.iy[0] + mainImage.y[index]-mainImage.iy[index];
        V(0) = mainImage.x[0];
        V(mainImage.numVertices) = mainImage.y[0];
    }
    for(int k=0;k<mainImage.numSelected;k++){
        int i=mainImage.selected[k];
        int j=i+mainImage.numVertices;
        V(i) = mainImage.x[i];
        V(j) = mainImage.y[i];
    }
    VectorXf Sol = solver.solve(V);
//    VectorXf Sol = MatrixXf(G).householderQr().solve(V);
    for(int i=0;i<mainImage.numVertices;i++){
        mainImage.x[i] = Sol(i);
        mainImage.y[i] = Sol(i+mainImage.numVertices);
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        int *point = (int *)CFDictionaryGetValue(touchedPts, (__bridge void*)touch);
        if(point != NULL){
            CFDictionaryRemoveValue(touchedPts, (__bridge void*)touch);
            mainImage.numSelected--;
            //NSLog(@"touch released : %d", *point);
            free(point);
        }
    }
    [self formEnergy];
}
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event{
    NSLog(@"allTouches count : %lu (touchesCancelled:withEvent:)", (unsigned long)[[event allTouches] count]);
    for (UITouch *touch in touches) {
        int *point = (int *)CFDictionaryGetValue(touchedPts, (__bridge void*)touch);
        if(point != NULL){
            CFDictionaryRemoveValue(touchedPts, (__bridge void*)touch);
            free(point);
        }
    }
    mainImage.numSelected=0;
    [self formEnergy];
}

// form energy matrix
- (void)formEnergy{
    // all starting points are updated
    for(int j=0;j<mainImage.numVertices;j++){
        mainImage.ix[j] = mainImage.x[j];
        mainImage.iy[j] = mainImage.y[j];
    }
//    [self formEnergy_LAPACK];
   [self formEnergy_Eigen];
}

- (void)formEnergy_Eigen{
    SpMat G(NUMV, NUMV);
    std::vector<T> tripletListMat(0);
    tripletListMat.reserve(mainImage.numTriangles*30);
    // incorporate constraints
    bool avoid[mainImage.numVertices];
    for(int i=0;i<mainImage.numVertices;i++) avoid[i]=false;
    if(mainImage.numSelected==0){
        return;
    }else if(mainImage.numSelected==1){
        avoid[0]=true;
        tripletListMat.push_back(T(0,0,1.0));
        tripletListMat.push_back(T(mainImage.numVertices,mainImage.numVertices,1.0));
    }
    for(int s=0;s<mainImage.numSelected;s++){
        int i=mainImage.selected[s];
        int j=i+mainImage.numVertices;
        avoid[i]=true;
        tripletListMat.push_back(T(i,i,1.0));
        tripletListMat.push_back(T(j,j,1.0));
    }
    
    // form the energy derivation matrix
    for(int i=0;i<mainImage.numTriangles;i++){
        int posx=mainImage.triangles[3*i];
        int posy=posx+mainImage.numVertices;
        int posz=mainImage.triangles[3*i+1];
        int posw=posz+mainImage.numVertices;
        int poss=mainImage.triangles[3*i+2];
        int post=poss+mainImage.numVertices;
        float a = mainImage.ix[posx];
        float b = mainImage.iy[posx];
        float c = mainImage.ix[posz];
        float d = mainImage.iy[posz];
        float e = mainImage.ix[poss];
        float f = mainImage.iy[poss];
        float detA2 = (a*d-a*f-b*c+b*e+c*f-d*e)*(a*d-a*f-b*c+b*e+c*f-d*e);
        // partial derivative of the energy |B|^2 - 2 det(B), where B=VP^{-1}
        // partial by x and y
        if(!avoid[posx]){
            tripletListMat.push_back(T(posx,posx,(c*c-2*c*e+d*d-2*d*f+e*e+f*f)/detA2));
            tripletListMat.push_back(T(posx,posz,(-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2));
            tripletListMat.push_back(T(posx,posw,(-a*d+a*f+b*c-b*e-c*f+d*e)/detA2));
            tripletListMat.push_back(T(posx,poss,(a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2));
            tripletListMat.push_back(T(posx,post,(a*d-a*f-b*c+b*e+c*f-d*e)/detA2));
            tripletListMat.push_back(T(posy,posy,(c*c-2*c*e+d*d-2*d*f+e*e+f*f)/detA2));
            tripletListMat.push_back(T(posy,posz,(a*d-a*f-b*c+b*e+c*f-d*e)/detA2));
            tripletListMat.push_back(T(posy,posw,(-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2));
            tripletListMat.push_back(T(posy,poss,(-a*d+a*f+b*c-b*e-c*f+d*e)/detA2));
            tripletListMat.push_back(T(posy,post,(a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2));
        }
        // partial by z and w
        if(!avoid[posz]){
            tripletListMat.push_back(T(posz,posx,(-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2));
            tripletListMat.push_back(T(posz,posy,(a*d-a*f-b*c+b*e+c*f-d*e)/detA2));
            tripletListMat.push_back(T(posz,posz,(a*a-2*a*e+b*b-2*b*f+e*e+f*f)/detA2));
            tripletListMat.push_back(T(posz,poss,(-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2));
            tripletListMat.push_back(T(posz,post,(-a*d+a*f+b*c-b*e-c*f+d*e)/detA2));
            tripletListMat.push_back(T(posw,posx,(-a*d+a*f+b*c-b*e-c*f+d*e)/detA2));
            tripletListMat.push_back(T(posw,posy,(-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2));
            tripletListMat.push_back(T(posw,posw,(a*a-2*a*e+b*b-2*b*f+e*e+f*f)/detA2));
            tripletListMat.push_back(T(posw,poss,(a*d-a*f-b*c+b*e+c*f-d*e)/detA2));
            tripletListMat.push_back(T(posw,post,(-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2));
        }
        // partial by s and t
        if(!avoid[poss]){
            tripletListMat.push_back(T(poss,posx,(a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2));
            tripletListMat.push_back(T(poss,posy,(-a*d+a*f+b*c-b*e-c*f+d*e)/detA2));
            tripletListMat.push_back(T(poss,posz,(-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2));
            tripletListMat.push_back(T(poss,posw,(a*d-a*f-b*c+b*e+c*f-d*e)/detA2));
            tripletListMat.push_back(T(poss,poss,(a*a-2*a*c+b*b-2*b*d+c*c+d*d)/detA2));
            tripletListMat.push_back(T(post,posx,(a*d-a*f-b*c+b*e+c*f-d*e)/detA2));
            tripletListMat.push_back(T(post,posy,(a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2));
            tripletListMat.push_back(T(post,posz,(-a*d+a*f+b*c-b*e-c*f+d*e)/detA2));
            tripletListMat.push_back(T(post,posw,(-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2));
            tripletListMat.push_back(T(post,post,(a*a-2*a*c+b*b-2*b*d+c*c+d*d)/detA2));
        }
    }
    G.setFromTriplets(tripletListMat.begin(), tripletListMat.end());
    solver.compute(G);
    return;
}

- (void)formEnergy_LAPACK{
    // clear matrix
    for(int i=0;i<N*N;i++){
        mat[i]=0;
    }
    // form the energy derivation matrix
    for(int i=0;i<mainImage.numTriangles;i++){
        int posx=mainImage.triangles[3*i];
        int posy=posx+mainImage.numVertices;
        int posz=mainImage.triangles[3*i+1];
        int posw=posz+mainImage.numVertices;
        int poss=mainImage.triangles[3*i+2];
        int post=poss+mainImage.numVertices;
        float a = mainImage.ix[posx];
        float b = mainImage.iy[posx];
        float c = mainImage.ix[posz];
        float d = mainImage.iy[posz];
        float e = mainImage.ix[poss];
        float f = mainImage.iy[poss];
        float detA2 = (a*d-a*f-b*c+b*e+c*f-d*e)*(a*d-a*f-b*c+b*e+c*f-d*e);
        // CAREFUL: matrix indexing for LAPACK is in FORTRAN style
        // partial derivative of the energy |B|^2 - 2 det(B), where B=VP^{-1}
        // partial by x and y
        mat[posx + posx*NUMV] += (c*c-2*c*e+d*d-2*d*f+e*e+f*f)/detA2;
        mat[posx + posz*NUMV] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posx + posw*NUMV] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posx + poss*NUMV] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        mat[posx + post*NUMV] += + (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posy + posy*NUMV] += (c*c-2*c*e+d*d-2*d*f+e*e+f*f)/detA2;
        mat[posy + posz*NUMV] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posy + posw*NUMV] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posy + poss*NUMV] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posy + post*NUMV] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        // partial by z and w
        mat[posz + posx*NUMV] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posz + posy*NUMV] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posz + posz*NUMV] += (a*a-2*a*e+b*b-2*b*f+e*e+f*f)/detA2;
        mat[posz + poss*NUMV] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        mat[posz + post*NUMV] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posw + posx*NUMV] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posw + posy*NUMV] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posw + posw*NUMV] += (a*a-2*a*e+b*b-2*b*f+e*e+f*f)/detA2;
        mat[posw + poss*NUMV] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posw + post*NUMV] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        // partial by s and t
        mat[poss + posx*NUMV] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        mat[poss + posy*NUMV] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[poss + posz*NUMV] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        mat[poss + posw*NUMV] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[poss + poss*NUMV] += (a*a-2*a*c+b*b-2*b*d+c*c+d*d)/detA2;
        mat[post + posx*NUMV] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[post + posy*NUMV] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        mat[post + posz*NUMV] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[post + posw*NUMV] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        mat[post + post*NUMV] += (a*a-2*a*c+b*b-2*b*d+c*c+d*d)/detA2;
    }
    // incorporate constraints
    if(mainImage.numSelected==1){
        for(int k=0;k<mainImage.numVertices;k++){
            mat[0 + k*NUMV] = 0;
            mat[mainImage.numVertices + k*NUMV] = 0;
        }
        mat[0 + 0*NUMV] = 1.0;
        mat[mainImage.numVertices + mainImage.numVertices*NUMV] = 1.0;
    }
    for(int s=0;s<mainImage.numSelected;s++){
        int i=mainImage.selected[s];
        int j=i+mainImage.numVertices;
        for(int k=0;k<mainImage.numVertices;k++){
            mat[i + k*NUMV] = 0;
            mat[j + k*NUMV] = 0;
        }
        mat[i + i*NUMV] = 1.0;
        mat[j + j*NUMV] = 1.0;
    }
    return;
}

/**
 *  Buttons
 */

// Initialise
- (IBAction)pushButton_Initialize:(UIBarButtonItem *)sender {
    NSLog(@"Initialize");
    [mainImage initialize];
}

// Load new image
- (IBAction)pushButton_ReadImage:(UIBarButtonItem *)sender {
    if([UIImagePickerController
        isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]){
        UIImagePickerController *imagePicker = [[UIImagePickerController alloc] init];
        imagePicker.delegate = self;
        imagePicker.allowsEditing = YES;
        imagePicker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone){
            //iPhone の場合
            [self presentViewController:imagePicker animated:YES completion:nil];
        }else{
            //iPadの場合
            if(imagePopController!=NULL){
                [imagePopController dismissPopoverAnimated:YES];
            }
            imagePopController = [[UIPopoverController alloc] initWithContentViewController:imagePicker];
            [imagePopController presentPopoverFromBarButtonItem:sender
                                       permittedArrowDirections:UIPopoverArrowDirectionAny
                                                       animated:YES];
        }
    }else{
        NSLog(@"Photo library not available");
    }
}
#pragma mark -
#pragma mark UIImagePickerControllerDelegate implementation
// select image
- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info{
    GLuint name = mainImage.texture.name;
    glDeleteTextures(1, &name);
    UIImage *pImage = [info objectForKey: UIImagePickerControllerOriginalImage];
    mainImage = [[ImageMesh alloc] initWithUIImage:pImage VerticalDivisions:VDIV HorizontalDivisions:HDIV];
    [self loadTexture:pImage];
    [self setupScreen];
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone){
        [self dismissViewControllerAnimated:YES completion:nil];
    }else{
        [imagePopController dismissPopoverAnimated:YES];
    }
}
//cancel
- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker{
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone){
        [self dismissViewControllerAnimated:YES completion:nil];
    }else{
        [imagePopController dismissPopoverAnimated:YES];
    }
}

// show help screen
- (IBAction)pushButton_HowToUse:(UIBarButtonItem *)sender {
//    creditController = [[CreditController alloc] initWithNibName:@"CreditController" bundle:nil];
//    [self.view addSubview:creditController.view];
}


// Devise orientation
- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration{
    NSLog(@"Orientation changed:%ld",(long)self.interfaceOrientation);
    [self setupScreen];
}



/**
 *  termination procedure
 */
- (void)viewDidUnload {
    [super viewDidUnload];
    if (touchedPts)
        CFRelease(touchedPts);
}

@end
