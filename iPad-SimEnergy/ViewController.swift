//
//  ViewController.swift
//  iPad-SimEnergy
//
//  Swift version of ViewController with multi-touch gesture handling
//

import UIKit
import GLKit
import CoreFoundation

class ViewController: GLKViewController {
    
    // MARK: - UI Components
    @IBOutlet weak var modeSwitch: UISwitch!
    @IBOutlet weak var iterationSlider: UISlider!
    @IBOutlet weak var iterationLabel: UILabel!
    
    // MARK: - Properties
    private var context: EAGLContext!
    private var effect: GLKBaseEffect!
    
    // Mesh data
    private var mainImage: ImageMesh!
    private var mathEngine: MathematicalEngine!
    
    // Touch tracking
    private var touchedPoints: [UITouch: Int] = [:]
    private let maxTouches = 5
    
    // Screen properties
    private var ratioHeight: Float = 1.0
    private var ratioWidth: Float = 1.0
    private var screenSize: CGSize = .zero
    
    // State
    private var mode: Int = 0
    private var iteration: Int = 1
    
    // Constants
    private let hDiv = 15
    private let vDiv = 15
    private let defaultImageName = "Default.png"
    
    // MARK: - View Lifecycle
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        setupOpenGL()
        setupMultiTouch()
        loadDefaultImage()
        setupUI()
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        updateScreenProperties()
    }
    
    deinit {
        tearDownGL()
    }
    
    // MARK: - Setup Methods
    
    private func setupOpenGL() {
        context = EAGLContext(api: .openGLES2)
        
        guard context != nil else {
            print("Failed to create ES context")
            return
        }
        
        let view = self.view as! GLKView
        view.context = context
        EAGLContext.setCurrent(context)
        
        setupGL()
    }
    
    private func setupMultiTouch() {
        view.isMultipleTouchEnabled = true
    }
    
    private func loadDefaultImage() {
        guard let image = UIImage(named: defaultImageName) else {
            print("Failed to load default image")
            return
        }
        
        mainImage = ImageMesh(uiImage: image, verticalDivisions: GLuint(vDiv), horizontalDivisions: GLuint(hDiv))
        mathEngine = MathematicalEngine(imageMesh: mainImage)
        loadTexture(image)
    }
    
    private func setupUI() {
        mode = 0
        iteration = 1
        updateIterationLabel()
    }
    
    private func updateScreenProperties() {
        screenSize = view.bounds.size
        
        // Calculate aspect ratios for coordinate conversion
        let aspectRatio = Float(screenSize.width / screenSize.height)
        ratioWidth = 2.0 / Float(screenSize.width)
        ratioHeight = 2.0 / Float(screenSize.height)
    }
    
    // MARK: - OpenGL Setup
    
    private func setupGL() {
        glEnable(GLenum(GL_DEPTH_TEST))
        glClearColor(0.65, 0.65, 0.65, 1.0)
        
        effect = GLKBaseEffect()
        effect.light0.enabled = GLboolean(GL_TRUE)
        effect.light0.diffuseColor = GLKVector4Make(1.0, 0.4, 0.4, 1.0)
    }
    
    private func tearDownGL() {
        EAGLContext.setCurrent(context)
        effect = nil
        EAGLContext.setCurrent(nil)
        context = nil
    }
    
    private func loadTexture(_ image: UIImage) {
        guard let cgImage = image.cgImage else { return }
        
        do {
            let textureInfo = try GLKTextureLoader.texture(with: cgImage, options: nil)
            mainImage.texture = textureInfo
        } catch {
            print("Error loading texture: \\(error)")
        }
    }
    
    // MARK: - Touch Handling
    
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard touches.count > 0 else { return }
        
        for touch in touches {
            handleTouchBegan(touch)
        }
    }
    
    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard mainImage.numSelected > 0 else { return }
        
        // Update coordinates of touched points
        for touch in touches {
            if let vertexIndex = touchedPoints[touch] {
                let point = convertTouchToOpenGLCoordinates(touch)
                mainImage.x[vertexIndex] = point.x
                mainImage.y[vertexIndex] = point.y
            }
        }
        
        // Perform deformation based on current mode
        performDeformation()
        mainImage.deform()
    }
    
    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            handleTouchEnded(touch)
        }
        formEnergy()
    }
    
    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        print("Touch cancelled - count: \\(event?.allTouches?.count ?? 0)")
        for touch in touches {
            handleTouchEnded(touch)
        }
    }
    
    // MARK: - Touch Helper Methods
    
    private func handleTouchBegan(_ touch: UITouch) {
        let point = convertTouchToOpenGLCoordinates(touch)
        
        // Find nearest vertex
        var closestVertex = 0
        var minDistance = mainImage.radius
        
        // Skip vertex 0 (lower left corner) - reserved for constraint handling
        for i in 1..<Int(mainImage.numVertices) {
            let dx = point.x - mainImage.x[i]
            let dy = point.y - mainImage.y[i]
            let distance = dx * dx + dy * dy
            
            if distance < minDistance {
                minDistance = distance
                closestVertex = i
            }
        }
        
        // If nearest vertex is close enough, select it
        if minDistance < mainImage.radius {
            touchedPoints[touch] = closestVertex
            mainImage.selected[Int(mainImage.numSelected)] = Int32(closestVertex)
            mainImage.numSelected += 1
            
            // Update vertex position
            mainImage.x[closestVertex] = point.x
            mainImage.y[closestVertex] = point.y
        }
    }
    
    private func handleTouchEnded(_ touch: UITouch) {
        if let _ = touchedPoints.removeValue(forKey: touch) {
            mainImage.numSelected -= 1
        }
    }
    
    private func convertTouchToOpenGLCoordinates(_ touch: UITouch) -> (x: Float, y: Float) {
        let touchPoint = touch.location(in: view)
        let x = Float((touchPoint.x - screenSize.width / 2.0) * CGFloat(ratioWidth))
        let y = Float((screenSize.height / 2.0 - touchPoint.y) * CGFloat(ratioHeight))
        return (x, y)
    }
    
    // MARK: - Mathematical Operations
    
    private func performDeformation() {
        if mode == 1 {
            // ARAP (As-Rigid-As-Possible) deformation
            solveVerticesARAP()
        } else {
            // Similarity deformation
            mathEngine.performDeformation()
        }
    }
    
    private func solveVerticesARAP() {
        // Placeholder for ARAP implementation
        // This would integrate with the existing C++ Eigen-based ARAP solver
        print("ARAP deformation not yet implemented in Swift version")
    }
    
    private func formEnergy() {
        mathEngine.formEnergyMatrix()
    }
    
    // MARK: - UI Actions
    
    @IBAction func readImageTapped(_ sender: UIBarButtonItem) {
        presentImagePicker()
    }
    
    @IBAction func initializeTapped(_ sender: UIBarButtonItem) {
        mainImage.initialize()
    }
    
    @IBAction func howToUseTapped(_ sender: UIBarButtonItem) {
        showHowToUseAlert()
    }
    
    @IBAction func modeSegmentChanged(_ sender: UISegmentedControl) {
        mode = sender.selectedSegmentIndex
    }
    
    @IBAction func iterationSliderChanged(_ sender: UISlider) {
        iteration = Int(sender.value)
        updateIterationLabel()
    }
    
    @IBAction func saveImageTapped(_ sender: UIBarButtonItem) {
        saveCurrentImage()
    }
    
    // MARK: - Helper Methods
    
    private func updateIterationLabel() {
        iterationLabel.text = "Iterations: \\(iteration)"
    }
    
    private func presentImagePicker() {
        let picker = UIImagePickerController()
        picker.delegate = self
        picker.sourceType = .photoLibrary
        picker.allowsEditing = false
        present(picker, animated: true)
    }
    
    private func showHowToUseAlert() {
        let alert = UIAlertController(
            title: "How to Use",
            message: "Multi-touch on the image to deform it. The app uses similarity invariant energy minimization for realistic deformation.",
            preferredStyle: .alert
        )
        alert.addAction(UIAlertAction(title: "OK", style: .default))
        present(alert, animated: true)
    }
    
    private func saveCurrentImage() {
        // Implementation for saving the current deformed image
        print("Save image functionality to be implemented")
    }
    
    // MARK: - GLKView Delegate
    
    override func glkView(_ view: GLKView, drawIn rect: CGRect) {
        glClear(GLbitfield(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT))
        
        effect.prepareToDraw()
        
        // Render the deformed mesh
        if let texture = mainImage.texture {
            effect.texture2d0.name = texture.name
            effect.texture2d0.enabled = GLboolean(GL_TRUE)
        }
        
        // Render mesh vertices and triangles
        renderMesh()
    }
    
    private func renderMesh() {
        // Enable vertex arrays
        glEnableVertexAttribArray(GLuint(GLKVertexAttrib.position.rawValue))
        glEnableVertexAttribArray(GLuint(GLKVertexAttrib.texCoord0.rawValue))
        
        // Set vertex pointers
        glVertexAttribPointer(
            GLuint(GLKVertexAttrib.position.rawValue),
            2,
            GLenum(GL_FLOAT),
            GLboolean(GL_FALSE),
            0,
            mainImage.verticesArr
        )
        
        glVertexAttribPointer(
            GLuint(GLKVertexAttrib.texCoord0.rawValue),
            2,
            GLenum(GL_FLOAT),
            GLboolean(GL_FALSE),
            0,
            mainImage.textureCoordsArr
        )
        
        // Draw elements
        glDrawElements(
            GLenum(GL_TRIANGLE_STRIP),
            GLsizei(mainImage.indexArrsize),
            GLenum(GL_UNSIGNED_INT),
            mainImage.vertexIndices
        )
        
        // Disable vertex arrays
        glDisableVertexAttribArray(GLuint(GLKVertexAttrib.position.rawValue))
        glDisableVertexAttribArray(GLuint(GLKVertexAttrib.texCoord0.rawValue))
    }
}

// MARK: - UIImagePickerControllerDelegate

extension ViewController: UIImagePickerControllerDelegate, UINavigationControllerDelegate {
    
    func imagePickerController(_ picker: UIImagePickerController, didFinishPickingMediaWithInfo info: [UIImagePickerController.InfoKey : Any]) {
        picker.dismiss(animated: true)
        
        guard let image = info[.originalImage] as? UIImage else { return }
        
        // Reload mesh with new image
        mainImage = ImageMesh(uiImage: image, verticalDivisions: GLuint(vDiv), horizontalDivisions: GLuint(hDiv))
        mathEngine = MathematicalEngine(imageMesh: mainImage)
        loadTexture(image)
    }
    
    func imagePickerControllerDidCancel(_ picker: UIImagePickerController) {
        picker.dismiss(animated: true)
    }
}