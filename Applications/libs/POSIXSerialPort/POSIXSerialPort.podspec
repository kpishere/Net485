Pod::Spec.new do |s|

  s.name         = "POSIXSerialPort"
  s.version      = "0.1"
  s.summary      = "Indepenant library for Objective-C and Swift Mac apps."

  s.description  = <<-DESC
                   A simple, Cocoa-like library useful for programmers writing Objective-C or Swift apps for the Mac that communicate with external devices through a serial port. POSIXSerialPort connects to pty and cu devices in identical way, configure serial ports, and send and receive data. A pre-data arrived callback allows you to define parsing of packets in arbitrary way in your delegate.
                   DESC

  s.homepage     = "https://github.com/kpishere/POSIXSerialPort"
  s.license      = "GPL V2.0"
  s.author             = { "kpishere" => "https://github.com/kpishere" }
  s.social_media_url   = ''

  s.platform     = :osx, "10.13"

  s.source       = { :git => "https://github.com/kpishere/POSIXSerialPort.git", :tag => s.version.to_s }
  s.source_files  = "Source/**/*.{h,m}"
  s.private_header_files = ""

  s.module_name = 'POSIXSerial'
end
