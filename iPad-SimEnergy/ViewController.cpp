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
#include "Eigen/Sparse"
#include "Eigen/Dense"
#include <vector>
using namespace Eigen;

/// threshold for being zero
#define EPSILON 10e-6
//
#define MAX_TOUCHES 5
// the numbers of horizontal and vertical grids
#define HDIV 15
#define VDIV 15
// size of the linear system: two-times (x and y coordinates) the number of vertices
#define N 2*(HDIV+1)*(VDIV+1)
#define DEFAULTIMAGE @"Default.png"

@interface ViewController ()
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;
- (void)setupGL;
- (void)tearDownGL;
@end

@implementation ViewController
@synthesize effect;

// for Eigen
typedef SparseMatrix<float> SpMat;
typedef SparseLU<SpMat, COLAMDOrdering<int>> SpSolver;
typedef Triplet<float> T;
SpSolver solver;
MatrixXf U;
std::vector<MatrixXf> Pinv;
// local transformations
std::vector<Matrix2f> A;


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
    
    Pinv.resize(mainImage.numTriangles);
    for(int i=0;i<mainImage.numTriangles;i++){
        Pinv[i] = MatrixXf::Zero(3, 2);
    }
    A.resize(mainImage.numTriangles);
    
    // UI Setup
    mode = 0;
    iteration = 1;
        
    [self setupGL];
}

