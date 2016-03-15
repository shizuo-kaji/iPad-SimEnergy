//
//  solve_LAPACK.h
//  iPad-SimEnergy
//
//  Created by 鍛冶静雄 on 15/3/16.
//  Copyright © 2016 mcg-q. All rights reserved.
//
// (optional) Use LAPACK instead of Eigen

#ifndef solve_LAPACK_h
#define solve_LAPACK_h

#import <Accelerate/Accelerate.h>

__CLPK_integer IPIV[N];
float A[N*N], mat[N*N];
float vec[N];

// determine the location of un-constraint vertices by solving a linear system using LAPACK
- (void)solve_vertices_LAPACK{
    // clear constraint vector
    for(int i=0;i<N;i++){
        vec[i]=0;
    }
    // when only a single vertex is constrained, add a constraint for the lower left vertex to kill the indeterminacy
    if(mainImage.numSelected==1){
        int index=mainImage.selected[mainImage.numSelected-1];
        mainImage.x[0] = mainImage.ix[0] + mainImage.x[index]-mainImage.ix[index];
        mainImage.y[0] = mainImage.iy[0] + mainImage.y[index]-mainImage.iy[index];
        vec[0] = mainImage.x[0];
        vec[mainImage.numVertices] = mainImage.y[0];
    }
    // constrain touched vertices
    for(int k=0;k<mainImage.numSelected;k++){
        int i=mainImage.selected[k];
        int j=i+mainImage.numVertices;
        vec[i] = mainImage.x[i];
        vec[j] = mainImage.y[i];
    }
    __CLPK_integer size=N, NRHS=1, LDA=N, LDB=N, INFO;
    // LAPACK destroys original matrix so mat should be duplicated
    memcpy(A, mat, sizeof(float) * N * N);
    sgesv_(&size, &NRHS, A, &LDA, IPIV, vec, &LDB, &INFO);
    // could be optimised by re-using the factrisation (the matrix is invariant during a drag)
    //    NSLog(@"LAPACK: %ld", INFO);
    for(int i=0;i<mainImage.numVertices;i++){
        mainImage.x[i] = vec[i];
        mainImage.y[i] = vec[i+mainImage.numVertices];
    }
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
        mat[posx + posx*N] += (c*c-2*c*e+d*d-2*d*f+e*e+f*f)/detA2;
        mat[posx + posz*N] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posx + posw*N] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posx + poss*N] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        mat[posx + post*N] += + (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posy + posy*N] += (c*c-2*c*e+d*d-2*d*f+e*e+f*f)/detA2;
        mat[posy + posz*N] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posy + posw*N] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posy + poss*N] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posy + post*N] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        // partial by z and w
        mat[posz + posx*N] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posz + posy*N] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posz + posz*N] += (a*a-2*a*e+b*b-2*b*f+e*e+f*f)/detA2;
        mat[posz + poss*N] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        mat[posz + post*N] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posw + posx*N] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[posw + posy*N] += (-a*c+a*e-b*d+b*f+c*e+d*f-e*e-f*f)/detA2;
        mat[posw + posw*N] += (a*a-2*a*e+b*b-2*b*f+e*e+f*f)/detA2;
        mat[posw + poss*N] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[posw + post*N] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        // partial by s and t
        mat[poss + posx*N] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        mat[poss + posy*N] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[poss + posz*N] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        mat[poss + posw*N] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[poss + poss*N] += (a*a-2*a*c+b*b-2*b*d+c*c+d*d)/detA2;
        mat[post + posx*N] += (a*d-a*f-b*c+b*e+c*f-d*e)/detA2;
        mat[post + posy*N] += (a*c-a*e+b*d-b*f-c*c+c*e-d*d+d*f)/detA2;
        mat[post + posz*N] += (-a*d+a*f+b*c-b*e-c*f+d*e)/detA2;
        mat[post + posw*N] += (-a*a+a*c+a*e-b*b+b*d+b*f-c*e-d*f)/detA2;
        mat[post + post*N] += (a*a-2*a*c+b*b-2*b*d+c*c+d*d)/detA2;
    }
    // incorporate constraints
    if(mainImage.numSelected==1){
        for(int k=0;k<mainImage.numVertices;k++){
            mat[0 + k*N] = 0;
            mat[mainImage.numVertices + k*N] = 0;
        }
        mat[0 + 0*N] = 1.0;
        mat[mainImage.numVertices + mainImage.numVertices*N] = 1.0;
    }
    for(int s=0;s<mainImage.numSelected;s++){
        int i=mainImage.selected[s];
        int j=i+mainImage.numVertices;
        for(int k=0;k<mainImage.numVertices;k++){
            mat[i + k*N] = 0;
            mat[j + k*N] = 0;
        }
        mat[i + i*N] = 1.0;
        mat[j + j*N] = 1.0;
    }
    return;
}



#endif /* solve_LAPACK_h */
