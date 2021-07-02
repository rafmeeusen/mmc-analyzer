# mmc-analyzer

Low level Saleae MMC analyzer


Restrictions:
- currently only using on Linux 64-bit
- only tried with AnalyzerSDK commit e4aa95e8484fb3fc0631ccabac517c845f256289 (Mon Nov 25 16:16:22 2019) 
- don't remember exact build dependencies ... probably assuming build-essentials package or so is installed

Setup:
- for minimal changes to original Sample Analyzer from Saleae, clone the SDK inside this project: 
    $ git clone https://github.com/saleae/AnalyzerSDK.git
  (note: dir AnalyzerSDK is in gitignore file)
- small modif in AnalyzerSDK when building for 64-bit Linux:
    $ mv AnalyzerSDK/lib/libAnalyzer64.so AnalyzerSDK/lib/libAnalyzer.so

Building:
$ python build_analyzer.py


