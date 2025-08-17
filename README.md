# iPad-SimEnergy

**Similarity Invariant Energy Minimization for Interactive Image Deformation**

## Overview

iPad-SimEnergy is an interactive iPad application that allows users to deform images in real-time using multi-touch gestures. The app implements similarity invariant energy minimization algorithms, enabling natural and mathematically sound image deformation that preserves visual quality.

## Demo

![iPad-SimEnergy Demo](https://github.com/shizuo-kaji/iPad-SimEnergy/blob/master/iPad-SimEnergy.gif?raw=true)

### Key Features

- **Multi-touch Interaction**: Intuitive gesture-based deformation using up to 5 simultaneous touch points
- **Real-time Rendering**: Smooth deformation with OpenGL/GLKit rendering pipeline
- **Mathematical Accuracy**: LAPACK/Eigen-based energy minimization algorithms
- **Dual Algorithms**: Support for both Similarity and ARAP (As-Rigid-As-Possible) deformation
- **Modern Architecture**: Swift UI with Objective-C/C++ mathematical backend

## Screenshots

### Mathematical Foundation

The deformation algorithms are based on the research paper:

**[Mathematical Analysis on Affine Maps for 2D Shape Interpolation](https://dl.acm.org/doi/10.1145/2338714.2338727)**
*S. Kaji, S. Hirose, S. Sakata, Y. Mizoguchi, and K. Anjyo*
Proceedings of SCA2012 (ACM SIGGRAPH/Eurographics Symposium on Computer Animation 2012), pp. 71-76.

### Dependencies

- **Eigen**: Included as git submodule in `third-party/eigen/`
- **LAPACK**: Uses iOS Accelerate framework

## Usage

### Basic Operation

1. **Load Image**: Tap the image button to select a photo from your library
2. **Deform**: Use multi-touch gestures to drag and deform the image
3. **Reset**: Tap initialize to return to original state
4. **Save**: Export the deformed image to your photo library

### Advanced Features

- **Algorithm Selection**: Switch between Similarity and ARAP deformation modes
- **Iteration Control**: Adjust solver iterations for quality vs. performance
- **Real-time Preview**: See deformation results immediately during interaction

### Controls

- **Multi-touch Drag**: Primary deformation interaction
- **Algorithm Toggle**: Switch deformation methods
- **Iteration Slider**: Control mathematical solver precision
- **Reset Button**: Return to original image state

## Project Structure

```
iPad-SimEnergy/
├── README.md                          # This file
├── AGENTS.md                          # Swift migration guide
├── iPad-SimEnergy/
│   ├── AppDelegate.swift              # Swift application delegate
│   ├── ViewController.swift           # Modern Swift view controller
│   ├── MathematicalEngine.swift       # Swift wrapper for math operations
│   ├── ImageMesh+Swift.swift          # Swift extensions for ImageMesh
│   ├── iPad-SimEnergy-Bridging-Header.h # Swift/Objective-C bridge
│   ├── ImageMesh.h/.m                 # Objective-C mesh management
│   ├── ViewController.cpp             # C++ mathematical implementation
│   └── Images.xcassets/               # App icons and assets
├── third-party/
│   ├── eigen/                         # Eigen library (submodule)
│   └── README.md                      # Third-party documentation
└── iPad-SimEnergy.xcodeproj/          # Xcode project
```

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Citation

```bibtex
@inproceedings{Kaji2012,
  title={Mathematical Analysis on Affine Maps for 2D Shape Interpolation},
  author={Kaji, Shizuo and Hirose, S. and Sakata, S. and Mizoguchi, Y. and Anjyo, K.},
  booktitle={Proceedings of SCA2012 (ACM SIGGRAPH/Eurographics Symposium on Computer Animation 2012)},
  pages={71--76},
  year={2012}
}
```
