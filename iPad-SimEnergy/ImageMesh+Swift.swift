//
//  ImageMesh+Swift.swift
//  iPad-SimEnergy
//
//  Swift extension for ImageMesh to provide modern Swift conveniences
//

import Foundation
import GLKit
import UIKit

extension ImageMesh {
    
    // MARK: - Convenience Initializers
    
    convenience init(uiImage: UIImage, verticalDivisions: GLuint, horizontalDivisions: GLuint) {
        self.init(uiImage: uiImage, verticalDivisions: verticalDivisions, horizontalDivisions: horizontalDivisions)
    }
    
    // MARK: - Swift-friendly Properties
    
    var selectedVertices: [Int32] {
        get {
            guard selected != nil else { return [] }
            let buffer = UnsafeBufferPointer(start: selected, count: Int(numSelected))
            return Array(buffer)
        }
    }
    
    var vertexPositions: [(x: Float, y: Float)] {
        get {
            guard x != nil && y != nil else { return [] }
            var positions: [(x: Float, y: Float)] = []
            for i in 0..<Int(numVertices) {
                positions.append((x: x[i], y: y[i]))
            }
            return positions
        }
    }
    
    var initialVertexPositions: [(x: Float, y: Float)] {
        get {
            guard ix != nil && iy != nil else { return [] }
            var positions: [(x: Float, y: Float)] = []
            for i in 0..<Int(numVertices) {
                positions.append((x: ix[i], y: iy[i]))
            }
            return positions
        }
    }
    
    // MARK: - Vertex Manipulation
    
    func setVertexPosition(at index: Int, x: Float, y: Float) {
        guard index >= 0 && index < numVertices else { return }
        self.x[index] = x
        self.y[index] = y
    }
    
    func getVertexPosition(at index: Int) -> (x: Float, y: Float)? {
        guard index >= 0 && index < numVertices else { return nil }
        return (x: x[index], y: y[index])
    }
    
    func isVertexSelected(_ index: Int) -> Bool {
        for i in 0..<Int(numSelected) {
            if selected[i] == index {
                return true
            }
        }
        return false
    }
    
    // MARK: - Selection Management
    
    func clearSelection() {
        numSelected = 0
    }
    
    func selectVertex(at index: Int) -> Bool {
        guard index >= 0 && index < numVertices else { return false }
        guard !isVertexSelected(index) else { return false }
        guard numSelected < numVertices else { return false }
        
        selected[Int(numSelected)] = Int32(index)
        numSelected += 1
        return true
    }
    
    func deselectVertex(at index: Int) -> Bool {
        for i in 0..<Int(numSelected) {
            if selected[i] == index {
                // Shift remaining elements
                for j in i..<Int(numSelected - 1) {
                    selected[j] = selected[j + 1]
                }
                numSelected -= 1
                return true
            }
        }
        return false
    }
    
    // MARK: - Geometric Calculations
    
    func findNearestVertex(to point: (x: Float, y: Float), maxDistance: Float = -1) -> Int? {
        let searchRadius = maxDistance > 0 ? maxDistance : self.radius
        var nearestIndex: Int?
        var minDistance = searchRadius
        
        // Skip vertex 0 as it's reserved for constraint handling
        for i in 1..<Int(numVertices) {
            let dx = point.x - x[i]
            let dy = point.y - y[i]
            let distance = sqrt(dx * dx + dy * dy)
            
            if distance < minDistance {
                minDistance = distance
                nearestIndex = i
            }
        }
        
        return nearestIndex
    }
    
    func calculateMeshBounds() -> (min: (x: Float, y: Float), max: (x: Float, y: Float)) {
        guard numVertices > 0 else {
            return (min: (x: 0, y: 0), max: (x: 0, y: 0))
        }
        
        var minX = x[0], maxX = x[0]
        var minY = y[0], maxY = y[0]
        
        for i in 1..<Int(numVertices) {
            minX = min(minX, x[i])
            maxX = max(maxX, x[i])
            minY = min(minY, y[i])
            maxY = max(maxY, y[i])
        }
        
        return (min: (x: minX, y: minY), max: (x: maxX, y: maxY))
    }
    
    // MARK: - Debug and Utility
    
    func printMeshInfo() {
        print("ImageMesh Info:")
        print("  Vertices: \\(numVertices)")
        print("  Triangles: \\(numTriangles)")
        print("  Selected: \\(numSelected)")
        print("  Divisions: \\(horizontalDivisions) x \\(verticalDivisions)")
        print("  Image Size: \\(image_width) x \\(image_height)")
        print("  Touch Radius: \\(radius)")
        
        let bounds = calculateMeshBounds()
        print("  Bounds: (\\(bounds.min.x), \\(bounds.min.y)) to (\\(bounds.max.x), \\(bounds.max.y))")
    }
    
    func validateMeshIntegrity() -> Bool {
        // Check if arrays are properly allocated
        guard x != nil && y != nil && ix != nil && iy != nil else {
            print("Error: Vertex arrays not properly allocated")
            return false
        }
        
        guard triangles != nil else {
            print("Error: Triangle array not properly allocated")
            return false
        }
        
        guard selected != nil else {
            print("Error: Selection array not properly allocated")
            return false
        }
        
        // Check selection bounds
        for i in 0..<Int(numSelected) {
            let selectedIndex = selected[i]
            if selectedIndex < 0 || selectedIndex >= numVertices {
                print("Error: Invalid selected vertex index: \\(selectedIndex)")
                return false
            }
        }
        
        return true
    }
    
    // MARK: - Animation Support
    
    func interpolateTowards(target: ImageMesh, factor: Float) {
        guard target.numVertices == self.numVertices else { return }
        
        let clampedFactor = max(0, min(1, factor))
        
        for i in 0..<Int(numVertices) {
            x[i] = x[i] + (target.x[i] - x[i]) * clampedFactor
            y[i] = y[i] + (target.y[i] - y[i]) * clampedFactor
        }
    }
    
    func resetToInitialPosition() {
        guard ix != nil && iy != nil else { return }
        
        for i in 0..<Int(numVertices) {
            x[i] = ix[i]
            y[i] = iy[i]
        }
        
        clearSelection()
    }
}