- (void)dealloc{    
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
    NSDictionary* options = @{GLKTextureLoaderOriginBottomLeft: @YES};
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
//    UIInterfaceOrientation orientation = [[UIApplication sharedApplication] statusBarOrientation];
    screen.height = [UIScreen mainScreen].bounds.size.height;
    screen.width = [UIScreen mainScreen].bounds.size.width;
    ratio = screen.height/screen.width;
    if (screen.width*mainImage.image_height<screen.height*mainImage.image_width) {
        gl_width = mainImage.image_width;
        gl_height = gl_width*ratio;
    }else{
        gl_height = mainImage.image_height;
        gl_width = gl_height/ratio;
    }
    ratio_height = gl_height / screen.height;
    ratio_width = gl_width / screen.width;
    
    GLKMatrix4 projectionMatrix = GLKMatrix4MakeOrtho(-gl_width/2.0, gl_width/2.0, -gl_height/2.0, gl_height/2.0, -1, 1);
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
            // touched location in OpenGL coordinates
            CGPoint p = [touch locationInView:self.view];
            p.x = (p.x - screen.width/2.0)*ratio_width;
            p.y = (screen.height/2.0 - p.y)*ratio_height;
            // nearest point will be selected
            int closest_vertex = 0;
            float min_dist = mainImage.radius;
            // !! the lower left corner is kept unselectable; it is reserved for the case when there's not enough constraint.
            for(int i=1;i<mainImage.numVertices;i++){
                float dist = (p.x-mainImage.x[i])*(p.x-mainImage.x[i])+(p.y-mainImage.y[i])*(p.y-mainImage.y[i]);
                if(dist<min_dist){
                    min_dist = dist;
                    closest_vertex = i;
                }
            }
            // if the nearest vertex is not too far from the touched point
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
// drag
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event {
    if(mainImage.numSelected==0) return;
    // update the coordinates of the touched points
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
    if(mode==1){
        [self solve_vertices_ARAP];
    }else if(mode==0){
        [self solve_vertices_Sim];
    }
    [mainImage deform];
}

// determine the location of un-constraint vertices by solving a linear system using Eigen
- (void)solve_vertices_Sim{
    VectorXf V = VectorXf::Zero(N);
    if(mainImage.numSelected==1){
        int index=mainImage.selected[0];
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

- (void)solve_vertices_ARAP{
    [self formArapRHS:A];
    MatrixXf Sol = solver.solve(U);
    // iterative refinement
    for(int iter=1;iter<iteration;iter++){
        std::vector<Matrix2f> A(mainImage.numTriangles);
        for(int i=0;i<mainImage.numTriangles;i++){
            int posx=mainImage.triangles[3*i];
            int posz=mainImage.triangles[3*i+1];
            int poss=mainImage.triangles[3*i+2];
            MatrixXf B(2,3);
            B << Sol(posx,0),Sol(posz,0),Sol(poss,0), Sol(posx,1),Sol(posz,1),Sol(poss,1);
            A[i] = [self Rotation:B*Pinv[i]];
        }
        [self formArapRHS:A];
        Sol = solver.solve(U);
    }
    // set coordinates
    for(int i=0;i<mainImage.numVertices;i++){
        mainImage.x[i] = Sol(i,0);
        mainImage.y[i] = Sol(i,1);
    }
}

// Rotation part of a matrix (Higham's algorithm)
- (Matrix2f)Rotation:(Matrix2f) X{
    Matrix2f Curr = X;
    Matrix2f Prev;
    do {
        Matrix2f Ad = Curr.inverse().transpose();
        double nad = Ad.array().abs().colwise().sum().maxCoeff() * Ad.array().abs().rowwise().sum().maxCoeff();
        double na = Curr.array().abs().colwise().sum().maxCoeff() * Curr.array().abs().rowwise().sum().maxCoeff();
        double gamma = sqrt(sqrt(nad / na));
        Prev = Curr;
        Curr = (0.5*gamma)*Curr + (0.5/gamma) *Ad;
    } while ((Prev-Curr).lpNorm<1>() > EPSILON*Prev.lpNorm<1>());
    return Curr;
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

// prepare the energy matrix
- (void)formEnergy{
    if(mainImage.numSelected==0){
        return;
    }
    // all starting points are updated
    for(int j=0;j<mainImage.numVertices;j++){
        mainImage.ix[j] = mainImage.x[j];
        mainImage.iy[j] = mainImage.y[j];
    }
    if(mode==1){
        [self formEnergy_ARAP];
        // clear local transformation
        for(int i=0;i<mainImage.numTriangles;i++){
            A[i] = Matrix2f::Identity();
        }
    }else if(mode==0){
        [self formEnergy_Sim];
    }
}

// Similarity invariant energy
- (void)formEnergy_Sim{
    SpMat G(N, N);
    std::vector<T> tripletListMat(0);
    tripletListMat.reserve(mainImage.numTriangles*30);
    // incorporate constraints
    bool avoid[mainImage.numVertices];
    for(int i=0;i<mainImage.numVertices;i++) avoid[i]=false;
    if(mainImage.numSelected==1){
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
    
    // compute the energy derivation matrix
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

// inverted mesh matrix
- (void)inverted_mesh_matrix{
    for(int i=0;i<mainImage.numTriangles;i++){
        int posx=mainImage.triangles[3*i];
        int posz=mainImage.triangles[3*i+1];
        int poss=mainImage.triangles[3*i+2];
        float a = mainImage.ix[posx];
        float b = mainImage.iy[posx];
        float c = mainImage.ix[posz];
        float d = mainImage.iy[posz];
        float e = mainImage.ix[poss];
        float f = mainImage.iy[poss];
        float detA = (a*d-a*f-b*c+b*e+c*f-d*e);
        Pinv[i] << d-f,-c+e, -b+f,a-e, b-d,-a+c;
        Pinv[i] /= detA;
    }
}

// ARAP energy
- (void)formEnergy_ARAP{
    [self inverted_mesh_matrix];
    int n=mainImage.numVertices;
    SpMat G(n, n);
    U = MatrixXf::Zero(n,2);
    std::vector<T> tripletListMat(0);
    tripletListMat.reserve(mainImage.numTriangles*9);
    // incorporate constraints
    bool avoid[mainImage.numVertices];
    for(int i=0;i<mainImage.numVertices;i++) avoid[i]=false;
    for(int s=0;s<mainImage.numSelected;s++){
        int i=mainImage.selected[s];
        avoid[i]=true;
        tripletListMat.push_back(T(i,i,1.0));
    }
    Matrix3f LHS;
    // compute the energy derivation matrix
    for(int i=0;i<mainImage.numTriangles;i++){
        int posx=mainImage.triangles[3*i];
        int posz=mainImage.triangles[3*i+1];
        int poss=mainImage.triangles[3*i+2];
        LHS = Pinv[i] * Pinv[i].transpose();
        // partial derivative of the energy |B-I|^2, where B=VP^{-1}
        // partial by x
        if(!avoid[posx]){
            tripletListMat.push_back(T(posx,posx,LHS(0,0)));
            tripletListMat.push_back(T(posx,posz,LHS(0,1)));
            tripletListMat.push_back(T(posx,poss,LHS(0,2)));
        }
        // partial by z
        if(!avoid[posz]){
            tripletListMat.push_back(T(posz,posx,LHS(1,0)));
            tripletListMat.push_back(T(posz,posz,LHS(1,1)));
            tripletListMat.push_back(T(posz,poss,LHS(1,2)));
        }
        // partial by s
        if(!avoid[poss]){
            tripletListMat.push_back(T(poss,posx,LHS(2,0)));
            tripletListMat.push_back(T(poss,posz,LHS(2,1)));
            tripletListMat.push_back(T(poss,poss,LHS(2,2)));
        }
    }
    G.setFromTriplets(tripletListMat.begin(), tripletListMat.end());
    solver.compute(G);
    return;
}

- (void)formArapRHS:(std::vector<Matrix2f>) A{
    U.setZero();
    for(int k=0;k<mainImage.numSelected;k++){
        int i=mainImage.selected[k];
        U.row(i) << mainImage.x[i], mainImage.y[i];
    }
    MatrixXf RHS(3,2);
    bool avoid[mainImage.numVertices];
    for(int i=0;i<mainImage.numVertices;i++) avoid[i]=false;
    for(int s=0;s<mainImage.numSelected;s++){
        avoid[mainImage.selected[s]]=true;
    }
    for(int i=0;i<mainImage.numTriangles;i++){
        int posx=mainImage.triangles[3*i];
        int posz=mainImage.triangles[3*i+1];
        int poss=mainImage.triangles[3*i+2];
        RHS = Pinv[i] * A[i].transpose();
        if(!avoid[posx]){
            U(posx,0) += RHS(0,0);
            U(posx,1) += RHS(0,1);
        }
        if(!avoid[posz]){
            U(posz,0) += RHS(1,0);
            U(posz,1) += RHS(1,1);
        }
        if(!avoid[poss]){
            U(poss,0) += RHS(2,0);
            U(poss,1) += RHS(2,1);
        }
    }
}

/**
 *  Buttons
 */

// Initialise
- (IBAction)pushButton_Initialize:(UIBarButtonItem *)sender {
    NSLog(@"Initialize");
    [mainImage initialize];
}

// snapshot
- (IBAction)pushSaveImg:(UIBarButtonItem *)sender{
    NSLog(@"saving image");
    UIImage* image = [(GLKView*)self.view snapshot];
    UIImageWriteToSavedPhotosAlbum(image, self, @selector(savingImageIsFinished:didFinishSavingWithError:contextInfo:), nil);
}
- (void) savingImageIsFinished:(UIImage *)_image didFinishSavingWithError:(NSError *)_error contextInfo:(void *)_contextInfo{
    NSMutableString *title = [NSMutableString string];
    NSMutableString *msg = [NSMutableString string];
    if(_error){
        [title setString:@"error"];
        [msg setString:@"Save failed."];
    }else{
        [title setString:@"Saved"];
        [msg setString:@"Image saved in Camera Roll"];
    }
    UIAlertController * ac = [UIAlertController alertControllerWithTitle:title
                                                                 message:msg
                                                          preferredStyle:UIAlertControllerStyleAlert];
    UIAlertAction * okAction =
    [UIAlertAction actionWithTitle:@"OK"
                             style:UIAlertActionStyleDefault
                           handler:^(UIAlertAction * action) {
                               NSLog(@"OK button tapped.");
                           }];
    [ac addAction:okAction];
    [self presentViewController:ac animated:YES completion:nil];
}

// Delta Slider
- (IBAction)iterationSliderChanged:(UISlider *)sender{
    iteration = (int)iterationSlider.value;
    iterationLabel.text = [NSString stringWithFormat:@"#Iter = %d",iteration];
}

// mode change
-(IBAction)pushSeg:(UISegmentedControl *)sender{
    mode = (int)sender.selectedSegmentIndex;
    [self formEnergy];
}


// Load new image
- (IBAction)pushButton_ReadImage:(UIBarButtonItem *)sender {
    if([UIImagePickerController
        isSourceTypeAvailable:UIImagePickerControllerSourceTypePhotoLibrary]){
        UIImagePickerController *imagePicker = [[UIImagePickerController alloc] init];
        imagePicker.delegate = self;
        imagePicker.allowsEditing = YES;
        imagePicker.sourceType = UIImagePickerControllerSourceTypePhotoLibrary;
        [self presentViewController:imagePicker animated:YES completion:nil];
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
    mainImage.image_width = (float)pImage.size.width;
    mainImage.image_height = (float)pImage.size.height;
    [self loadTexture:pImage];
    [self setupScreen];
    [self dismissViewControllerAnimated:YES completion:nil];
}
//cancel
- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker{
    [self dismissViewControllerAnimated:YES completion:nil];
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
