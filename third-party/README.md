# Third-Party Dependencies

This directory contains external libraries used by the iPad-SimEnergy project.

## Eigen

**Version**: Latest from main branch  
**Source**: https://gitlab.com/libeigen/eigen.git  
**License**: Mozilla Public License 2.0 (MPL2) / GNU LGPL v3+

Eigen is a C++ template library for linear algebra: matrices, vectors, numerical solvers, and related algorithms. It's used in this project for:

- Sparse matrix operations in ARAP (As-Rigid-As-Possible) deformation
- Dense matrix computations
- Linear system solving
- Mathematical transformations for mesh deformation

### Usage in Project

The Eigen library is included in the C++ portions of the codebase:
- `ViewController.cpp` - Main mathematical computations
- Swift bridging header - For mixed C++/Swift compatibility

### Updating Eigen

To update the Eigen submodule to the latest version:

```bash
cd third-party/eigen
git pull origin master
cd ../..
git add third-party/eigen
git commit -m "Update Eigen submodule to latest version"
```

### Build Configuration

The Xcode project is configured to use this local Eigen installation via:
- **Header Search Paths**: `$(SRCROOT)/third-party/eigen`
- **Include statements**: `#include "../third-party/eigen/Eigen/..."`

This ensures the project builds without requiring system-wide Eigen installation.