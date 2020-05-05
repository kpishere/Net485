////
// Helper extensions/functions
//
extension String {
    var isInt: Bool {
        return Int(self) != nil
    }
}
extension Data {
    var asHexString: String {
        return reduce("") {$0 + String(format: " %02x", $1 as CVarArg)}
    }
}
func subclasses<T>(of theClass: T) -> [T] {
    var count: UInt32 = 0, result: [T] = []
    let allClasses = objc_copyClassList(&count)!

    for n in 0 ..< count {
        let someClass: AnyClass = allClasses[Int(n)]
        guard let someSuperClass = class_getSuperclass(someClass), String(describing: someSuperClass) == String(describing: theClass) else { continue }
        result.append(someClass as! T)
    }

    return result
}
//
// Helper extensions/functions
////
