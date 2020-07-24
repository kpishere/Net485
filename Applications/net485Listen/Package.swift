// swift-tools-version:4.2
// The swift-tools-version declares the minimum version of Swift required to build this package.

#if os(Linux)
// Code specific to Linux
#elseif os(macOS)
// Code specific to macOS
#endif

#if canImport(UIKit)
// Code specific to platforms where UIKit is available
#endif
/*
import PackageDescription

let package = Package(
    name: "net485Listen",
    products: [
        // Products define the executables and libraries produced by a package, and make them visible to other packages.
        .library(
            name: "net485Listen",
 //           path: "bin",
            targets: ["net485Listen"]
            ),
    ],
    dependencies: [
        // Dependencies declare other packages that this package depends on.
        // .package(url: /* package url */, from: "1.0.0"),
         .package(/*path: "Frameworks/ORSSerialPort"*/
         url: "https://github.com/armadsen/ORSSerialPort.git", .branch("master") ),
    ],
    targets: [
        // Targets are the basic building blocks of a package. A target can define a module or a test suite.
        // Targets can depend on other targets in this package, and on products in packages which this package depends on.
        .target(
            name: "net485Listen"
            ,dependencies: ["Frameworks/ORSSerialPort"]
            , path: "Source"
            )
    ]
)
*/
